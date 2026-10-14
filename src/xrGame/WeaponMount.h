#pragma once
#include "stdafx.h"

#include "script_attachment_manager.h"
#include "holder_custom.h"
#include "GameObject.h"
#include "WeaponStatMgun.h"
#include "Car.h"

#define RETURN_IF_WEAPON_MOUNT_DISABLED(...) do { if (!Enabled()) return __VA_ARGS__; } while (0)
#define WEAPON_MOUNT_HAND "weapon_mount_hand"

class CWeaponStatMgun;
class CCar;

class CWeaponMount
{
public:
    enum ECWeaponMountAnimation
    {
        eAnimIdle = 0,
        eAnimFire,
        eAnimDeploy,
        eAnimReload,
        eAnimReloadEmpty,
        eAnimSize,
    };

    enum ECWeaponMountHandBind
    {
        eHandBindX = 0,
        eHandBindY,
        eHandSize,
    };

private:
    shared_str m_section;
    CGameObject* m_object;
    CWeaponStatMgun* m_stm;
    CCar* m_car;

    script_attachment* m_attachment;
    CPhysicsShell* m_pPhysicsShell;

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
    void ClampRotationHorz(float& tgt_val, const float& cur_val, const float& lim_min, const float& lim_max);
    Fmatrix XFORM();

    struct SBone
    {
        CWeaponMount* wpn;
        Fmatrix xfm;

        u16 offset_bid;
        Fvector offset_pos;
        Fvector offset_hpb;
        Fmatrix offset_xfm;

        SBone();
        ~SBone();
        void Init(CWeaponMount* O);
        void Load(u16& bid, Fvector& pos, Fvector& hpb);
        Fmatrix& XFORM();
    };

    SBone m_bone_fire;
    SBone m_bone_drop;
    SBone m_bone_camera_def;
    SBone m_bone_camera_aim;
    SBone m_bone_camera_anm;
    SBone m_bone_gunner;
    SBone m_bone_weapon;
    SBone m_bone_hand[eHandSize];

    shared_str m_animations[eAnimSize];

    u16 m_hand_visual_bone;
    shared_str m_hand_visual_name;
    shared_str m_hand_animations[eAnimSize];
    script_attachment* HandAttachment();
    LPCSTR HandVisualName();
    void HandSwitch(u16 idx, bool force = false);
    u32 HandPlayAnimation(u16 idx);

    const LPCSTR m_sound_prefix = "sound@";
    HUD_SOUND_COLLECTION_LAYERED m_sounds;

    xr_vector<u16> m_bullet_bones;
    u16 m_bullet_count;

public:
    CWeaponMount(CGameObject* obj);
    ~CWeaponMount();
    bool Enabled() { return m_attachment != nullptr; }
    LPCSTR Section() { return m_section.c_str(); }
    CGameObject* Object() { return m_object; }
    CWeaponStatMgun* Stm() { return m_stm; }
    CCar* Car() { return m_car; }
    script_attachment* Attachment() { return m_attachment; }

public:
    void SetBoneCallbacks(bool flag);
    void SetDesireDir(Fvector vec);
    void SetDesireAng(Fvector vec);

    Fmatrix& get_ParticlesXFORM() { return m_fire_xfm; }
    Fvector& get_CurrentFirePoint() { return m_fire_pos; }
    Fvector GetFirePos() { return m_fire_pos; }
    Fvector GetFireDir() { return m_fire_dir; }
    Fmatrix& DropXFORM() { return m_bone_drop.XFORM(); }
    Fmatrix& CameraDefXFORM() { return m_bone_camera_def.XFORM(); }
    Fmatrix& CameraAimXFORM() { return m_bone_camera_aim.XFORM(); }
    Fmatrix& CameraAnmXFORM() { return m_bone_camera_anm.XFORM(); }
    Fmatrix& GunnerXFORM() { return m_bone_gunner.XFORM(); }
    Fmatrix& WeaponXFORM() { return m_bone_weapon.XFORM(); }

    void Load(LPCSTR section);
    void UpdateBarrelDir();
    LPCSTR Animation(u16 idx) { return (idx < eAnimSize) ? m_animations[eAnimSize].c_str() : nullptr; }
    u32 PlayAnimation(u16 idx);

    LPCSTR SoundPrefix() { return m_sound_prefix; }
    void PlaySound(LPCSTR alias, CGameObject* who);

    void HandCreate();
    void HandRemove();
    void HandShow(bool flag);

    void UpdateBulletVisibility(u16 num);
};