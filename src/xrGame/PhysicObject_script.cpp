#include "pch_script.h"
#include "PhysicObject.h"
#include "PHCollisionDamageReceiver.h"
#include "PHDestroyable.h"
#include "hit_immunity.h"
#include "damage_manager.h"
#include "DestroyablePhysicsObject.h"


using namespace luabind;

#pragma optimize("s",on)
void CPhysicObject::script_register(lua_State* L)
{
	module(L)
	[
		class_<CDestroyablePhysicsObject, CGameObject>("CDestroyablePhysicsObject")
#ifdef CPHYSICOBJECT
        .def("GetHealth", &CDestroyablePhysicsObject::GetHealth)
        .def("SetHealth", &CDestroyablePhysicsObject::SetHealth)
        .def("Destroyable", &CDestroyablePhysicsObject::Destroyable)
        .def("Destroyed", &CDestroyablePhysicsObject::Destroyed)
        .def("CanDestroy", &CDestroyablePhysicsObject::CanDestroy)
        .def("set_script_before_hit_callback", (void (CDestroyablePhysicsObject::*)())(&CDestroyablePhysicsObject::set_script_before_hit_callback))
        .def("set_script_before_hit_callback", (void (CDestroyablePhysicsObject::*)(const ::luabind::functor<bool>&))(&CDestroyablePhysicsObject::set_script_before_hit_callback))
        .def("set_script_before_hit_callback", (void (CDestroyablePhysicsObject::*)(const ::luabind::functor<bool>&, const ::luabind::object&))(&CDestroyablePhysicsObject::set_script_before_hit_callback))
#endif
		.def(constructor<>()),
		class_<CPhysicObject, CGameObject>("CPhysicObject")
		.def(constructor<>())
		.def("run_anim_forward", &CPhysicObject::run_anim_forward)
		.def("run_anim_back", &CPhysicObject::run_anim_back)
		.def("stop_anim", &CPhysicObject::stop_anim)
		.def("anim_time_get", &CPhysicObject::anim_time_get)
		.def("anim_time_set", &CPhysicObject::anim_time_set)
		.def("play_bones_sound", &CPhysicObject::play_bones_sound)
		.def("stop_bones_sound", &CPhysicObject::stop_bones_sound)
		.def("set_door_ignore_dynamics", &CPhysicObject::set_door_ignore_dynamics)
		.def("unset_door_ignore_dynamics", &CPhysicObject::unset_door_ignore_dynamics)

#ifdef CPHYSICOBJECT
        .def("get_is_ai_obstacle", &CPhysicObject::get_is_ai_obstacle)
        .def("set_is_ai_obstacle", &CPhysicObject::set_is_ai_obstacle)
#endif
	];
}
