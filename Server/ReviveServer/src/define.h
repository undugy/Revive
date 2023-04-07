#pragma once

#include <WS2tcpip.h>
#include <MSWSock.h>
#include<sqlext.h>


#include<concurrent_priority_queue.h>
#include"util/state.h"
#include"util/vec2.h"
#include"util/vec3.h"
#include"util/vec4.h"
#include"protocol.h"
#include"event/timer_event.h"


extern "C" {
#include "include\lua.h"
#include "include\lauxlib.h"
#include "include\lualib.h"
}

#pragma comment (lib, "lib/lua54.lib")

const int BUFSIZE = 2048;
const float ATTACK_RANGE = 5000.0f;
const float GROUND_HEIGHT = 350.0f;

class EXP_OVER
{
public:
	WSAOVERLAPPED _wsa_over;
	COMP_OP		  _comp_op;
	WSABUF		  _wsa_buf;
	unsigned char _net_buf[BUFSIZE];
	int target_id;
	int room_id;
public:
	EXP_OVER(COMP_OP comp_op, char num_bytes, void* mess) : _comp_op(comp_op),room_id(-1)
	{
		ZeroMemory(&_wsa_over, sizeof(_wsa_over));
		_wsa_buf.buf = reinterpret_cast<char*>(_net_buf);
		_wsa_buf.len = num_bytes;
		memcpy(_net_buf, mess, num_bytes);
	};
	EXP_OVER(COMP_OP comp_op) :_comp_op(comp_op), room_id(-1) {}
	EXP_OVER(): room_id(-1) { _comp_op = COMP_OP::OP_RECV; }
	~EXP_OVER() {}


};



class db_task {
public:
	db_task(int c_id,DB_TASK_TYPE type, char* id, char* pw) : dt(type), obj_id(c_id)
	{
		strcpy_s(user_id, id);
		strcpy_s(user_password, pw);
	};
	db_task() = default;
	int obj_id;
	DB_TASK_TYPE dt;
	char user_id[MAX_NAME_SIZE];
	char user_password[MAX_PASSWORD_SIZE];
};

struct db_result {
	int obj_id;
	LOGINFAIL_TYPE result;
};
struct db_login_result
	:public db_result 
{	
	char user_id[MAX_NAME_SIZE];
	char user_password[MAX_PASSWORD_SIZE];
};


struct BoxCollision2D
{
	BoxCollision2D(const Vector3& min_pos, const Vector3 max_pos)
	{
		p[0] = Vector2(min_pos.x,min_pos.z);
		p[1] = Vector2(min_pos.x, max_pos.z);
		p[2] = Vector2(max_pos.x, min_pos.z);
		p[3] = Vector2(max_pos.x, max_pos.z);
	}
	Vector2 p[4];
};

constexpr unsigned int HashCode(const char* str)
{
	return str[0] ? static_cast<unsigned int>(str[0]) + 0xEDB8832Full * HashCode(str + 1) : 8603;
}

namespace IOCP_GLOBAL
{
	extern HANDLE g_hiocp;

};

namespace GS_GLOBAL
{
	extern concurrency::concurrent_priority_queue <timer_event> g_timer_queue;
}

namespace CONST_VALUE
{
	extern const Vector3 g_ground_base_pos;
	extern const Vector3 PLAYER_SPAWN_POINT[3];
	extern const int ROUND_TIME;
	extern const int HEAL_TIME;
	extern const int ROUND_MAX;
	extern const float HEURISTICS;
	extern const int ATTACK_INTERVAL;
	extern const int MOVE_INTERVAL;

}
#include"lua/functions/lua_functions.h"