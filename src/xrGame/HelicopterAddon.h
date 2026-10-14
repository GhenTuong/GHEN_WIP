#pragma once
#include "stdafx.h"

#ifdef HELICOPTER_NEW
#include "GameObject.h"
#include "ShootingObject.h"
#include "HudSound.h"

#include "script_export_space.h"
#include "script_attachment_manager.h"

class CArmament;
class CHelicopterWeapon;

class CHelicopterWeapon
{
private:
    shared_str m_name;
    CGameObject* m_object;
    script_attachment* m_attachment;

    xr_vector<CArmament> m_armaments;

public:
    CHelicopterWeapon(CGameObject* O, LPCSTR name);
    ~CHelicopterWeapon();

    LPCSTR Name() { return m_name.c_str(); }
    CGameObject* Object() { return m_object; }
    script_attachment* Attachment() { return m_attachment; }

    void net_Spawn();
    void net_Destroy();
    void Activate(bool val);
    bool IsActive() { return m_attachment != nullptr; }
    void LoadArmaments();
    void UpdateCL();
    void renderable_Render();

    ::luabind::object ArmamentList();
    CArmament* Armament(LPCSTR sec);
    void Fire(bool val);
};

class CArmament : public CShootingObject
{
protected:
    typedef CShootingObject inherited;
    virtual const Fvector& get_CurrentFirePoint() { return m_fire_pos; };
    virtual const Fmatrix& get_ParticlesXFORM() { return m_fire_xfm; };
    virtual bool IsHudModeNow() { return false; };

public:
    struct SBone
    {
        CHelicopterWeapon* wpn;
        Fmatrix xfm;

        u16 offset_bid;
        Fvector offset_pos;
        Fvector offset_hpb;
        Fmatrix offset_xfm;

        SBone();
        ~SBone();
        void Init(CHelicopterWeapon* O);
        void Load(u16& bid, Fvector& pos, Fvector& hpb);
        Fmatrix& XFORM();
    };

private:
    shared_str m_name;
    CGameObject* m_object;
    CHelicopterWeapon* m_weapon;

    Fmatrix m_fire_xfm;
    Fvector m_fire_pos;
    Fvector m_fire_dir;

    SBone m_bone_fire;
    SBone m_bone_drop;

    CGameObject* Object() { return m_object; };
    CHelicopterWeapon* Weapon() { return m_weapon; };

    CCartridge m_DefaultCartridge;

    u8 m_ammoType;
    xr_vector<shared_str> m_ammoTypes;
    int iAmmoElapsed;
    int iMagazineSize;

    bool m_unlimited_ammo;
    bool IsUnlimitedAmmo() { return m_unlimited_ammo; }

    HUD_SOUND_COLLECTION_LAYERED m_sounds;

    u16 m_gatling_bid;
    float m_gatling_inc;
    float m_gatling_ang;
    void SetGatlingBoneCallbacks(bool val);
    static void _BCL GatlingBoneCallback(CBoneInstance* B);
    void UpdateGatling();

public:
    CArmament(CGameObject* O, CHelicopterWeapon* W, LPCSTR name);
    ~CArmament();

    LPCSTR Name() { return m_name.c_str(); }
    void Load(LPCSTR section);
    void UpdateCL();
    void renderable_Render();
    void Fire(bool val);
    bool Firing() { return bWorking; }

    virtual void UpdateFire();
    virtual void OnShot();
    virtual void OnShellDrop(const Fmatrix& xfm, const Fvector& vel);

    u8 GetAmmoType() { return m_ammoType; }
    void SetAmmoType(u8 type);
    int GetAmmoMagSize() { return iMagazineSize; }
    int GetAmmoElapsed() { return iAmmoElapsed; }
    void SetAmmoElapsed(int num);
};
#endif