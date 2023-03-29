#pragma once
#include<unordered_map>
#include<mutex>
#include<chrono>

class Object;
class Player;
class Enemy;
class Room
{
public:
	Room(int room_id) ;
	Room() = default;
	~Room();



	void Init(int user_num);
	void EnterRoom(Player* player);
	void ResetRoom();

	int GetCurrentUserSize() { return m_player_vec.size(); }
	int GetCurrentEnemySize() { return m_enemy_vec.size(); }
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
	void RouteToOther(const Pk& packet,int sender_id);
	template<typename Pk>
	void RouteToAll(const Pk& packet);
	template<typename Pk>
	void SendTo(const Pk& packet,int c_id);

	bool CompleteMatching();
	bool CheckPlayerReady();
	void InitializeObject();
	bool IsGameEnd();
	void GameEnd();
	bool EnemyCollisionCheck(Enemy* enemy);
	const vector<Enemy*>& GetWaveEnemyList(int sordier_num, int king_num);
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


