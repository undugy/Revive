#include"LuaComponent.h"
#include"object/move_objects/enemy.h"
#include"define.h"
#include<iostream>
using namespace std;
void LuaComponent::Init(const char* script_name, Enemy* volatile enemy)
{
	lua_State* L = m_L;
	lock_guard<mutex>gu(this->lua_lock);
	luaL_openlibs(L);
	//cout << "Thread:" << this_thread::get_id() << "call" << enemy->GetID() << "lua func" << endl;
	int error = luaL_loadfile(L, script_name) || lua_pcall(L, 0, 0, 0);
	if (error) { cout << "Error : " << lua_tostring(L, -1); lua_pop(L, 1); }

	lua_getglobal (L, "initializEnemy");
	lua_pushnumber(L, enemy->GetID());
	lua_pushnumber(L, enemy->GetPosX());
	lua_pushnumber(L, enemy->GetPosY());
	lua_pushnumber(L, enemy->GetPosZ());
	lua_pushnumber(L, enemy->GetHP());
	lua_pushnumber(L, enemy->GetDamge());
	lua_pushnumber(L, CONST_VALUE::g_ground_base_pos.x);
	lua_pushnumber(L, CONST_VALUE::g_ground_base_pos.y);
	lua_pushnumber(L, CONST_VALUE::g_ground_base_pos.z);
	lua_pushnumber(L, BASE_ID);
	error = lua_pcall(L, 10, 0, 0);
	if (error)
		LuaErrorDisplay(error);

	RegisterAPI();
}

void LuaComponent::RegisterAPI()
{
	lua_register(m_L, "API_get_x", API_get_x);
	lua_register(m_L, "API_get_y", API_get_y);
	lua_register(m_L, "API_get_z", API_get_z);
	lua_register(m_L, "API_move", API_move);
	lua_register(m_L, "API_attack", API_attack);
	lua_register(m_L, "API_test_lua", API_test_lua);
}

void LuaComponent::LuaErrorDisplay(int err_num)
{
	cout << "Error : " << lua_tostring(m_L, -1) << endl;
	lua_pop(m_L, 1);
}

void LuaComponent::CallStateMachine(Enemy* enemy)
{
	//lock_guard<mutex>gu(this->lua_lock);
	lua_getglobal(m_L, "state_machine");
	lua_pushnumber(m_L, enemy->GetTargetId());
	int err = lua_pcall(m_L, 1, 0, 0);
	//if (err)
	//	LuaErrorDisplay(err);

}
