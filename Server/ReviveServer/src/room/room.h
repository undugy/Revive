#pragma once
#include<unordered_map>
#include<mutex>
#include<chrono>
#include<unordered_set>
#include<vector>
#include"util/state.h"
#include"protocol.h"

#include"object/move_objects/player.h"
#include"object/move_objects/enemy.h"

class Room
{
public:
	Room(int room_id) ;
	Room() = default;
	~Room();



	void Init(int user_num);
	void EnterRoom(Player* player);
	void ResetRoom();

	size_t GetCurrentUserSize() { return m_player_vec.size(); }
	size_t GetCurrentEnemySize() { return m_enemy_vec.size(); }
	int GetRoomID() { return room_id; }
	int GetMaxUser() { return max_user; }
	int GetMaxEnemy() { return max_npc; }
	int GetRound() { return curr_round; }
	float GetBaseHp() { return m_base_hp; }
	std::vector<Player*>& GetObjList() { return m_player_vec; }
	std::chrono::system_clock::time_point GetRoundTime() { return m_round_time; }
	ROOM_STATE GetState() { return m_room_state; }


	void SetBaseHp(float val) { m_base_hp=val; }
	void SetState(ROOM_STATE val) { m_room_state = val; }
	void SetRoundTime(int seconds);
	void SetRound(int val) { curr_round = val; }
	void InsertEnemy(Enemy* enemy);

	template<typename Pk>
	void RouteToOther(Pk& packet, int sender_id)
	{
		for (Player* pl : m_player_vec)
		{
			if (pl->GetID() == sender_id)continue;
			pl->DoSend(sizeof(packet), &packet);
		}
	}
	template<typename Pk>
	void RouteToAll(Pk& packet)
	{
		for (Player* pl : m_player_vec)
		{
			pl->DoSend(sizeof(packet), &packet);
		}
	}
	template<typename Pk>
	void SendTo(Pk& packet, int c_id)
	{
		for (Player* pl : m_player_vec)
		{
			if (pl->GetID() != c_id)continue;
			pl->DoSend(sizeof(packet), &packet);
		}
	}

	bool CompleteMatching();
	bool CheckPlayerReady();
	void InitializeObject();
	bool IsGameEnd();
	void GameEnd();
	bool EnemyCollisionCheck(Enemy* enemy);
	const std::unordered_set<Enemy*> GetWaveEnemyList(int sordier_num, int king_num);
	std::mutex m_base_hp_lock;
	std::mutex m_state_lock;
	
private:

	int room_id;
	int max_user;
	ROOM_STATE m_room_state;
	int max_npc;
	int curr_round;
	std::vector<Player*>m_player_vec;
	std::vector<Enemy*>m_enemy_vec;
	std::chrono::system_clock::time_point	m_round_time;
	float m_base_hp = BASE_HP;
};


