#pragma once

#include<thread>
#include<memory>
#include<functional>
#include"protocol.h"

class MoveObj;


class PacketManager
{
public:
	PacketManager();
	~PacketManager() = default;

	void Init();
	void ProcessPacket(int c_id, unsigned char* p);
	void ProcessAccept(HANDLE, SOCKET&, EXP_OVER*);
	void ProcessRecv(int, EXP_OVER*, DWORD);



	 sc_packet_move          SendMovePacket(MoveObj* mover);
	 sc_packet_npc_attack    SendNPCAttackPacket(int obj_id, int target_id);
	 sc_packet_matching      SendMatchingOK();
	 sc_packet_obj_info      SendObjInfo(MoveObj* obj);
	 sc_packet_time          SendTime(float round_time);
	 sc_packet_attack        SendAttackPacket(int attacker, const Vector3& forward_vec);
	 sc_packet_base_status   SendBaseStatus(int room_id, float base_hp);
	 sc_packet_status_change SendStatusChange(int obj_id, float hp);
	 sc_packet_defeat        SendGameDefeat();
	 sc_packet_win           SendGameWin();
	 sc_packet_wave_info     SendWaveInfo(int curr_round, int king_num, int sordier_num);
	 sc_packet_dead          SendDead(int obj_id);

	void SendLoginFailPacket(int c_id, int reason);
	void SendLoginFailPacket(SOCKET& c_socket, int reason);
	void SendSignInOK(int c_id);
	void SendSignUpOK(int c_id);

	void SendTestPacket(int c_id, int obj_id, float x, float y, float z);



	void RegisterRecvFunction(char key,std::function<void(int, unsigned char*)>func)
	{
		m_recv_func_map[key] = func;
	}
private:

	std::unordered_map<char, std::function<void(int, unsigned char*)> >m_recv_func_map;


};

