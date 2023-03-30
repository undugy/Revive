#include<map>
#include<set>
#include<bitset>
#include "pch.h"
#include "packet_manager.h"

#include"object/moveobj_manager.h"


using namespace std;



PacketManager::PacketManager()
{
}

void PacketManager::ProcessPacket(int c_id, unsigned char* p)
{
	unsigned char packet_type = p[1];
	
	if (m_recv_func_map[packet_type] == nullptr)
	{
		cout << "등록되지 않은 함수를 실행하려고 했습니다." << endl;
	}
	else
	{
		 m_recv_func_map[packet_type](c_id, p);
	}

}

void PacketManager::ProcessAccept(HANDLE hiocp ,SOCKET& s_socket,EXP_OVER*exp_over)
{
	SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_net_buf));
	int new_id = MoveObjManager::GetInst()->GetNewID();
	if (-1 == new_id) {
		std::cout << "Maxmum user overflow. Accept aborted.\n";
		SendLoginFailPacket(c_socket, static_cast<int>(LOGINFAIL_TYPE::FULL));
	}
	else {
		Player* cl = MoveObjManager::GetInst()->GetPlayer(new_id);
		cl->SetID(new_id);
		cl->Init(c_socket);
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), hiocp, new_id, 0);
		cl->DoRecv();
	}

	ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
	c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	
	*(reinterpret_cast<SOCKET*>(exp_over->_net_buf)) = c_socket;
	AcceptEx(s_socket, c_socket, exp_over->_net_buf + 8, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, NULL, &exp_over->_wsa_over);
}

void PacketManager::ProcessRecv(int c_id , EXP_OVER*exp_over, DWORD num_bytes)
{
	
	Player* cl = MoveObjManager::GetInst()->GetPlayer(c_id);
	int remain_data = num_bytes+ cl->m_prev_size;
	unsigned char* packet_start = exp_over->_net_buf;
	int packet_size = packet_start[0];
	while (packet_size <= remain_data) {
		ProcessPacket(c_id, packet_start);
		remain_data -= packet_size;
		packet_start += packet_size;
		if (remain_data > 0) packet_size = packet_start[0];
		else break;
	}

	if (0 < remain_data) {
		cl->m_prev_size = remain_data;
		memcpy(&exp_over->_net_buf, packet_start, remain_data);
	}
	if (remain_data == 0)cl->m_prev_size=0;
	cl->DoRecv();
}



//Packet Sender로 옮기자
 sc_packet_move PacketManager::SendMovePacket(MoveObj* mover)
{
	sc_packet_move packet;

	packet.id = mover->GetID();
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MOVE;

	packet.x =mover->GetPosX();
	packet.y =mover->GetPosY();
	packet.z =mover->GetPosZ();
	packet.move_time = mover->m_last_move_time;

	return packet;

}

 sc_packet_npc_attack PacketManager::SendNPCAttackPacket(int obj_id, int target_id)
{
	sc_packet_npc_attack packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_NPC_ATTACK;
	packet.obj_id = obj_id;
	packet.target_id = target_id;
	return packet;

}

void PacketManager::SendLoginFailPacket(int c_id, int reason)
{
	sc_packet_login_fail packet;
	Player* pl = MoveObjManager::GetInst()->GetPlayer(c_id);
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_FAIL;
	packet.reason = reason;
	pl->DoSend(sizeof(packet), &packet);

}

void PacketManager::SendLoginFailPacket(SOCKET& c_socket, int reason)
{
	sc_packet_login_fail packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_FAIL;
	packet.reason = reason;
	EXP_OVER* ex_over = new EXP_OVER(COMP_OP::OP_SEND, sizeof(packet), &packet);
	int ret = WSASend(c_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
}

void PacketManager::SendSignInOK(int c_id)
{
	sc_packet_sign_in_ok packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_SIGN_IN_OK;
	packet.id = c_id;
	MoveObjManager::GetInst()->GetPlayer(c_id)->DoSend(sizeof(packet), &packet);
}

void PacketManager::SendSignUpOK(int c_id)
{
	sc_packet_sign_up_ok packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_SIGN_UP_OK;
	MoveObjManager::GetInst()->GetPlayer(c_id)->DoSend(sizeof(packet), &packet);
}

sc_packet_matching PacketManager::SendMatchingOK()
{
	sc_packet_matching packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MATCHING;
	return packet;
}


 sc_packet_obj_info PacketManager::SendObjInfo(MoveObj* obj)
{
	sc_packet_obj_info packet;

	packet.id = obj->GetID();
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_OBJ_INFO;
	packet.damage = obj->GetDamge();
	packet.maxhp = obj->GetHP();
	strcpy_s(packet.name,MAX_NAME_SIZE+2 ,obj->GetName());
	packet.object_type = static_cast<char>(obj->GetType());
	packet.color_type = static_cast<char>(obj->GetColorType());
	packet.x = obj->GetPosX();
	packet.y = obj->GetPosY();
	packet.z = obj->GetPosZ();
	return packet;
}


void PacketManager::SendTestPacket(int c_id, int mover, float x, float y, float z)
{
	sc_packet_test packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_TEST;
	packet.id = mover;
	packet.x = x;
	packet.y = y;
	packet.z = z;

	MoveObjManager::GetInst()->GetPlayer(c_id)->DoSend(sizeof(packet), &packet);
}
 sc_packet_time PacketManager::SendTime(float round_time)
{
	sc_packet_time packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_TIME;
	packet.time = round_time;
	packet.send_time = chrono::system_clock::now().time_since_epoch().count();
	return packet;
}

 sc_packet_attack PacketManager::SendAttackPacket(int attacker, const Vector3& forward_vec)
{
	sc_packet_attack packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ATTACK;
	packet.obj_id = attacker;
	packet.f_x = forward_vec.x;
	packet.f_y = forward_vec.y;
	packet.f_z = forward_vec.z;
	return packet;
}

 sc_packet_base_status PacketManager::SendBaseStatus(int room_id,float base_hp)
{
	sc_packet_base_status packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_BASE_STATUS;
	packet.hp = base_hp;
	packet.room_id = room_id;
	return packet;
}

sc_packet_status_change PacketManager::SendStatusChange(int obj_id, float hp)
{
	sc_packet_status_change packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_STATUS_CHANGE;
	packet.id = obj_id;
	packet.hp = hp;
	return packet;
}

sc_packet_win PacketManager::SendGameWin()
{
	sc_packet_win packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_WIN;
	return packet;
}


sc_packet_defeat PacketManager::SendGameDefeat()
{

	sc_packet_defeat packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_DEFEAT;
	return packet;
}

 sc_packet_dead PacketManager::SendDead(int obj_id)
{
	sc_packet_dead packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_DEAD;
	packet.obj_id = obj_id;
	return packet;
}

 sc_packet_wave_info PacketManager::SendWaveInfo(int curr_round, int king_num, int sordier_num)
{
	sc_packet_wave_info packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_WAVE_INFO;
	packet.curr_round = curr_round;
	packet.king_num = king_num;
	packet.sordier_num = sordier_num;
	return packet;
}




//이벤트쪽으로 옮기기
