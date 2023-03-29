#pragma once
#include"define.h"
#include<concurrent_queue.h>
#include<thread>
#include<memory>
#include"map/map_manager.h"
#include"room/room_manager.h"
#include<functional>
class MoveObjManager;
class DB;


class PacketManager
{
public:
	PacketManager();
	~PacketManager() = default;
	
	void Init();
	void ProcessPacket(int c_id, unsigned char* p);
	void ProcessAccept(HANDLE ,SOCKET&,EXP_OVER*);
	void ProcessRecv(int, EXP_OVER*, DWORD);
	


	void CountTime(int room_id);

	void SpawnEnemy(int room_id);
	void SpawnEnemyByTime(int enemy_id,int room_id);
	void DoEnemyMove(int room_id, int enemy_id);
	void DoEnemyAttack(int enemy_id, int target_id, int room_id);
	void BaseAttackByTime(int room_id,int enemy_id);
	void ActivateHealEvent(int room_id, int player_id);

	void SendMovePacket(int c_id, int mover);
	void SendNPCAttackPacket(int c_id,int obj_id, int target_id);
	void SendLoginFailPacket(int c_id, int reason);
	void SendLoginFailPacket(SOCKET& c_socket, int reason);
	void SendSignInOK(int c_id);
	void SendSignUpOK(int c_id);
	sc_packet_matching SendMatchingOK();
	void SendPutObjPacket(int c_id, int obj_id, OBJ_TYPE obj_type);
	sc_packet_obj_info SendObjInfo(MoveObj* obj);
	sc_packet_time SendTime(float round_time);
	void SendTestPacket(int c_id, int obj_id, float x, float y, float z);
	void SendAttackPacket(int c_id, int attacker,const Vector3&forward_vec);
	sc_packet_base_status SendBaseStatus(int room_id, float base_hp);
	void SendStatusChange(int c_id, int obj_id, float hp);
	sc_packet_win SendGameWin();
	void SendGameDefeat(int c_id);
	void SendDead(int c_id,int obj_id);
	sc_packet_wave_info SendWaveInfo(int curr_round, int king_num, int sordier_num);
	



	void End();
	void Disconnect(int c_id);
	bool IsRoomInGame(int room_id);
	void EndGame(int room_id);



	void CreateDBThread();
	void DBThread();
	void ProcessDBTask(db_task& dt);
	void JoinDBThread();

	void ProcessTimer(HANDLE hiocp);
	void ProcessEvent(HANDLE hiocp, timer_event& ev);

	static timer_event SetTimerEvent(int obj_id, int target_id, EVENT_TYPE ev, int seconds);
	static timer_event SetTimerEvent(int obj_id, int target_id,int room_id, EVENT_TYPE ev, int seconds);
	static concurrency::concurrent_priority_queue <timer_event> g_timer_queue;
	
	template<typename T>
	void RegisterRecvFunction(char key, void(T::* recv_func)(int,unsigned char*))
	{
		m_recv_func_map[key] = recv_func;
	}
private:
	
	
	
	std::unordered_map<char, std::function<void(int, unsigned char*)> >m_recv_func_map;
	
	
	
	std::thread db_thread;

	void CallStateMachine(int enemy_id, int room_id,const Vector3& base_pos);
	bool CheckMoveOK(int enemy_id,int room_id);


	void StartGame(int room_id);
};

