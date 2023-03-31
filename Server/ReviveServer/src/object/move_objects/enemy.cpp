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
	
	return true;
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

	m_room_id = -1;
	m_attack_time = std::chrono::system_clock::now();
	m_check_time = std::chrono::system_clock::now();
	in_use=false;

	m_is_active = false;
	ZeroMemory(m_name, MAX_NAME_SIZE + 1);
	m_prev_test_pos=Vector3{ 0.0f,0.0f,0.0f };
}

void Enemy::SetAttackTime()
{
	m_attack_time = chrono::system_clock::now() + 1s;
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

const vector<Vector3> Enemy::MakeWays(Vector3& target_vec)
{
	vector<Vector3>move_ways;
	move_ways.reserve(10);
	Vector3 target_right_vec = target_vec.Cross(Vector3(0.0f, 1.0f, 0.0f));
	Vector3 target_diagonal_vec = target_right_vec + target_vec;
	Vector3 target_diagonal_vec2 = (target_right_vec * -1) + target_vec;
	move_ways.push_back(target_vec);
	move_ways.push_back(target_right_vec);
	move_ways.push_back(target_diagonal_vec);
	move_ways.push_back(target_diagonal_vec2);
	for (int i = 0; i < 4; i++)
		move_ways.push_back(move_ways[i] * -1);
	
	return move_ways;
}

bool Enemy::CheckTargetChangeTime()
{
	auto check_end_time = std::chrono::system_clock::now();
	if (m_check_time <= check_end_time)
	{
		m_check_time = check_end_time + 1s;
		return true;
	}
	return false;
}

void Enemy::CallLuaStateMachine()
{
	m_lua.CallStateMachine(this);
}


