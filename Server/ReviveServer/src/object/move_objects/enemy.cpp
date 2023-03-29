#include "pch.h"
#include "enemy.h"

using namespace std;

//기지 아이디= -1
bool Enemy::Init(OBJ_TYPE type)
{
	
	
	switch (type)
	{
	
	case OBJ_TYPE::OT_NPC_SKULL: 
	{
		m_type = type;
		m_hp = SKULL_HP;
		m_maxhp = SKULL_HP;
		m_damage = SORDIER_DAMAGE;
		m_collision = BoxCollision{ m_pos, SOLDIER_LOCAL_POS, SOLDIER_EXTENT, SOLDIER_SCALE };
		strcpy_s(m_name, "Skull Soldier");
		m_lua.Init("src/lua/sclipt/enemy_sordier.lua", this);
	}
		break;
	case OBJ_TYPE::OT_NPC_SKULLKING:
	{
		m_type = type;
		m_hp = SKULLKING_HP;
		m_maxhp = SKULLKING_HP;
		m_damage = SORDIER_DAMAGE;
		m_collision = BoxCollision{ m_pos, KING_LOCAL_POS, KING_EXTENT, KING_SCALE };
		strcpy_s(m_name, "Skull King");
		m_lua.Init("src/lua/sclipt/enemy_king.lua", this);
	}
		break;
	default:
		cout << "Enemy Type Error !" << endl;
		break;
	}
	
}

void Enemy::SetSpawnPoint(float x, float z)
{
	Vector3 pos(x, 300.0f, z);
	SetPos(pos);
	m_prev_pos = pos;
}

void Enemy::Reset()
{
	target_id = BASE_ID;
	lua_lock.lock();
	lua_close(m_L);
	lua_lock.unlock();
	m_room_id = -1;
	m_attack_time = std::chrono::system_clock::now();
	m_check_time = std::chrono::system_clock::now();
	in_use=false;
	in_game = false;
	m_is_active = false;
	ZeroMemory(m_name, MAX_NAME_SIZE + 1);
	m_prev_test_pos=Vector3{ 0.0f,0.0f,0.0f };
}

void Enemy::DoMove(const Vector3& target_pos)
{
	Vector3& nlook = m_look;
	Vector3& curr_pos = m_pos;
	m_prev_pos = m_pos;
	nlook = target_pos;
	Vector3 move_vec = nlook.Normalrize();
	Vector3 npos = curr_pos + (move_vec * MAX_SPEED);
	m_pos = npos;
	m_collision.UpdateCollision(m_pos);
}

void Enemy::DoPrevMove(const Vector3& target_pos)
{
	Vector3& nlook = m_look;
	Vector3& curr_pos = m_prev_pos;
	nlook = Vector3{ target_pos - curr_pos };
	Vector3 move_vec = nlook.Normalrize();
	Vector3 npos = curr_pos + (move_vec * MAX_SPEED);
	m_prev_pos = m_pos;
	m_pos = npos;
	m_collision.UpdateCollision(m_pos);
}


