
#include"lua_functions.h"
#include"object/moveobj_manager.h"
#include"define.h"

using namespace std;
int API_get_x(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	float x= MoveObjManager::GetInst()->GetMoveObj(user_id)->GetPosX();
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	float y = MoveObjManager::GetInst()->GetMoveObj(user_id)->GetPosY();
	lua_pushnumber(L, y);
	return 1;
}

int API_get_z(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	float z = MoveObjManager::GetInst()->GetMoveObj(user_id)->GetPosZ();
	lua_pushnumber(L, z);
	return 1;
}

int API_test_lua(lua_State* L)
{
	float now_x = (float)lua_tonumber(L, -2);
	float now_y = (float)lua_tonumber(L, -1);
	lua_pop(L, 3);
	return 0;
}

int API_attack(lua_State* L)
{
	int target_id = (int)lua_tointeger(L, -1);
	int npc_id = (int)lua_tointeger(L, -2);
	lua_pop(L, 3);
	Enemy* en = MoveObjManager::GetInst()->GetEnemy(npc_id);
	en->SetTargetId(target_id);
	auto attack_t = chrono::system_clock::now();
	chrono::milliseconds mil = chrono::duration_cast<chrono::milliseconds>(en->GetAttackTime() - attack_t);
	
	if (attack_t <=en->GetAttackTime())
		GS_GLOBAL::g_timer_queue.push(timer_event{ npc_id, target_id, en->GetRoomID(),
		EVENT_TYPE::EVENT_NPC_ATTACK, static_cast<int>(mil.count()) });
	else
		GS_GLOBAL::g_timer_queue.push(timer_event{ npc_id, target_id, en->GetRoomID(),
		EVENT_TYPE::EVENT_NPC_ATTACK, CONST_VALUE::ATTACK_INTERVAL });

	return 0;
}

int API_move(lua_State* L)
{
	int target_id = (int)lua_tointeger(L, -1);
	int npc_id = (int)lua_tointeger(L, -2);
	lua_pop(L, 3);
	Enemy* en = MoveObjManager::GetInst()->GetEnemy(npc_id);
	en->SetTargetId(target_id);
	GS_GLOBAL::g_timer_queue.push(timer_event{ en->GetID(), en->GetID(), en->GetRoomID(),
		EVENT_TYPE::EVENT_NPC_MOVE,CONST_VALUE::MOVE_INTERVAL });
	return 0;
}
