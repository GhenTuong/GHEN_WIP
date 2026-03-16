#pragma once

#include "ShootingObject.h"

#include "pch_script.h"
#include "level.h"
#include "script_attachment_manager.h"

#define DBG_MSG(fmt, ...) Msg("%s: " fmt, __FUNCTION__, ##__VA_ARGS__)

class CGameObject;
class CWeaponMountedGun;

class CWeaponMountedGun :
    public CShootingObject
{
public:
    CWeaponMountedGun(CGameObject* obj, LPCSTR sec, u16 bid);
    virtual ~CWeaponMountedGun();
    CGameObject* object() { return m_object; }
    script_attachment* atm() { return m_atm; }
    LPCSTR AttachName() { return m_attach_name.c_str(); }
    LPCSTR WeaponName() { return m_weapon_name.c_str(); }
    bool IsAttachName(LPCSTR sec);
    bool IsWeaponName(LPCSTR sec);

    CGameObject* Gunner() { return m_gunner; }
    void AttachGunner(CGameObject* obj);
    void DetachGunner();
    virtual void renderable_Render();

    void Load(LPCSTR section);
    void LoadWeapon(LPCSTR section);
    void UpdateCL();

    bool GetEnable() { return m_enable; }
    void SetEnable(bool flag);
    bool IsActive() { return m_bActive; }

private:
    CGameObject* m_object;
    script_attachment* m_atm;
    shared_str m_attach_name;
    shared_str m_weapon_name;
    u16 m_attach_bone;

    u16 m_rotate_x_bone;
    u16 m_rotate_y_bone;
    u16 m_fire_bone;
    Fmatrix m_i_bind_x_xform;
    Fmatrix m_i_bind_y_xform;
    Fmatrix m_fire_bone_xform;
    Fvector m_fire_pos;
    Fvector m_fire_dir;
    Fvector2 m_lim_x_rot;
    Fvector2 m_lim_y_rot;
    float m_bind_x_rot;
    float m_bind_y_rot;
    float m_cur_x_rot;
    float m_cur_y_rot;
    float m_tgt_x_rot;
    float m_tgt_y_rot;

    float m_rotate_x_speed;
    float m_rotate_y_speed;

    u16 m_desired_idx;
    Fvector m_desired_pos;
    Fvector m_desired_dir;
    Fvector m_desired_ang;

    u16 m_state_index;
    float m_state_delay;
    HUD_SOUND_COLLECTION_LAYERED m_sounds;

    CGameObject* m_gunner;

    u16 m_iShotNum;

protected:
    virtual bool IsHudModeNow() { return false; }
    virtual const Fvector& get_CurrentFirePoint();
    virtual const Fmatrix& get_ParticlesXFORM();
    static void _BCL BoneCallbackX(CBoneInstance* B);
    static void _BCL BoneCallbackY(CBoneInstance* B);
    void BoneCallbacks(bool flag);
    virtual void FireStart();
    virtual void FireEnd();
    virtual void UpdateFire();
    virtual void OnShot();
    void UpdateBarrelDir();
    void ClampRotationHorz(float& tgt_val, const float& cur_val, const float& lim_min, const float& lim_max);


    IC u16 GetState() const { return m_state_index; }
    IC float GetStateDelay() const { return m_state_delay; }
    virtual void SwitchState(u16 state);
    virtual void switch2_Idle();
    virtual void switch2_Fire();

    bool m_enable;

    bool m_bActive;
    bool m_bAutoFire;
    float m_weapon_h;

    u8 m_ammoType;
    xr_vector<shared_str> m_ammoTypes;
    xr_vector<CCartridge> m_magazine;
    CCartridge m_DefaultCartridge;

public:
    enum
    {
        eActivate = 0,
        eFire,
        eDesiredPos,
        eDesiredDir,
        eDesiredAng,
    };
    virtual void Action(u16 id, u32 flags);
    virtual void SetParam(int id, Fvector val);

    enum
    {
        eStateIdle = 0,
        eStateFire,
    };


    float GetBaseDispersion(float cartridge_k);
    float GetFireDispersion(bool with_cartridge, bool for_crosshair = false);
    virtual float GetFireDispersion(float cartridge_k, bool for_crosshair = false);
};
