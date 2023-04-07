#pragma once
extern "C" {
#include "include\lua.h"
#include "include\lauxlib.h"
#include "include\lualib.h"
}
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

	 void Init(const char* script_name);
	 void LuaInitializeEnemy(Enemy* volatile enemy);
	 void RegisterAPI();
	 void LuaErrorDisplay(int err_num);
	 lua_State* GetLua() { return m_L; }
	 void CallStateMachine(Enemy* enemy);
private:
	 lua_State* m_L;
	 std::mutex lua_lock;
};