#pragma once
#include "stdafx.h"

#include "script_attachment_manager.h"
#include "holder_custom.h"
#include "PhysicsShellHolder.h"

#define RETURN_IF_WEAPON_MOUNT_DISABLED(...) do { if (!Enabled()) return __VA_ARGS__; } while (0)

class CWeaponMount;

class CWeaponMount
{
private:
    shared_str m_section;
    CGameObject* m_object;

    script_attachment* m_attachment;
    CPhysicsShell* m_pPhysicsShell;

    u16 m_fire_bid;
    Fmatrix m_fire_xfm;
    Fvector m_fire_pos;
    Fvector m_fire_dir;

    u16 m_rotate_x_bone;
    u16 m_rotate_y_bone;
    Fmatrix m_i_xform;
    Fvector m_bind_x;
    Fvector m_bind_y;
    float m_rotate_x_speed;
    float m_rotate_y_speed;
    Fvector2 m_lim_x_rot;
    Fvector2 m_lim_y_rot;

    Fvector dep;
    float m_tgt_x_rot;
    float m_tgt_y_rot;
    float m_cur_x_rot;
    float m_cur_y_rot;
    float m_bind_x_rot;
    float m_bind_y_rot;

    Fvector m_desire_dir;
    Fvector m_desire_ang;

    static void _BCL BoneCallbackX(CBoneInstance* B);
    static void _BCL BoneCallbackY(CBoneInstance* B);
    void SetBoneCallbacks(bool value);
    void ClampRotationHorz(float& tgt_val, const float& cur_val, const float& lim_min, const float& lim_max);
    virtual Fmatrix XFORM();

    u16 m_actor_bid;
    Fmatrix m_actor_off;
    Fmatrix m_actor_xfm;

public:

    u16 m_camera_bone_def;
    u16 m_camera_bone_aim;

    enum ECWeaponMountAnimation
    {
        eAnimIdle = 0,
        eAnimFire,
        eAnimDeploy,
        eAnimReload,
        eAnimReloadEmpty,
        eAnimSize,
    };

private:
    shared_str m_animations[eAnimSize];
    xr_vector<u16> m_bullet_bones;
    u16 m_bullet_count;

public:
    CWeaponMount(CGameObject* obj);
    ~CWeaponMount();
    bool Enabled() { return m_attachment != nullptr; }
    LPCSTR Section() { return m_section.c_str(); }
    CGameObject* Object() { return m_object; }
    void Load(LPCSTR section);
    void UpdateBarrelDir();

    Fmatrix& ActorXFORM();
    const Fvector& get_CurrentFirePoint() { return m_fire_pos; }
    const Fmatrix& get_ParticlesXFORM() { return m_fire_xfm; }
    Fvector GetFirePos() { return m_fire_pos; }
    Fvector GetFireDir() { return m_fire_dir; }

    LPCSTR Animation(u16 idx) { return (idx < eAnimSize) ? m_animations[eAnimSize].c_str() : nullptr; }
    u32 PlayAnimation(u16 idx);

    void UpdateBulletVisibility(u16 num);
};