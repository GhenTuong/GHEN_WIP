#include "stdafx.h"
#include "WeaponMount.h"

CWeaponMount::CWeaponMount(CGameObject* obj)
{
    m_object = obj;
    m_stm = smart_cast<CWeaponStatMgun*>(m_object);
    m_car = smart_cast<CCar*>(m_object);

    m_section._set(nullptr);
    m_attachment = nullptr;
    m_pPhysicsShell = nullptr;

    m_bone_fire.Init(this);
    m_bone_drop.Init(this);
    m_bone_camera_def.Init(this);
    m_bone_camera_aim.Init(this);
    m_bone_camera_anm.Init(this);
    m_bone_gunner.Init(this);
    m_bone_weapon.Init(this);

    for (int idx = 0;idx < eHandSize;idx++)
    {
        m_bone_hand[idx].Init(this);
    }

    m_fire_xfm.identity();
    m_fire_pos.set(0, 0, 0);
    m_fire_dir.set(0, 0, 1);

    m_rotate_x_bone = BI_NONE;
    m_rotate_y_bone = BI_NONE;
    m_rotate_x_speed = 0;
    m_rotate_y_speed = 0;
    m_lim_x_rot.set(0, 0);
    m_lim_y_rot.set(0, 0);
    m_tgt_x_rot = 0;
    m_tgt_y_rot = 0;
    m_cur_x_rot = 0;
    m_cur_y_rot = 0;
    m_bind_x_rot = 0;
    m_bind_y_rot = 0;

    m_desire_dir.set(0, 0, 0);
    m_desire_ang.set(0, 0, 0);

    m_hand_visual_bone = eHandBindX;
    m_hand_visual_name._set(nullptr);

    m_bullet_bones.clear();
    m_bullet_count = 0;
}

CWeaponMount::~CWeaponMount()
{
    m_bullet_bones.clear();
    m_object->remove_attachment(m_attachment);
    m_object = nullptr;
}

void CWeaponMount::Load(LPCSTR section)
{
    u16 bid;
    Fvector pos;
    Fvector hpb;
    const Fvector vec_zero = Fvector().set(0, 0, 0);

    m_section._set(section);
    m_object->remove_attachment(m_attachment);
    m_attachment = nullptr;

    if (m_section.size() == 0)
        return;

    IKinematics* K = m_object->Visual()->dcast_PKinematics();
    CInifile* ini = K->LL_UserData();
    R_ASSERT2(ini->section_exist(section), FormatString("[%s] %s", m_object->cNameSect_str(), section).c_str());

    u16 bone_bid = K->LL_BoneID(READ_IF_EXISTS(ini, r_string, section, "attach_bone", nullptr));
    R_ASSERT2(bone_bid != BI_NONE, FormatString("[%s] %s", m_object->cNameSect_str(), section).c_str());
    LPCSTR visual = READ_IF_EXISTS(ini, r_string, section, "visual", nullptr);
    R_ASSERT2(visual, FormatString("[%s] %s", m_object->cNameSect_str(), section).c_str());

    m_attachment = xr_new<script_attachment>(section, visual);
    R_ASSERT2(m_attachment, FormatString("[%s] %s", m_object->cNameSect_str(), section).c_str());
    m_attachment->SetType(script_attachment_type::eSA_World);
    m_attachment->SetParent(m_object);
    m_attachment->SetParentBone(bone_bid);
    m_attachment->SetPosition(READ_IF_EXISTS(ini, r_fvector3, section, "attach_position", vec_zero));
    m_attachment->SetRotation(READ_IF_EXISTS(ini, r_fvector3, section, "attach_rotation", vec_zero));

    /* Controls */
    K = m_attachment->dcast_PKinematics();
    R_ASSERT2(K, FormatString("[%s] %s", m_object->cNameSect_str(), section).c_str());
    m_rotate_x_bone = ini->line_exist(section, "rotate_x_bone") ? K->LL_BoneID(ini->r_string(section, "rotate_x_bone")) : BI_NONE;
    m_rotate_y_bone = ini->line_exist(section, "rotate_y_bone") ? K->LL_BoneID(ini->r_string(section, "rotate_y_bone")) : BI_NONE;
    m_rotate_x_speed = deg2rad(READ_IF_EXISTS(ini, r_float, section, "rotate_x_speed", 20));
    m_rotate_y_speed = deg2rad(READ_IF_EXISTS(ini, r_float, section, "rotate_y_speed", 20));

    xr_vector<Fmatrix> matrices;
    K->LL_GetBindTransform(matrices);
    if (m_rotate_x_bone != BI_NONE)
    {
        CBoneData& B = K->LL_GetData(m_rotate_x_bone);
        m_lim_x_rot.set(B.IK_data.limits[0].limit.x, B.IK_data.limits[0].limit.y);
        m_bind_x_rot = matrices[m_rotate_x_bone].k.getP();
        m_bind_x.set(matrices[m_rotate_x_bone].c);
    }
    if (m_rotate_y_bone != BI_NONE)
    {
        CBoneData& B = K->LL_GetData(m_rotate_y_bone);
        m_lim_y_rot.set(B.IK_data.limits[1].limit.x, B.IK_data.limits[1].limit.y);
        m_bind_y_rot = matrices[m_rotate_y_bone].k.getH();
        m_bind_y.set(matrices[m_rotate_y_bone].c);
    }

    {
        bid = ini->line_exist(section, "fire_bone") ? K->LL_BoneID(ini->r_string(section, "fire_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "fire_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "fire_rotation", vec_zero);
        m_bone_fire.Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(section, "drop_bone") ? K->LL_BoneID(ini->r_string(section, "drop_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "drop_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "drop_rotation", vec_zero);
        m_bone_drop.Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(section, "camera_def_bone") ? K->LL_BoneID(ini->r_string(section, "camera_def_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "camera_def_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "camera_def_rotation", vec_zero);
        m_bone_camera_def.Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(section, "camera_aim_bone") ? K->LL_BoneID(ini->r_string(section, "camera_aim_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "camera_aim_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "camera_aim_rotation", vec_zero);
        m_bone_camera_aim.Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(section, "camera_anm_bone") ? K->LL_BoneID(ini->r_string(section, "camera_anm_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "camera_anm_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "camera_anm_rotation", vec_zero);
        m_bone_camera_anm.Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(section, "gunner_bone") ? K->LL_BoneID(ini->r_string(section, "gunner_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "gunner_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "gunner_rotation", vec_zero);
        m_bone_gunner.Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(section, "weapon_bone") ? K->LL_BoneID(ini->r_string(section, "weapon_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "weapon_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "weapon_rotation", vec_zero);
        m_bone_weapon.Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(section, "hand_bind_x_bone") ? K->LL_BoneID(ini->r_string(section, "hand_bind_x_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "hand_bind_x_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "hand_bind_x_rotation", vec_zero);
        m_bone_hand[eHandBindX].Load(bid, pos, hpb);
    }
    {
        bid = ini->line_exist(section, "hand_bind_y_bone") ? K->LL_BoneID(ini->r_string(section, "hand_bind_y_bone")) : BI_NONE;
        pos = READ_IF_EXISTS(ini, r_fvector3, section, "hand_bind_y_position", vec_zero);
        hpb = READ_IF_EXISTS(ini, r_fvector3, section, "hand_bind_y_rotation", vec_zero);
        m_bone_hand[eHandBindY].Load(bid, pos, hpb);
    }

    /* Animations */
    m_animations[eAnimIdle] = READ_IF_EXISTS(ini, r_string, section, "anm_idle", nullptr);
    m_animations[eAnimFire] = READ_IF_EXISTS(ini, r_string, section, "anm_fire", nullptr);
    m_animations[eAnimDeploy] = READ_IF_EXISTS(ini, r_string, section, "anm_deploy", nullptr);
    m_animations[eAnimReload] = READ_IF_EXISTS(ini, r_string, section, "anm_reload", nullptr);
    m_animations[eAnimReloadEmpty] = READ_IF_EXISTS(ini, r_string, section, "anm_reload_empty", nullptr);

    m_hand_animations[eAnimIdle] = READ_IF_EXISTS(ini, r_string, section, "hand_anm_idle", nullptr);
    m_hand_animations[eAnimFire] = READ_IF_EXISTS(ini, r_string, section, "hand_anm_fire", nullptr);
    m_hand_animations[eAnimDeploy] = READ_IF_EXISTS(ini, r_string, section, "hand_anm_deploy", nullptr);
    m_hand_animations[eAnimReload] = READ_IF_EXISTS(ini, r_string, section, "hand_anm_reload", nullptr);
    m_hand_animations[eAnimReloadEmpty] = READ_IF_EXISTS(ini, r_string, section, "hand_anm_reload_empty", nullptr);

    int n = ini->line_count(section);
    for (int k = 0; k < n; k++)
    {
        LPCSTR key = nullptr;
        LPCSTR val = nullptr;
        ini->r_line(section, k, &key, &val);
        if (key && strlen(key) > strlen(m_sound_prefix) && strstr(key, m_sound_prefix))
        {
            R_ASSERT2(ini->section_exist(val), FormatString("[%s] %s %s", m_object->cNameSect_str(), section, key).c_str());
            m_sounds.LoadSound(ini, section, key, key, false, SOUND_TYPE_WEAPON_RECHARGING);
            Msg("%s: %s = %s", __FUNCTION__, key, val);
        }
    }

    m_bullet_bones.clear();
    for (int i = 0; i < 10; i++)
    {
        string128 key;
        string128 bone_name;
        xr_sprintf(key, "bullet_bones");
        if (i)
        {
            xr_sprintf(key, "bullet_bones%d", i);
        }
        LPCSTR str = READ_IF_EXISTS(ini, r_string, section, key, nullptr);
        if (str == nullptr)
            continue;
        for (int k = 0, n = _GetItemCount(str); k < n; k++)
        {
            _GetItem(str, k, bone_name);
            u16 bid = K->LL_BoneID(bone_name);
            if (bid != BI_NONE)
            {
                m_bullet_bones.push_back(bid);
            }
            else
            {
                break;
            }
        }
    }
}

void CWeaponMount::UpdateBarrelDir()
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();

    m_fire_xfm.set(m_bone_fire.XFORM());
    m_fire_pos.set(0, 0, 0);
    m_fire_xfm.transform_tiny(m_fire_pos);
    m_fire_dir.set(0, 0, 1);
    m_fire_xfm.transform_dir(m_fire_dir);

    if (fis_zero(m_desire_dir.square_magnitude()))
    {
        dep.setHP(m_desire_ang.x, m_desire_ang.y);
    }
    else
    {
        m_i_xform.invert(XFORM()).transform_dir(dep, m_desire_dir);
    }
    dep.normalize();

    m_tgt_x_rot = angle_normalize_signed(m_bind_x_rot - dep.getP());
    clamp(m_tgt_x_rot, -m_lim_x_rot.y, -m_lim_x_rot.x);
    m_tgt_y_rot = angle_normalize_signed(m_bind_y_rot - dep.getH());
    ClampRotationHorz(m_tgt_y_rot, m_cur_y_rot, -m_lim_y_rot.y, -m_lim_y_rot.x);

    if (Stm())
    {
        switch (Stm()->GetState())
        {
        case CWeaponStatMgun::eStateIdle:
        case CWeaponStatMgun::eStateFire:
            m_cur_x_rot = angle_inertion_var(m_cur_x_rot, m_tgt_x_rot, m_rotate_x_speed, m_rotate_x_speed, PI, Device.fTimeDelta);
            m_cur_y_rot = angle_inertion_var(m_cur_y_rot, m_tgt_y_rot, m_rotate_y_speed, m_rotate_y_speed, PI, Device.fTimeDelta);
            break;
        case CWeaponStatMgun::eStateDeploy:
        case CWeaponStatMgun::eStateReload:
            m_cur_x_rot = m_bind_x_rot;
            break;
        default:
            break;
        }
    }
}

void CWeaponMount::BoneCallbackX(CBoneInstance* B)
{
    CWeaponMount* P = static_cast<CWeaponMount*>(B->callback_param());
    B->mTransform.mulB_43(Fmatrix().rotateX(P->m_cur_x_rot));
}

void CWeaponMount::BoneCallbackY(CBoneInstance* B)
{
    CWeaponMount* P = static_cast<CWeaponMount*>(B->callback_param());
    B->mTransform.mulB_43(Fmatrix().rotateY(P->m_cur_y_rot));
}

void CWeaponMount::SetBoneCallbacks(bool flag)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();
    if (flag)
    {
        if (m_rotate_x_bone != BI_NONE)
        {
            CBoneInstance& B = m_attachment->dcast_PKinematics()->LL_GetBoneInstance(m_rotate_x_bone);
            B.set_callback(bctCustom, BoneCallbackX, this);
        }
        if (m_rotate_y_bone != BI_NONE)
        {
            CBoneInstance& B = m_attachment->dcast_PKinematics()->LL_GetBoneInstance(m_rotate_y_bone);
            B.set_callback(bctCustom, BoneCallbackY, this);
        }
    }
    else
    {
        if (m_rotate_x_bone != BI_NONE)
        {
            CBoneInstance& B = m_attachment->dcast_PKinematics()->LL_GetBoneInstance(m_rotate_x_bone);
#if 0
            B.set_callback(bctPhysics, PPhysicsShell()->GetBonesCallback(), PPhysicsShell()->get_Element(m_rotate_x_bone));
#else
            B.reset_callback();
#endif
            B.mTransform.mulB_43(Fmatrix().rotateY(-m_cur_x_rot));
        }
        if (m_rotate_y_bone != BI_NONE)
        {
            CBoneInstance& B = m_attachment->dcast_PKinematics()->LL_GetBoneInstance(m_rotate_y_bone);
#if 0
            B.set_callback(bctPhysics, PPhysicsShell()->GetBonesCallback(), PPhysicsShell()->get_Element(m_rotate_y_bone));
#else
            B.reset_callback();
#endif
            B.mTransform.mulB_43(Fmatrix().rotateY(-m_cur_y_rot));
        }
    }
}

void CWeaponMount::ClampRotationHorz(float& tgt_val, const float& cur_val, const float& lim_min, const float& lim_max)
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

Fmatrix CWeaponMount::XFORM()
{
    R_ASSERT2(m_attachment, FormatString("[%s] no attachment", m_object->cNameSect_str()).c_str());
    return m_attachment->GetTransform();
}

void CWeaponMount::SetDesireDir(Fvector vec)
{
    m_desire_dir.set(vec);
    m_desire_ang.set(0, 0, 0);
}

void CWeaponMount::SetDesireAng(Fvector vec)
{
    m_desire_dir.set(0, 0, 0);
    m_desire_ang.set(vec);
}

u32 CWeaponMount::PlayAnimation(u16 idx)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED(0);
    HandPlayAnimation(idx);
    return (m_animations[idx].size()) ? m_attachment->PlayMotion(m_animations[idx].c_str(), false) : 0;
}

void CWeaponMount::PlaySound(LPCSTR alias, CGameObject* who)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();
    if (m_sounds.FindSoundItem(alias, false))
    {
        m_sounds.PlaySound(alias, WeaponXFORM().c, who, false);
    }
}

void CWeaponMount::HandCreate()
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();
    HandRemove();
    LPCSTR vis = HandVisualName();
    if (vis)
    {
        script_attachment* atm = xr_new<script_attachment>(WEAPON_MOUNT_HAND, vis);
        R_ASSERT(atm);
        atm->SetType(script_attachment_type::eSA_World);
        atm->SetParent(m_attachment);
        HandPlayAnimation(eAnimIdle);
    }
}

void CWeaponMount::HandRemove()
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();
    m_attachment->RemoveAttachment(WEAPON_MOUNT_HAND);
}

void CWeaponMount::HandShow(bool flag)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();
    script_attachment* atm = HandAttachment();
    if (atm)
    {
        float num = (flag) ? 1.0f : 0.0f;
        atm->SetScale(num, num, num);
    }
}

script_attachment* CWeaponMount::HandAttachment()
{
    RETURN_IF_WEAPON_MOUNT_DISABLED(nullptr);
    return m_attachment->GetChild(WEAPON_MOUNT_HAND);
}

LPCSTR CWeaponMount::HandVisualName()
{
    m_hand_visual_name._set(nullptr);
    if (g_player_hud == nullptr || g_player_hud->section_name().size() == 0)
        return nullptr;
    LPCSTR str = READ_IF_EXISTS(pSettings, r_string, g_player_hud->section_name().c_str(), "visual", nullptr);
    if (str == nullptr || strlen(str) == 0)
        return nullptr;
    string128 vis;
    xr_sprintf(vis, sizeof(vis), "%s.ogf", str);
    m_hand_visual_name._set(vis);
    return m_hand_visual_name.c_str();
}

void CWeaponMount::HandSwitch(u16 idx, bool force)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();
    switch (idx)
    {
    case eHandBindX:
    case eHandBindY:
        if (m_hand_visual_bone != idx || force)
        {
            m_hand_visual_bone = idx;
            script_attachment* atm = HandAttachment();
            if (atm)
            {
                SBone& B = m_bone_hand[idx];
                atm->SetParentBone(B.offset_bid);
                atm->SetPosition(B.offset_pos);
                atm->SetRotation(B.offset_hpb);
            }
        }
        break;
    default:
        break;
    }
}

u32 CWeaponMount::HandPlayAnimation(u16 idx)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED(0);
    switch (idx)
    {
    case eAnimIdle:
        HandSwitch(eHandBindX, true);
        break;
    case eAnimFire:
        HandSwitch(eHandBindX);
        break;
    case eAnimDeploy:
    case eAnimReload:
    case eAnimReloadEmpty:
        HandSwitch(eHandBindY);
        break;
    default:
        break;
    }
    script_attachment* atm = HandAttachment();
    if (atm && m_hand_animations[idx].size())
    {
        HandShow(true);
        return atm->PlayMotion(m_hand_animations[idx].c_str(), false);
    }
    else
    {
        HandShow(false);
    }
    return 0;
}

void CWeaponMount::UpdateBulletVisibility(u16 num)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();
    m_bullet_count = num;
    for (int k = 0, n = m_bullet_bones.size(); k < n; ++k)
    {
        u16 bid = m_bullet_bones.at(k);
        bool visibility = (k < m_bullet_count) ? true : false;
        if (m_attachment->GetBoneVisible(bid) != visibility)
        {
            m_attachment->SetBoneVisible(bid, visibility, false);
        }
    }
}

/*----------------------------------------------------------------------------------------------------
    SBone
----------------------------------------------------------------------------------------------------*/
CWeaponMount::SBone::SBone()
{
    wpn = nullptr;
    xfm.identity();
    offset_bid = BI_NONE;
    offset_pos.set(0, 0, 0);
    offset_hpb.set(0, 0, 0);
    offset_xfm.identity();
}

CWeaponMount::SBone::~SBone()
{
    wpn = nullptr;
}

void CWeaponMount::SBone::Init(CWeaponMount* O)
{
    wpn = O;
}

void CWeaponMount::SBone::Load(u16& bid, Fvector& pos, Fvector& hpb)
{
    offset_bid = (bid != BI_NONE) ? bid : wpn->Attachment()->dcast_PKinematics()->LL_GetBoneRoot();;
    offset_pos.set(pos);
    offset_hpb.set(hpb);
    offset_xfm.identity().setHPB(deg2rad(hpb.x), deg2rad(hpb.y), deg2rad(hpb.z)).translate_over(pos);
}

Fmatrix& CWeaponMount::SBone::XFORM()
{
    R_ASSERT2(wpn->Attachment(), FormatString("[%s] no attachment", wpn->Section()).c_str());
    xfm.mul_43(wpn->Attachment()->dcast_PKinematics()->LL_GetTransform(offset_bid), offset_xfm);
    xfm.mulA_43(wpn->XFORM());
    return xfm;
}
