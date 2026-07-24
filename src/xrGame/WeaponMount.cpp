#include "stdafx.h"
#include "WeaponMount.h"

CWeaponMount::CWeaponMount(CGameObject* obj)
{
    m_object = obj;
    m_section._set(nullptr);
    m_attachment = nullptr;
    m_pPhysicsShell = nullptr;

    m_fire_bid = BI_NONE;
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

    m_camera_bone_def = BI_NONE;
    m_camera_bone_aim = BI_NONE;

    m_actor_bid = BI_NONE;
    m_actor_off.identity();

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
    m_section._set(section);
    m_object->remove_attachment(m_attachment);
    m_attachment = nullptr;

    if (m_section.size() == 0)
        return;

    IKinematics* K = m_object->Visual()->dcast_PKinematics();
    CInifile* ini = K->LL_UserData();
    R_ASSERT2(ini->section_exist(section), m_object->cNameSect_str());

    u16 bone_bid = K->LL_BoneID(READ_IF_EXISTS(ini, r_string, section, "attach_bone", nullptr));
    R_ASSERT2(bone_bid != BI_NONE, m_object->cNameSect_str());
    LPCSTR visual = READ_IF_EXISTS(ini, r_string, section, "visual", nullptr);
    R_ASSERT2(visual, m_object->cNameSect_str());

    m_attachment = xr_new<script_attachment>(section, visual);
    R_ASSERT2(m_attachment, m_object->cNameSect_str());
    m_attachment->SetType(script_attachment_type::eSA_World);
    m_attachment->SetParent(m_object);
    m_attachment->SetParentBone(bone_bid);
    m_attachment->SetPosition(READ_IF_EXISTS(ini, r_fvector3, section, "attach_position", Fvector().set(0, 0, 0)));
    m_attachment->SetRotation(READ_IF_EXISTS(ini, r_fvector3, section, "attach_rotation", Fvector().set(0, 0, 0)));

    /* Controls */
    K = m_attachment->dcast_PKinematics();
    R_ASSERT2(K, m_object->cNameSect_str());
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

    m_fire_bid = ini->line_exist(section, "actor_bone") ? K->LL_BoneID(ini->r_string(section, "fire_bone")) : BI_NONE;

    m_actor_bid = ini->line_exist(section, "actor_bone") ? K->LL_BoneID(ini->r_string(section, "actor_bone")) : BI_NONE;
    Fvector pos = READ_IF_EXISTS(ini, r_fvector3, section, "actor_position", Fvector().set(0, 0, 0));
    Fvector hpb = READ_IF_EXISTS(ini, r_fvector3, section, "actor_rotation", Fvector().set(0, 0, 0));
    m_actor_off.identity().setHPB(deg2rad(hpb.x), deg2rad(hpb.y), deg2rad(hpb.z)).translate_over(pos);

    /* Camera */
    m_camera_bone_def = ini->line_exist(section, "camera_bone_def") ? K->LL_BoneID(ini->r_string(section, "camera_bone_def")) : BI_NONE;
    m_camera_bone_aim = ini->line_exist(section, "camera_bone_aim") ? K->LL_BoneID(ini->r_string(section, "camera_bone_aim")) : BI_NONE;

    /* Animations */
    m_animations[eAnimIdle] = READ_IF_EXISTS(ini, r_string, section, "anm_idle", nullptr);
    m_animations[eAnimFire] = READ_IF_EXISTS(ini, r_string, section, "anm_fire", nullptr);
    m_animations[eAnimDeploy] = READ_IF_EXISTS(ini, r_string, section, "anm_deploy", nullptr);
    m_animations[eAnimReload] = READ_IF_EXISTS(ini, r_string, section, "anm_reload", nullptr);
    m_animations[eAnimReloadEmpty] = READ_IF_EXISTS(ini, r_string, section, "anm_reload_empty", nullptr);

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
                Msg("%s:%d m_bullet_bones.push_back(%s)", __FUNCTION__, __LINE__, bone_name);
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
    R_ASSERT2(m_attachment, m_object->cNameSect_str());
    IKinematics* K = m_attachment->dcast_PKinematics();
    {
        m_fire_xfm.mul_43(XFORM(), K->LL_GetTransform(m_fire_bid));
        m_fire_pos.set(0, 0, 0);
        m_fire_xfm.transform_tiny(m_fire_pos);
        m_fire_dir.set(0, 0, 1);
        m_fire_xfm.transform_dir(m_fire_dir);
    }
    {
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
    }
    {
        m_cur_x_rot = angle_inertion_var(m_cur_x_rot, m_tgt_x_rot, m_rotate_x_speed, m_rotate_x_speed, PI, Device.fTimeDelta);
        m_cur_y_rot = angle_inertion_var(m_cur_y_rot, m_tgt_y_rot, m_rotate_y_speed, m_rotate_y_speed, PI, Device.fTimeDelta);
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

void CWeaponMount::SetBoneCallbacks(bool value)
{
    R_ASSERT2(m_attachment, m_object->cNameSect_str());
    IKinematics* K = m_attachment->dcast_PKinematics();
    if (value)
    {
        if (m_rotate_x_bone != BI_NONE)
        {
            CBoneInstance& B = K->LL_GetBoneInstance(m_rotate_x_bone);
            B.set_callback(bctCustom, BoneCallbackX, this);
        }
        if (m_rotate_y_bone != BI_NONE)
        {
            CBoneInstance& B = K->LL_GetBoneInstance(m_rotate_y_bone);
            B.set_callback(bctCustom, BoneCallbackY, this);
        }
    }
    else
    {
#if 0
        if (m_rotate_x_bone != BI_NONE)
        {
            CBoneInstance& B = K->LL_GetBoneInstance(m_rotate_x_bone);
            B.set_callback(bctPhysics, PPhysicsShell()->GetBonesCallback(), PPhysicsShell()->get_Element(m_rotate_x_bone));
            B.mTransform.mulB_43(Fmatrix().rotateY(-m_cur_x_rot));
        }
        if (m_rotate_y_bone != BI_NONE)
        {
            CBoneInstance& B = K->LL_GetBoneInstance(m_rotate_y_bone);
            B.set_callback(bctPhysics, PPhysicsShell()->GetBonesCallback(), PPhysicsShell()->get_Element(m_rotate_y_bone));
            B.mTransform.mulB_43(Fmatrix().rotateY(-m_cur_y_rot));
        }
#endif
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
    R_ASSERT2(m_attachment, m_object->cNameSect_str());
    return m_attachment->GetTransform();
}

Fmatrix& CWeaponMount::ActorXFORM()
{
    R_ASSERT2(m_attachment, m_object->cNameSect_str());
    u16 bid = (m_actor_bid != BI_NONE) ? m_actor_bid : m_attachment->dcast_PKinematics()->LL_GetBoneRoot();
    m_actor_xfm.mul_43(m_attachment->dcast_PKinematics()->LL_GetTransform(bid), m_actor_off);
    m_actor_xfm.mulA_43(XFORM());
    return m_actor_xfm;
}

u32 CWeaponMount::PlayAnimation(u16 idx)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED(0);
    if (m_animations[idx].size())
    {
        return m_attachment->PlayMotion(m_animations[idx].c_str(), TRUE);
    }
    return 0;
}

void CWeaponMount::UpdateBulletVisibility(u16 num)
{
    RETURN_IF_WEAPON_MOUNT_DISABLED();
    if (m_bullet_bones.size() == 0)
        return;
    m_bullet_count = num;
    IKinematics* K = m_attachment->dcast_PKinematics();
    for (int k = 0, n = m_bullet_bones.size(); k < n; ++k)
    {
        u16 bid = m_bullet_bones.at(k);
        BOOL visibility = (k < m_bullet_count) ? TRUE : FALSE;
        if (K->LL_GetBoneVisible(bid) != visibility)
        {
            K->LL_SetBoneVisible(bid, visibility, FALSE);
        }
    }
}
