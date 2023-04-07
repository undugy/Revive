
#include "room.h"
#include "Wave.h"
#include"util/collision/collision_checker.h"
using namespace std;
Room::Room(int room_id):room_id(room_id),max_user(0),max_npc(0)
{
	m_round_time =chrono::system_clock::now();
	m_room_state = ROOM_STATE::RT_FREE;
	m_player_vec.reserve(10);
	m_enemy_vec.reserve(60);
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

	for (int i = 0; i < m_enemy_vec.size(); ++i)
	{
		
		if (i < SORDIER_PER_USER * max_user)
		{
			m_enemy_vec[i]->Init(OBJ_TYPE::OT_NPC_SKULL);
		}
		else
		{
			m_enemy_vec[i]->Init(OBJ_TYPE::OT_NPC_SKULLKING);
		}
	}

}

void Room::MakeWave()
{
	unordered_set<Enemy*> close_set;

	
	for (int i = 1; i <= CONST_VALUE::ROUND_MAX; ++i)
	{
		Wave wave{ i,max_user };
		for (Enemy* en : m_enemy_vec)
		{
			if (wave.GetSize() == wave.GetKingNum() + wave.GetSordierNum())break;
			if (close_set.count(en) == 1)continue;
			if (en->GetType() == OBJ_TYPE::OT_NPC_SKULL && wave.GetSize() < wave.GetSordierNum())
			{
	
				close_set.insert(en);
				wave.PushEnemy(en);
			}
			else if (en->GetType() == OBJ_TYPE::OT_NPC_SKULLKING)
			{
	
				close_set.insert(en);
				wave.PushEnemy(en);
			}
		}
		if (wave.GetSize() < wave.GetKingNum() + wave.GetSordierNum())
		{
			cout <<"방:"<<room_id<<", 라운드:"<<i<< "적절한 웨이브 생성이 되지 않았습니다." << endl;
		}
		m_rounds[i] = move(wave);
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

const Wave& Room::GetWave(int round)
{
	
	return m_rounds[round];

}




