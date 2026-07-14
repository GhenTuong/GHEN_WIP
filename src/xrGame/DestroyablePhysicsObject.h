#pragma once

class CDestroyablePhysicsObject :
	public CPhysicObject,
	public CPHDestroyable,
	public CPHCollisionDamageReceiver,
	public CHitImmunity,
	public CDamageManager
{
protected:
	typedef CPhysicObject inherited;
private:
	float m_fHealth;
	ref_sound m_destroy_sound;
	shared_str m_destroy_particles;
public:
	CDestroyablePhysicsObject();
	virtual ~CDestroyablePhysicsObject();
	virtual CPhysicsShellHolder* PPhysicsShellHolder();
	virtual BOOL net_Spawn(CSE_Abstract* DC);
	virtual void net_Destroy();
	virtual void Hit(SHit* pHDS);
	virtual void InitServerObject(CSE_Abstract* D);
	virtual ICollisionDamageReceiver* PHCollisionDamageReceiver() { return (this); }
	virtual DLL_Pure* _construct();
	virtual CPhysicsShellHolder* cast_physics_shell_holder() { return this; }
	virtual CParticlesPlayer* cast_particles_player() { return this; }
	virtual CPHDestroyable* ph_destroyable() { return this; }
	virtual void shedule_Update(u32 dt);
	virtual bool CanRemoveObject();
	virtual void OnChangeVisual();
protected:
	void Destroy();
private:

#ifdef CPHYSICOBJECT
private:
    bool m_script_before_hit_enable;
    CScriptCallbackEx<bool> m_script_before_hit_callback;

public:
    IC float GetHealth() const { return m_fHealth; }
    IC void SetHealth(const float value) { m_fHealth = value; }
    IC bool Destroyable() { return CPHDestroyable::Destroyable(); }
    IC bool Destroyed() { return CPHDestroyable::Destroyed(); }
    IC bool CanDestroy() { return CPHDestroyable::CanDestroy(); }

    void set_script_before_hit_callback();
    void set_script_before_hit_callback(const ::luabind::functor<bool>& func);
    void set_script_before_hit_callback(const ::luabind::functor<bool>& func, const ::luabind::object& bind);
#endif
};
