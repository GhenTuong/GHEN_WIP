#include "stdafx.h"
#include "WeaponMountedGun.h"

#include "../xrphysics/PhysicsShell.h"
#include "../Include/xrRender/Kinematics.h"


CWeaponMountedGun::CWeaponMountedGun(CGameObject *obj, LPCSTR sec, u16 bid)
{
    R_ASSERT(obj);
    R_ASSERT(sec);
    R_ASSERT(bid != BI_NONE);
    m_object = obj;
    m_attach_name._set(sec);
    m_attach_bone = bid;

    m_enable = false;
    m_bActive = false;

    m_rotate_x_bone = BI_NONE;
    m_rotate_y_bone = BI_NONE;
    m_fire_bone = BI_NONE;
    m_lim_x_rot.set(0.0F, 0.0F);
    m_lim_y_rot.set(0.0F, 0.0F);
    m_fire_pos.set(0.0F, 0.0F, 0.0F);
    m_fire_dir.set(0.0F, 0.0F, 1.0F);

    m_bind_x_rot = 0.0F;
    m_bind_y_rot = 0.0F;
    m_cur_x_rot = 0.0F;
    m_cur_y_rot = 0.0F;
    m_tgt_x_rot = 0.0F;
    m_tgt_x_rot = 0.0F;

    m_desired_idx = eDesiredAng;
    m_desired_pos.set(0, 0, 0);
    m_desired_dir.set(0, 0, 1);
    m_desired_ang.set(0, 0, 0);

    /* Ammo */
    m_iShotNum = 0;
    CShootingObject::Light_Create();
}

CWeaponMountedGun::~CWeaponMountedGun()
{
    SetEnable(false);
    m_ammoTypes.clear();
    m_object = nullptr;

    CShootingObject::StopFlameParticles();
    CShootingObject::StopLight();
    CShootingObject::Light_Destroy();
}

void CWeaponMountedGun::AttachGunner(CGameObject *obj)
{
    if (m_enable == false)
        return;
    if (Gunner())
        return;
    if (obj == nullptr)
        return;
    m_gunner = obj;

}

void CWeaponMountedGun::DetachGunner()
{
    m_gunner = nullptr;
}

void CWeaponMountedGun::renderable_Render()
{
    CShootingObject::RenderLight();
}

void CWeaponMountedGun::LoadWeapon(LPCSTR section)
{
    CShootingObject::Load(section);
    m_weapon_name._set(section);

    m_sounds.LoadSound(section, "snd_shoot", "sndShoot", false, SOUND_TYPE_WEAPON_SHOOTING);
    m_sounds.LoadSound(section, "snd_empty", "sndEmpty", false, SOUND_TYPE_WEAPON_EMPTY_CLICKING);
    m_sounds.LoadSound(section, "snd_reload", "sndReload", true, SOUND_TYPE_WEAPON_RECHARGING);
    m_sounds.LoadSound(section, "snd_unload", "sndUnload", true, SOUND_TYPE_WEAPON_RECHARGING);

    m_ammoTypes.clear();
    LPCSTR ammo_class = pSettings->r_string(section, "ammo_class");
    if (ammo_class && strlen(ammo_class))
    {
        string128 tmp;
        int n = _GetItemCount(ammo_class);
        for (int i = 0; i < n; ++i)
        {
            _GetItem(ammo_class, i, tmp);
            m_ammoTypes.push_back(tmp);
        }
    }
    R_ASSERT(m_ammoTypes.size());
    m_ammoType = 0;
    m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
}

void CWeaponMountedGun::SetEnable(bool flag)
{
    m_enable = flag;
    if (m_enable)
    {
        LPCSTR vis = READ_IF_EXISTS(pSettings, r_string, WeaponName(), "visual", nullptr);
        R_ASSERT(vis);

        object()->remove_attachment(AttachName());
        m_atm = xr_new<script_attachment>(AttachName(), vis);
        R_ASSERT(atm());
        atm()->SetType(script_attachment_type::eSA_World);
        atm()->SetParent(object());
        atm()->SetParentBone(m_attach_bone);
        atm()->spatial.type |= STYPE_FEELVISIONIGNORE;

        IKinematics *K = atm()->dcast_PKinematics();
        R_ASSERT(K);
        CInifile *ini = K->LL_UserData();
        R_ASSERT(ini);
        const LPCSTR mwd = "mounted_weapon_definition";

        m_rotate_x_bone = K->LL_BoneID(ini->r_string(mwd, "rotate_x_bone"));
        m_rotate_y_bone = K->LL_BoneID(ini->r_string(mwd, "rotate_y_bone"));
        m_fire_bone = K->LL_BoneID(ini->r_string(mwd, "fire_bone"));
        m_rotate_x_speed = deg2rad(READ_IF_EXISTS(ini, r_float, mwd, "rotate_x_speed", 10.0F));
        m_rotate_y_speed = deg2rad(READ_IF_EXISTS(ini, r_float, mwd, "rotate_y_speed", 10.0F));

        CBoneData &BDX = K->LL_GetData(m_rotate_x_bone);
        VERIFY(BDX.IK_data.type == jtJoint);
        m_lim_x_rot.set(BDX.IK_data.limits[0].limit.x, BDX.IK_data.limits[0].limit.y);
        CBoneData &BDY = K->LL_GetData(m_rotate_y_bone);
        VERIFY(BDY.IK_data.type == jtJoint);
        m_lim_y_rot.set(BDY.IK_data.limits[1].limit.x, BDY.IK_data.limits[1].limit.y);

        xr_vector<Fmatrix> matrices;
        K->LL_GetBindTransform(matrices);
        m_i_bind_x_xform.invert(matrices[m_rotate_x_bone]);
        m_i_bind_y_xform.invert(matrices[m_rotate_y_bone]);
        m_bind_x_rot = matrices[m_rotate_x_bone].k.getP();
        m_bind_y_rot = matrices[m_rotate_y_bone].k.getH();

        m_cur_x_rot = 0;
        m_cur_y_rot = 0;
        m_tgt_x_rot = 0;
        m_tgt_y_rot = 0;
    }
    else
    {
        DetachGunner();
        object()->remove_attachment(AttachName());
        m_atm = nullptr;
    }
}

void CWeaponMountedGun::UpdateCL()
{
    if (m_enable == false)
    {
        return;
    }

    atm()->dcast_PKinematics()->CalculateBones();

    UpdateBarrelDir();
    UpdateFire();
}

const Fvector &CWeaponMountedGun::get_CurrentFirePoint()
{
    return m_fire_pos;
}

const Fmatrix &CWeaponMountedGun::get_ParticlesXFORM()
{
    return m_fire_bone_xform;
}

void CWeaponMountedGun::BoneCallbackX(CBoneInstance *B)
{
    CWeaponMountedGun *wmg = static_cast<CWeaponMountedGun *>(B->callback_param());
    Fmatrix xfm;
    xfm.rotateX(wmg->m_cur_x_rot);
    B->mTransform.mulB_43(xfm);
}

void CWeaponMountedGun::BoneCallbackY(CBoneInstance *B)
{
    CWeaponMountedGun *wmg = static_cast<CWeaponMountedGun *>(B->callback_param());
    Fmatrix xfm;
    xfm.rotateY(wmg->m_cur_y_rot);
    B->mTransform.mulB_43(xfm);
}

void CWeaponMountedGun::BoneCallbacks(bool flag)
{
    if (atm() == nullptr)
    {
        return;
    }

    IKinematics *K = atm()->dcast_PKinematics();
    CBoneInstance &BX = K->LL_GetBoneInstance(m_rotate_x_bone);
    CBoneInstance &BY = K->LL_GetBoneInstance(m_rotate_y_bone);

    if (flag)
    {
        BX.set_callback(bctCustom, BoneCallbackX, this, TRUE);
        BY.set_callback(bctCustom, BoneCallbackY, this, TRUE);
    }
    else
    {
#if 0
        BX.set_callback(bctPhysics, PPhysicsShell()->GetBonesCallback(), PPhysicsShell()->get_Element(m_rotate_x_bone), TRUE);
        BY.set_callback(bctPhysics, PPhysicsShell()->GetBonesCallback(), PPhysicsShell()->get_Element(m_rotate_y_bone), TRUE);
#else
        BX.reset_callback();
        BY.reset_callback();
#endif
    }
}

void CWeaponMountedGun::UpdateBarrelDir()
{
    Fmatrix xfm = atm()->GetTransform();
    IKinematics *K = atm()->dcast_PKinematics();

    /* Update fire bone. */
    m_fire_bone_xform = K->LL_GetTransform(m_fire_bone);
    m_fire_bone_xform.mulA_43(xfm);
    m_fire_pos.set(0, 0, 0);
    m_fire_bone_xform.transform_tiny(m_fire_pos);
    m_fire_dir.set(0, 0, 1);
    m_fire_bone_xform.transform_dir(m_fire_dir);

    if (IsActive() == false)
        return;

    /* Update barrel. */
    Fmatrix inv;
    inv.invert(xfm);
    Fvector dep;

    Fvector dir;
    if (m_desired_idx == eDesiredPos)
    {
        Fvector vec = Fmatrix().mul_43(xfm, atm()->dcast_PKinematics()->LL_GetTransform(m_rotate_x_bone)).c;
        dir.sub(m_desired_pos, vec).normalize_safe();
    }
    else
    {
        dir.set(m_desired_dir);
    }

    {
        if (m_desired_idx == eDesiredPos || m_desired_idx == eDesiredDir)
        {
            dep.setHP(m_desired_ang.x, m_desired_ang.y);
        }
        else
        {
            inv.transform_dir(dep, dir);
        }
        m_i_bind_y_xform.transform_dir(dep);
        dep.normalize();
        m_tgt_x_rot = angle_normalize_signed(m_bind_x_rot - dep.getP());
        clamp(m_tgt_x_rot, -m_lim_x_rot.y, -m_lim_x_rot.x);
    }
    {
        if (m_desired_idx == eDesiredPos || m_desired_idx == eDesiredDir)
        {
            dep.setHP(m_desired_ang.x, m_desired_ang.y);
        }
        else
        {
            inv.transform_dir(dep, dir);
        }
        m_i_bind_y_xform.transform_dir(dep);
        dep.normalize();
        m_tgt_y_rot = angle_normalize_signed(m_bind_y_rot - dep.getH());
        ClampRotationHorz(m_tgt_y_rot, m_cur_y_rot, -m_lim_y_rot.y, -m_lim_y_rot.x);
    }

    m_cur_x_rot = angle_inertion_var(m_cur_x_rot, m_tgt_x_rot, m_rotate_x_speed, m_rotate_x_speed, PI, Device.fTimeDelta);
    m_cur_y_rot = angle_inertion_var(m_cur_y_rot, m_tgt_y_rot, m_rotate_y_speed, m_rotate_y_speed, PI, Device.fTimeDelta);
}

void CWeaponMountedGun::FireStart()
{
    CShootingObject::FireStart();
}

void CWeaponMountedGun::FireEnd()
{
    CShootingObject::FireEnd();
    StopFlameParticles();
}

void CWeaponMountedGun::UpdateFire()
{
    fShotTimeCounter -= Device.fTimeDelta;

    CShootingObject::UpdateFlameParticles();
    CShootingObject::UpdateLight();

    if (!IsWorking())
    {
        clamp(fShotTimeCounter, 0.0f, flt_max);
        return;
    }

    while (fShotTimeCounter <= 0)
    {
        OnShot();
        fShotTimeCounter += fOneShotTime;
    }
}

void CWeaponMountedGun::OnShot()
{
    CGameObject *owner = (Gunner()) ? Gunner() : object();
    FireBullet(m_fire_pos, m_fire_dir, GetFireDispersion(true), m_DefaultCartridge, owner->ID(), object()->ID(), true, m_iShotNum);
    m_iShotNum++;

    if (m_bLightShotEnabled)
    {
        CShootingObject::Light_Start();
    }
    StartShotParticles();
    StartFlameParticles();
    StartSmokeParticles(m_fire_pos, zero_vel);

    m_sounds.PlaySound("sndShoot", m_fire_pos, owner, false);
}

void CWeaponMountedGun::ClampRotationHorz(float &tgt_val, const float &cur_val, const float &lim_min, const float &lim_max)
{
    /* Rotating limit must be lesser than 180 in both direction. */
    if (abs(lim_min) < PI || abs(lim_max) < PI)
    {
        /* Target is outside of rotating limit. Clamp to the closest limit. */
        if (tgt_val < lim_min || tgt_val > lim_max)
        {
            if (angle_difference(tgt_val, lim_min) < angle_difference(tgt_val, lim_max))
                tgt_val = lim_min;
            else
                tgt_val = lim_max;
        }
        /* If the rotation to reach tgt_val from cur_val crosses 180 degrees in the back, make it swings around 0. */
        if (abs(tgt_val - cur_val) >= PI)
        {
            tgt_val = 0.0F;
        }
    }
}

void CWeaponMountedGun::Action(u16 id, u32 flags)
{
    switch (id)
    {
    case eActivate:
        if (flags == 1 && m_bActive != true)
        {
            m_bActive = true;
            BoneCallbacks(true);
        }
        if (flags != 1 && m_bActive == true)
        {
            m_bActive = false;
            BoneCallbacks(false);
        }
        break;
    case eFire:
        if (flags == 1 && GetState() == eStateIdle)
        {
            SwitchState(eStateFire);
        }
        if (flags != 1 && GetState() == eStateFire)
        {
            SwitchState(eStateIdle);
        }
        break;
    default:
        break;
    }
}

void CWeaponMountedGun::SetParam(int id, Fvector val)
{
    switch (id)
    {
    case eDesiredPos:
        m_desired_idx = eDesiredPos;
        m_desired_pos.set(val);
        break;
    case eDesiredDir:
        m_desired_idx = eDesiredDir;
        m_desired_dir.set(val).normalize_safe();
        break;
    case eDesiredAng:
        m_desired_idx = eDesiredAng;
        m_desired_ang.set(val);
    default:
        break;
    }
}

void CWeaponMountedGun::SwitchState(u16 state)
{
    switch (state)
    {
    case eStateIdle:
        switch2_Idle();
        break;
    case eStateFire:
        switch2_Fire();
        break;
    default:
        break;
    }
}

void CWeaponMountedGun::switch2_Idle()
{
    m_state_index = eStateIdle;
    m_state_delay = 0;
    m_iShotNum = 0;
    FireEnd();
    m_sounds.StopSound("sndReload");
    m_sounds.StopSound("sndUnload");
}

void CWeaponMountedGun::switch2_Fire()
{
#if 0
    if (m_magazine.size() == 0)
    {
        Fmatrix xfm = Fmatrix().mul_43(XFORM(), Visual()->dcast_PKinematics()->LL_GetTransform(m_rotate_x_bone));
        m_sounds.PlaySound("sndEmpty", xfm.c, Owner(), false);
        return;
    }
#endif

    m_state_index = eStateFire;
    m_state_delay = 0;
    if (IsWorking())
    {
        return;
    }
    m_iShotNum = 0;
    FireStart();
    m_sounds.StopSound("sndReload");
    m_sounds.StopSound("sndUnload");
}

float CWeaponMountedGun::GetBaseDispersion(float cartridge_k)
{
    return fireDispersionBase * cur_silencer_koef.fire_dispersion * cartridge_k;
}

float CWeaponMountedGun::GetFireDispersion(bool with_cartridge, bool for_crosshair)
{
    if (with_cartridge == false)
    {
        return GetFireDispersion(1.0F, for_crosshair);
    }
    float ammo_dispersion = (m_magazine.empty()) ? m_DefaultCartridge.param_s.kDisp : m_magazine.back().param_s.kDisp;
    return GetFireDispersion(ammo_dispersion, for_crosshair);
}

float CWeaponMountedGun::GetFireDispersion(float cartridge_k, bool for_crosshair)
{
    float fire_disp = GetBaseDispersion(cartridge_k);
#if 0
    if (OwnerActor())
    {
        fire_disp += (OwnerActor()->GetWeaponAccuracyStm() * fireDispersionOwnerScale);
    }
    else if (Owner())
    {
        fire_disp += (Owner()->cast_stalker()->GetWeaponAccuracy() * fireDispersionOwnerScale);
    }
#endif
    return fire_disp;
}