#include"LuaComponent.h"
#include"object/move_objects/enemy.h"
#include"define.h"
#include<iostream>
using namespace std;
void LuaComponent::Init(const char* script_name)
{


	lua_State*L = luaL_newstate();
	luaL_openlibs(L);
	
	
	if (luaL_loadfile(L, script_name) || lua_pcall(L, 0, 0, 0)) 
	{ cout << "Error : " << lua_tostring(L, -1); lua_pop(L, 1); }
	m_L = L;
	
}

void LuaComponent::LuaInitializeEnemy(Enemy* volatile enemy)
{
	lua_getglobal(m_L, "initializEnemy");
	lua_pushnumber(m_L, enemy->GetID());
	lua_pushnumber(m_L, enemy->GetPosX());
	lua_pushnumber(m_L, enemy->GetPosY());
	lua_pushnumber(m_L, enemy->GetPosZ());
	lua_pushnumber(m_L, enemy->GetHP());
	lua_pushnumber(m_L, enemy->GetDamge());
	lua_pushnumber(m_L, CONST_VALUE::g_ground_base_pos.x);
	lua_pushnumber(m_L, CONST_VALUE::g_ground_base_pos.y);
	lua_pushnumber(m_L, CONST_VALUE::g_ground_base_pos.z);
	lua_pushnumber(m_L, BASE_ID);
	int error = lua_pcall(m_L, 10, 0, 0);
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
	
	lua_getglobal(m_L, "state_machine");
	lua_pushnumber(m_L, enemy->GetTargetId());
	int err = lua_pcall(m_L, 1, 0, 0);
	if (err)
		LuaErrorDisplay(err);

}
