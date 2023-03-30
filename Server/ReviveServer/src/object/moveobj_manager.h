#pragma once
#include"move_objects/move_object.h"
#include"move_objects/enemy.h"
#include"move_objects/player.h"
#include"protocol.h"
#include<array>


class MoveObjManager
{
private:
	static MoveObjManager* m_pInst;

public:
	static MoveObjManager* GetInst()
	{
		if (!m_pInst)
			m_pInst = new MoveObjManager;
		return m_pInst;
	}

	static void DestroyInst()
	{
		if (m_pInst)
		{
			delete m_pInst;
			m_pInst = NULL;
		}
	}
public:
	MoveObjManager() {  };
	~MoveObjManager() = default;

	Player* GetPlayer(int id) { 
		
		Player*p=((Player*)(m_moveobj_arr[id])); 
		return p;
		
	}
	Enemy* GetEnemy(int id) {
		//Enemy* e = ;
		return ((Enemy*)(m_moveobj_arr[id]));
	}
	MoveObj* GetMoveObj(int id) { return m_moveobj_arr[id]; }
	bool IsPlayer(int id) { return (id >= 0) && (id < MAX_USER); }
	bool IsNear(int a, int b);
	float ObjDistance(int a, int b);
	
	
	static void LuaErrorDisplay(lua_State* L,int err_num);
	int GetNewID();
	void Disconnect(int);
	void InitPlayer();
	void InitNPC();
	void DestroyObject();
	bool CheckLoginUser(char* user_id);
private:
	std::array <MoveObj*, MAX_USER + MAX_NPC> m_moveobj_arr;
	int m_id;
};

