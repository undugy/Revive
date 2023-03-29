#pragma once
#include"define.h"
#include<mutex>
class Enemy;
class LuaComponent
{
public:
	LuaComponent(){

	}
	 ~LuaComponent(){
		 if (m_L)
			 lua_close(m_L);
	 }

	 void Init(const char* script_name, Enemy* enemy);
	 void RegisterAPI();
	 void LuaErrorDisplay(int err_num);
	 lua_State* GetLua() { return m_L; }
private:
	 lua_State* m_L;
	 std::mutex lua_lock;
};