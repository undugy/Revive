#include "pch.h"
#include "room.h"
#include"object/object.h"
#include"object/move_objects/player.h"
#include"object/move_objects/enemy.h"
#include"util/collision/collision_checker.h"
using namespace std;
Room::Room(int room_id):room_id(room_id),max_user(0),max_npc(0)
{
	m_round_time =chrono::system_clock::now();
	m_room_state = ROOM_STATE::RT_FREE;
	m_player_vec.reserve(10);
	m_enemy_vec.reserve(100);
}

Room::~Room()
{
}

void Room::Init(int user_num)
{

	
	max_user = user_num;
	max_npc = user_num * NPC_PER_USER;
	curr_round = 0;
	
}

void Room::EnterRoom(Player*player)
{
	player->SetRoomID(room_id);
	m_player_vec.push_back(player);
}

void Room::ResetRoom()
{
	max_user = 0;
	max_npc = 0;
	curr_round = 0;
	m_base_hp = BASE_HP;
	m_round_time = chrono::system_clock::now();
	
	m_player_vec.clear();
	m_enemy_vec.clear();
	
	m_state_lock.lock();
	SetState(ROOM_STATE::RT_FREE);
	m_state_lock.unlock();
	
	
}

void Room::SetRoundTime(int seconds)
{
	m_round_time = chrono::system_clock::now() + (1ms * seconds);
}

void Room::InsertEnemy(Enemy* enemy)
{
	enemy->SetRoomID(room_id);
	m_enemy_vec.push_back(enemy);
}

bool Room::CompleteMatching()
{
	for (Player* player : m_player_vec)
	{
		player->state_lock.lock();
		player->SetState(STATE::ST_INGAME);
		player->state_lock.unlock();
		//player->is_matching = false;
		player->SetIsActive(true);
	}
	return false;
}

bool Room::CheckPlayerReady()
{
	for (Player* pl : m_player_vec)
	{
		if (pl->GetIsReady() == false)
			return false;
	}
	return true;
}

void Room::InitializeObject()
{
	for (int i=0; i< m_player_vec.size(); ++i)
	{
		m_player_vec[i]->SetPos(CONST_VALUE::PLAYER_SPAWN_POINT[i]);
		m_player_vec[i]->SetColorType(COLOR_TYPE(i + 1));
	}
	for (int i = 0; i < m_player_vec.size(); ++i)
	{
		if (i < SORDIER_PER_USER * max_user)
			m_enemy_vec[i]->Init(OBJ_TYPE::OT_NPC_SKULL);
		else
			m_enemy_vec[i]->Init(OBJ_TYPE::OT_NPC_SKULLKING);
	}
	
}

bool Room::IsGameEnd()
{
	if (curr_round != CONST_VALUE::ROUND_MAX)
		return false;

	for (Enemy* en : m_enemy_vec)
	{
		if (en->GetIsActive() == true)
			return false;
	}


	return true;
}

void Room::GameEnd()
{
	m_state_lock.lock();
	SetState(ROOM_STATE::RT_RESET);
	m_state_lock.unlock();
	for (Player* pl : m_player_vec)
		pl->Reset();
	for (Enemy* en : m_enemy_vec)
		en->Reset();
	
	
}

bool Room::EnemyCollisionCheck(Enemy* enemy)
{
	for (auto& en : m_enemy_vec) {

		if (enemy == en)continue;
		if (false == en->GetIsActive())continue;
		if (true == CollisionChecker::CheckCollisions(enemy->GetCollision(),en->GetCollision()))
		{
			return false;
		}
	}
	return true;
}

const vector<Enemy*>& Room::GetWaveEnemyList(int sordier_num, int king_num)
{
	vector<Enemy*>spawn_vec;
	int vec_size = 0;
	for (Enemy* en : m_enemy_vec)
	{
		vec_size = spawn_vec.size();
		if (vec_size == sordier_num + king_num)
			break;
		if (en->GetIsActive() == true)
			continue;
		if (en->GetType() == OBJ_TYPE::OT_NPC_SKULL && vec_size < sordier_num)
		{
			en->SetIsActive(true);
			spawn_vec.push_back(en);
		}
		else if (en->GetType() == OBJ_TYPE::OT_NPC_SKULLKING)
		{
			en->SetIsActive(true);
			spawn_vec.push_back(en);
		}
	}
	if (spawn_vec.size() < sordier_num + king_num)
		cout << "적 객체가 모자랍니다" << endl;
	return spawn_vec;
	// TODO: 여기에 return 문을 삽입합니다.
}

template<typename Pk>
void Room::RouteToOther(const Pk& packet,int sender_id)
{
	for (Player* pl : m_player_vec)
	{
		if (pl->GetID() == sender_id)continue;
		pl->DoSend(sizeof(packet), &packet);
	}
}

template<typename Pk>
void Room::RouteToAll(const Pk& packet)
{
	for (Player* pl : m_player_vec)
	{
		pl->DoSend(sizeof(packet), &packet);
	}
}

template<typename Pk>
void Room::SendTo(const Pk& packet,int c_id)
{
	for (Player* pl : m_player_vec)
	{
		if (pl->GetID() != c_id)continue;
		pl->DoSend(sizeof(packet), &packet);
	}
}



