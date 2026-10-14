#include "stdafx.h"

#ifdef HELICOPTER_NEW
#include "HelicopterAddon.h"

CHelicopterWeapon::CHelicopterWeapon(CGameObject* O, LPCSTR name)
{
    R_ASSERT(O);
    m_name._set(name);
    m_object = O;
    m_attachment = nullptr;
}

CHelicopterWeapon::~CHelicopterWeapon()
{
    m_armaments.clear();
    m_object->remove_attachment(m_attachment);
    m_object = nullptr;
    m_attachment = nullptr;
}

void CHelicopterWeapon::net_Spawn()
{
    CInifile* ini = m_object->Visual()->dcast_PKinematics()->LL_UserData();
    Activate(!!READ_IF_EXISTS(ini, r_bool, Name(), "active", FALSE));
}

void CHelicopterWeapon::net_Destroy()
{
    m_armaments.clear();
}

void CHelicopterWeapon::Activate(bool val)
{
    m_object->remove_attachment(m_attachment);
    m_attachment = nullptr;
    if (val)
    {
        const Fvector vec_zero = Fvector().set(0, 0, 0);
        LPCSTR sec = Name();
        IKinematics* K = m_object->Visual()->dcast_PKinematics();
        CInifile* ini = K->LL_UserData();
        R_ASSERT2(ini->section_exist(sec), FormatString("[%s] %s", m_object->cNameSect_str(), sec).c_str());

        u16 bone_bid = K->LL_BoneID(READ_IF_EXISTS(ini, r_string, sec, "attach_bone", nullptr));
        R_ASSERT2(bone_bid != BI_NONE, FormatString("[%s] %s", m_object->cNameSect_str(), sec).c_str());
        LPCSTR visual = READ_IF_EXISTS(ini, r_string, sec, "visual", nullptr);
        R_ASSERT2(visual, FormatString("[%s] %s", m_object->cNameSect_str(), sec).c_str());

        m_attachment = xr_new<script_attachment>(sec, visual);
        R_ASSERT2(m_attachment, FormatString("[%s] %s", m_object->cNameSect_str(), sec).c_str());
        m_attachment->SetType(script_attachment_type::eSA_World);
        m_attachment->SetParent(m_object);
        m_attachment->SetParentBone(bone_bid);
        m_attachment->SetPosition(READ_IF_EXISTS(ini, r_fvector3, sec, "attach_position", vec_zero));
        m_attachment->SetRotation(READ_IF_EXISTS(ini, r_fvector3, sec, "attach_rotation", vec_zero));
    }
    LoadArmaments();
}

void CHelicopterWeapon::LoadArmaments()
{
    m_armaments.clear();
    if (Attachment())
    {
        CInifile* ini = Attachment()->dcast_PKinematics()->LL_UserData();
        LPCSTR str = READ_IF_EXISTS(ini, r_string, "definition", "armaments", nullptr);
        string128 sec;
        m_armaments.reserve(_GetItemCount(str));
        for (int k = 0, n = _GetItemCount(str); k < n; ++k)
        {
            _GetItem(str, k, sec);
            VERIFY2(strlen(sec) && ini->section_exist(sec), sec);
            m_armaments.emplace_back(m_object, this, sec);
            m_armaments.back().Load(READ_IF_EXISTS(ini, r_string, sec, "wpn_section", nullptr));
        }
    }
}

void CHelicopterWeapon::UpdateCL()
{
    for (auto& I : m_armaments)
    {
        I.UpdateCL();
    }
}

void CHelicopterWeapon::renderable_Render()
{
    for (auto& I : m_armaments)
    {
        I.renderable_Render();
    }
}

::luabind::object CHelicopterWeapon::ArmamentList()
{
    ::luabind::object res = ::luabind::newtable(ai().script_engine().lua());
    int idx = 1;
    for (auto& I : m_armaments)
    {
        res[idx] = I.Name();
        idx++;
    }
    return res;
}

CArmament* CHelicopterWeapon::Armament(LPCSTR sec)
{
    for (auto& I : m_armaments)
    {
        if (sec && strcmp(I.Name(), sec) == 0)
        {
            return &I;
        }
    }
    return nullptr;
}

void CHelicopterWeapon::Fire(bool val)
{
    for (auto& I : m_armaments)
    {
        I.Fire(val);
    }
}

/*----------------------------------------------------------------------------------------------------
    CArmament
----------------------------------------------------------------------------------------------------*/
CArmament::CArmament(CGameObject* O, CHelicopterWeapon* W, LPCSTR name)
{
    R_ASSERT(O);
    R_ASSERT(W);
    m_name._set(name);
    m_object = O;
    m_weapon = W;

    m_fire_xfm.identity();
    m_fire_pos.set(0, 0, 0);
    m_fire_dir.set(0, 0, 1);

    IKinematics* K = Weapon()->Attachment()->dcast_PKinematics();
    CInifile* ini = K->LL_UserData();
    LPCSTR sec = Name();
    VERIFY(ini);

    m_bone_fire.Init(Weapon());
    m_bone_drop.Init(Weapon());

    m_unlimited_ammo = true;
    m_ammoType = 0;
    m_ammoTypes.clear();
    iMagazineSize = 0;
    iAmmoElapsed = 0;

    u16 bid;
    Fvector pos;
    Fvector hpb;
    const Fvector vec_zero = Fvector().set(0, 0, 0);
    {
        bid = ini->line_exist(sec, "fire_bone") ? K->LL_BoneID(ini->r_string(sec, "fire_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, sec, "fire_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, sec, "fire_rotation", vec_zero);
        m_bone_fire.Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(sec, "drop_bone") ? K->LL_BoneID(ini->r_string(sec, "drop_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, sec, "drop_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, sec, "drop_rotation", vec_zero);
        m_bone_drop.Load(bid, pos, hpb);
    }

    m_gatling_bid = ini->line_exist(sec, "gatling") ? K->LL_BoneID(ini->r_string(sec, "gatling")) : BI_NONE;
    m_gatling_inc = READ_IF_EXISTS(ini, r_float, sec, "gatling_num", 1.0F);
    VERIFY2(m_gatling_inc > 0.0F, sec);
    m_gatling_inc = PI_MUL_2 / m_gatling_inc * (READ_IF_EXISTS(ini, r_bool, sec, "gatling_dir", FALSE) ? 1.0F : -1.0F);
    m_gatling_ang = 0.0F;

    SetGatlingBoneCallbacks(true);
}

CArmament::~CArmament()
{
    StopFlameParticles();
    StopLight();
    Light_Destroy();
    SetGatlingBoneCallbacks(false);

    m_object = nullptr;
    m_weapon = nullptr;
}

void CArmament::Load(LPCSTR section)
{
    inherited::Load(section);
    m_unlimited_ammo = !!READ_IF_EXISTS(pSettings, r_bool, section, "unlimited_ammo", true);

    m_ammoType = 0;
    m_ammoTypes.clear();
    LPCSTR str = pSettings->r_string(section, "ammo_class");
    if (str && strlen(str))
    {
        string128 tmp;
        for (int k = 0, n = _GetItemCount(str); k < n; ++k)
        {
            _GetItem(str, k, tmp);
            m_ammoTypes.push_back(tmp);
        }
    }
    R_ASSERT2(m_ammoTypes.size(), section);
    m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);

    iMagazineSize = READ_IF_EXISTS(pSettings, r_s32, section, "ammo_mag_size", 0);
    SetAmmoElapsed(READ_IF_EXISTS(pSettings, r_s32, section, "ammo_elapsed", iMagazineSize));

    m_sounds.LoadSound(section, "snd_shoot", "sndShoot", false, ESoundTypes::SOUND_TYPE_WEAPON_SHOOTING);
}

void CArmament::UpdateCL()
{
    m_fire_xfm.set(m_bone_fire.XFORM());
    m_fire_pos.set(0, 0, 0);
    m_fire_xfm.transform_tiny(m_fire_pos);
    m_fire_dir.set(0, 0, 1);
    m_fire_xfm.transform_dir(m_fire_dir);

    UpdateFire();
    UpdateGatling();
}

void CArmament::renderable_Render()
{
    RenderLight();
}

void CArmament::Fire(bool val)
{
    if (val == true && (IsUnlimitedAmmo() || iAmmoElapsed > 0))
    {
        FireStart();
    }
    if (val != true)
    {
        FireEnd();
    }
}

void CArmament::UpdateFire()
{
    inherited::UpdateFlameParticles();
    inherited::UpdateLight();

    if (!IsWorking())
    {
        fShotTimeCounter = 0.0;
        return;
    }

    fShotTimeCounter -= Device.fTimeDelta;

    while (fShotTimeCounter <= 0)
    {
        if (!IsUnlimitedAmmo() && iAmmoElapsed == 0)
        {
            FireEnd();
            return;
        }
        fShotTimeCounter += fOneShotTime;
        OnShot();
    }
}

void CArmament::OnShot()
{
    FireBullet(m_fire_pos, m_fire_dir, fireDispersionBase, m_DefaultCartridge, m_object->ID(), m_object->ID(), true, iMagazineSize - iAmmoElapsed);

    StartShotParticles();

    if (m_bLightShotEnabled)
    {
        Light_Start();
    }

    StartFlameParticles();
    StartSmokeParticles(m_fire_pos, zero_vel);

    OnShellDrop(m_bone_drop.XFORM(), zero_vel);
    m_sounds.PlaySound("sndShoot", m_fire_pos, m_object, false);
}

void CArmament::OnShellDrop(const Fmatrix& xfm, const Fvector& vel)
{
    if (m_sShellParticles.size() == 0)
        return;
    CParticlesObject* P = CParticlesObject::Create(m_sShellParticles.c_str(), TRUE);
    P->UpdateParent(xfm, vel);
    P->Play(false);
}

void CArmament::SetAmmoType(u8 type)
{
    m_ammoType = type;
    if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
    {
        m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
    }
}

void CArmament::SetAmmoElapsed(int num)
{
    iAmmoElapsed = std::max(num, 0);
}

void CArmament::SetGatlingBoneCallbacks(bool val)
{
    if (val)
    {
        if (m_gatling_bid != BI_NONE)
        {
            CBoneInstance& B = Weapon()->Attachment()->dcast_PKinematics()->LL_GetBoneInstance(m_gatling_bid);
            B.set_callback(bctCustom, GatlingBoneCallback, this);
        }
    }
    else
    {
        if (m_gatling_bid != BI_NONE)
        {
            CBoneInstance& B = Weapon()->Attachment()->dcast_PKinematics()->LL_GetBoneInstance(m_gatling_bid);
            B.reset_callback();
        }
    }
}

void CArmament::GatlingBoneCallback(CBoneInstance* B)
{
    CArmament* P = static_cast<CArmament*>(B->callback_param());
    B->mTransform.mulB_43(Fmatrix().rotateZ(P->m_gatling_ang));
}

void CArmament::UpdateGatling()
{
    if (m_gatling_bid == BI_NONE)
        return;
    if (!IsWorking())
        return;
    m_gatling_ang = angle_normalize(m_gatling_ang + (m_gatling_inc * Device.fTimeDelta / fOneShotTime));
}

/*----------------------------------------------------------------------------------------------------
    SBone
----------------------------------------------------------------------------------------------------*/
CArmament::SBone::SBone()
{
    wpn = nullptr;
    xfm.identity();
    offset_bid = BI_NONE;
    offset_pos.set(0, 0, 0);
    offset_hpb.set(0, 0, 0);
    offset_xfm.identity();
}

CArmament::SBone::~SBone()
{
    wpn = nullptr;
}

void CArmament::SBone::Init(CHelicopterWeapon* O)
{
    wpn = O;
}

void CArmament::SBone::Load(u16& bid, Fvector& pos, Fvector& hpb)
{
    offset_bid = (bid != BI_NONE) ? bid : wpn->Attachment()->dcast_PKinematics()->LL_GetBoneRoot();
    offset_pos.set(pos);
    offset_hpb.set(hpb);
    offset_xfm.identity().setHPB(deg2rad(hpb.x), deg2rad(hpb.y), deg2rad(hpb.z)).translate_over(pos);
}

Fmatrix& CArmament::SBone::XFORM()
{
    xfm.mul_43(wpn->Attachment()->dcast_PKinematics()->LL_GetTransform(offset_bid), offset_xfm);
    xfm.mulA_43(wpn->Attachment()->GetTransform());
    return xfm;
}
#endif