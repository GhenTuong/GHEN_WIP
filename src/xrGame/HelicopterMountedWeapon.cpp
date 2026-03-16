#include "pch_script.h"
#ifdef HELICOPTER_NEW
#include "helicopter.h"

CWeaponMountedGun* CHelicopter::GetMountedWeapon(LPCSTR name)
{
    if (name == nullptr)
        return nullptr;
    for (auto I : m_mounted_weapons)
    {
        if (I.IsAttachName(name))
        {
            return &I;
        }
    }
    return nullptr;
}

BOOL CHelicopter::MountedWeapon_net_Spawn(CSE_Abstract* DC)
{
    IKinematics* K = Visual()->dcast_PKinematics();
    CInifile* ini = K->LL_UserData();
    m_mounted_weapons.clear();
    if (ini->line_exist(cNameSect_str(), "mounted_weapons"))
    {
        LPCSTR str = ini->r_string(cNameSect_str(), "mounted_weapons");
        string128 sec;
        int n = _GetItemCount(str);
        for (int i = 0; i < n; ++i)
        {
            _GetItem(str, i, sec);
            if (strlen(sec))
            {
                u16 bid = K->LL_BoneID(ini->r_string(sec, "bone"));
                m_mounted_weapons.emplace_back(this, sec, bid);
                m_mounted_weapons.back().Load(sec);
            }
        }
    }
    return TRUE;
}

void CHelicopter::MountedWeapon_net_Destroy()
{
    m_mounted_weapons.clear();
}

void CHelicopter::MountedWeapon_UpdateCL()
{
    for (auto I : m_mounted_weapons)
    {
        I.UpdateCL();
    }
}

void CHelicopter::MountedWeapon_renderable_Render()
{
    for (auto I : m_mounted_weapons)
    {
        I.renderable_Render();
    }
}
#endif