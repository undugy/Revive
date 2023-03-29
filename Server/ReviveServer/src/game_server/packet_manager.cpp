#include<map>
#include<set>
#include<bitset>
#include "pch.h"
#include "packet_manager.h"
#include"database/db.h"
#include"object/moveobj_manager.h"
#include"room/room.h"
#include"object/move_objects/enemy.h"
#include"lua/functions/lua_functions.h"
#include"util/Astar.h"
#include"util/collision/collisioner.h"
#include"util/collision/collision_checker.h"
concurrency::concurrent_priority_queue<timer_event> PacketManager::g_timer_queue = concurrency::concurrent_priority_queue<timer_event>();

//#include"map_loader.h"

using namespace std;
PacketManager::PacketManager()
{
	MoveObjManager::GetInst();


	
}

void PacketManager::Init()
{
	
	MoveObjManager::GetInst()->InitPlayer();
	MoveObjManager::GetInst()->InitNPC();
	m_room_manager->InitRoom();
	m_map_manager->LoadMap("src/map/map.txt");
	m_login_db->Init();
	m_account_db->Init();
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
	
	
	switch (packet_type) {
	case CS_PACKET_SIGN_IN: {
		ProcessSignIn(c_id, p);
		break;
	}
	case CS_PACKET_SIGN_UP: {
		ProcessSignUp(c_id, p);
		break;
	}
	case CS_PACKET_MOVE: {
		ProcessMove(c_id, p);		
		break;
	}
	case CS_PACKET_ATTACK: {
		ProcessAttack(c_id, p);
		break;
	}
	case CS_PACKET_MATCHING: {
		ProcessMatching(c_id, p);
		break;
	}
	case CS_PACKET_HIT: {
		ProcessHit(c_id, p);
		break;
	}
	case CS_PACKET_GAME_START: {
		ProcessGameStart(c_id, p);
		break;
		
	}
	case CS_PACKET_DAMAGE_CHEAT: {
		ProcessDamageCheat(c_id, p);
		break;
		
	}
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

	if (num_bytes == 0) {
		Disconnect(c_id);
	}
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




void PacketManager::DoEnemyMove(int room_id, int enemy_id)//Room*, move packet
{
	Room* room = m_room_manager->GetRoom(room_id);
	
	Enemy*  enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);
	if (false == enemy->GetIsActive())return;
	Vector3 target_pos;
	const Vector3 base_pos = m_map_manager->GetMapObjectByType(OBJ_TYPE::OT_BASE).GetGroundPos();
	if (enemy->GetTargetId() == BASE_ID)//-1기지 아이디
	{
		target_pos = base_pos;
	}
	else
	{
		target_pos = MoveObjManager::GetInst()->GetPlayer(enemy->GetTargetId())->GetPos();

	}
	Vector3 target_vec = Vector3{ target_pos - enemy->GetPos() };
	enemy->DoMove(target_vec);	
	if (false == CheckMoveOK(enemy_id, room_id))
	{
		
		enemy->SetToPrevPos();
		vector<Vector3>move_ways;
		move_ways.reserve(10);
		Vector3 target_right_vec = target_vec.Cross(Vector3(0.0f, 1.0f, 0.0f));
		move_ways.push_back(target_vec * -1);
		move_ways.push_back(target_right_vec);
		move_ways.push_back((target_right_vec * -1));
		Vector3 target_diagonal_vec = target_right_vec + target_vec;
		Vector3 target_diagonal_vec2 = (target_right_vec * -1) + target_vec;
		move_ways.push_back(target_diagonal_vec);
		move_ways.push_back(target_diagonal_vec2);
		move_ways.push_back(target_diagonal_vec * -1);
		move_ways.push_back(target_diagonal_vec2 * -1);

		map<float, Vector3>nearlist;
		for (Vector3& move_vec : move_ways)
		{
			enemy->DoMove(move_vec);
			if (true == CheckMoveOK(enemy_id, room_id))
				nearlist.try_emplace(enemy->GetPos().Dist2d(target_pos)-((enemy->GetPos().Dist2d(enemy->GetPrevTestPos()) + 50.0f)), move_vec);
			enemy->SetToPrevPos();
		}
		if (nearlist.size() != 0)
		{
			enemy->SetPrevTestPos(enemy->GetPrevPos());
			enemy->DoMove(nearlist.begin()->second);

			
		}
	}
					
	

	for (auto pl : room->GetObjList())
	{
		if (false == MoveObjManager::GetInst()->IsPlayer(pl))continue;
		SendMovePacket(pl, enemy_id);
	}
	CallStateMachine(enemy_id, room_id, base_pos);
}

void PacketManager::CountTime(int room_id)
{
	Room* room = m_room_manager->GetRoom(room_id);
	auto end_time = std::chrono::system_clock::now();
	std::chrono::duration<float> elapsed = room->GetRoundTime() - end_time;

	for (int pl : room->GetObjList())
	{
		if (false == MoveObjManager::GetInst()->IsPlayer(pl))
			break;
		SendTime(pl, elapsed.count());
	}


	if (end_time >= room->GetRoundTime())
	{
		room->SetRoundTime(ROUND_TIME);
		
		if (room->GetRound() < 3)
		{
			
			room->SetRound(room->GetRound() + 1);
			g_timer_queue.push(timer_event{ room->GetRoomID(),
				room->GetRoomID(), room->GetRoomID(), EVENT_TYPE::EVENT_NPC_SPAWN, 30 });
		}
		
	}


	if (room->GetRound() == 3)
	{
		Enemy* enemy = nullptr;
		int live_count{ 0 };
		for (int obj_id : room->GetObjList())
		{
			if (true == MoveObjManager::GetInst()->IsPlayer(obj_id))continue;
			enemy = MoveObjManager::GetInst()->GetEnemy(obj_id);
			if (enemy->GetIsActive() == true)live_count++;
		}
		if (live_count == 0)
		{
			for (auto p_id : room->GetObjList())
			{
				if (false == MoveObjManager::GetInst()->IsPlayer(p_id))continue;
				SendGameWin(p_id);
			}
			EndGame(room_id);
			return;
		}
	}
	g_timer_queue.push(SetTimerEvent(room_id, room_id, room_id,
		EVENT_TYPE::EVENT_TIME, 1000));
}

void PacketManager::DoEnemyAttack(int enemy_id, int target_id, int room_id)
{
	//초당두발
	Room* room = m_room_manager->GetRoom(room_id);
	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);

	if (false == enemy->GetIsActive())return;

	if (target_id == BASE_ID)
	{
		Vector3 base_pos = m_map_manager->GetMapObjectByType(OBJ_TYPE::OT_BASE).GetPos();
		int base_attack_t = 1000;
		if (enemy->GetType() == OBJ_TYPE::OT_NPC_SKULL) {
			float dist = enemy->GetPos().Dist(base_pos);
			base_attack_t = (dist / 1500.0f) * 1000;
		}
		g_timer_queue.push(SetTimerEvent(enemy_id, enemy_id, room_id, EVENT_TYPE::EVENT_BASE_ATTACK,
			base_attack_t));
	}
	for (int pl : room->GetObjList())
	{
		if (false==MoveObjManager::GetInst()->IsPlayer(pl))continue;
		
			SendNPCAttackPacket(pl, enemy_id, target_id);
		
	}

	auto& attack_time = enemy->GetAttackTime();
	attack_time = chrono::system_clock::now() + 1s;
	const Vector3 base_pos = m_map_manager->GetMapObjectByType(OBJ_TYPE::OT_BASE).GetGroundPos();
	CallStateMachine(enemy_id, room_id, base_pos);
}

void PacketManager::BaseAttackByTime(int room_id, int enemy_id)
{
	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);
	Room* room = m_room_manager->GetRoom(room_id);
	room->m_base_hp_lock.lock();
	float base_hp = room->GetBaseHp() - enemy->GetDamge();
	room->SetBaseHp(base_hp);
	room->m_base_hp_lock.unlock();
	for (int pl : room->GetObjList())
	{
		if (false == MoveObjManager::GetInst()->IsPlayer(pl))continue;
		SendBaseStatus(pl, room_id);
	}
	if (base_hp <= 0.0f)
	{
		for (int pl : room->GetObjList())
		{
			if (false == MoveObjManager::GetInst()->IsPlayer(pl))continue;
			SendGameDefeat(pl);
		}
		EndGame(room_id);
	}
}

void PacketManager::ActivateHealEvent(int room_id, int player_id)
{
	Player* player = MoveObjManager::GetInst()->GetPlayer(player_id);
	Room* room = m_room_manager->GetRoom(room_id);
	
	player->m_hp_lock.lock();
	player->SetHP(player->GetHP() + PLAYER_HP / 10);
	if (player->GetHP() > player->GetMaxHP())
		player->SetHP(player->GetMaxHP());
	player->m_hp_lock.unlock();
	player->SetIsHeal(false);
	for (int pl : room->GetObjList())
	{
		if (false == MoveObjManager::GetInst()->IsPlayer(pl))continue;
		SendStatusChange(pl,player->GetID(), player->GetHP());
	}
	player->m_hp_lock.lock();
	if (player->GetHP() < player->GetMaxHP()) {
		player->m_hp_lock.unlock();
		if (true == m_map_manager->CheckInRange(player->GetPos(), OBJ_TYPE::OT_HEAL_ZONE) && false == player->GetIsHeal())
		{
			player->SetIsHeal(true);
			g_timer_queue.push(SetTimerEvent(player_id, player_id, room->GetRoomID(), 
				EVENT_TYPE::EVENT_HEAL, HEAL_TIME));
		}
	}
	else
		player->m_hp_lock.unlock();
}



//Packet Sender로 옮기자
void PacketManager::SendMovePacket(int c_id, int mover)
{
	sc_packet_move packet;
	MoveObj* p = MoveObjManager::GetInst()->GetMoveObj(mover);
	packet.id = mover;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MOVE;

	packet.x =p->GetPosX();
	packet.y =p->GetPosY();
	packet.z =p->GetPosZ();
	packet.move_time = p->m_last_move_time;
	Player* cl = MoveObjManager::GetInst()->GetPlayer(c_id);
	cl->DoSend(sizeof(packet), &packet);
}

void PacketManager::SendNPCAttackPacket(int c_id,int obj_id, int target_id)
{
	sc_packet_npc_attack packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_NPC_ATTACK;
	packet.obj_id = obj_id;
	packet.target_id = target_id;
	Player* cl = MoveObjManager::GetInst()->GetPlayer(c_id);
	cl->DoSend(sizeof(packet), &packet);
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

void PacketManager::SendPutObjPacket(int c_id, int obj_id, OBJ_TYPE obj_type)
{
	sc_packet_put_object packet;
	packet.id = obj_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_PUT_OBJECT;
	packet.object_type = (char)obj_type;
	packet.x = MoveObjManager::GetInst()->GetMoveObj(obj_id)->GetPosX();
	packet.y = MoveObjManager::GetInst()->GetMoveObj(obj_id)->GetPosY();
	packet.z = MoveObjManager::GetInst()->GetMoveObj(obj_id)->GetPosZ();
	MoveObjManager::GetInst()->GetPlayer(c_id)->DoSend(sizeof(packet), &packet);
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

sc_packet_time PacketManager::SendTime(float round_time)
{
	sc_packet_time packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_TIME;
	packet.time = round_time;
	packet.send_time = chrono::system_clock::now().time_since_epoch().count();
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

void PacketManager::SendAttackPacket(int c_id, int attacker, const Vector3& forward_vec)
{
	sc_packet_attack packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_ATTACK;
	packet.obj_id = attacker;
	packet.f_x = forward_vec.x;
	packet.f_y = forward_vec.y;
	packet.f_z = forward_vec.z;
	MoveObjManager::GetInst()->GetPlayer(c_id)->DoSend(sizeof(packet), &packet);
}

sc_packet_base_status PacketManager::SendBaseStatus(int room_id,float base_hp)
{
	sc_packet_base_status packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_BASE_STATUS;
	packet.hp = base_hp;
	packet.room_id = room_id;
	return packet
}

void PacketManager::SendStatusChange(int c_id, int obj_id, float hp)
{
	sc_packet_status_change packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_STATUS_CHANGE;
	packet.id = obj_id;
	packet.hp = hp;
	MoveObjManager::GetInst()->GetPlayer(c_id)->DoSend(sizeof(packet), &packet);
}

sc_packet_win PacketManager::SendGameWin()
{
	sc_packet_win packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_WIN;
	return packet;
}


void PacketManager::SendGameDefeat(int c_id)
{

	sc_packet_defeat packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_DEFEAT;

	MoveObjManager::GetInst()->GetPlayer(c_id)->DoSend(sizeof(packet), &packet);
}

void PacketManager::SendDead(int c_id, int obj_id)
{
	sc_packet_dead packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_DEAD;
	packet.obj_id = obj_id;
	MoveObjManager::GetInst()->GetPlayer(c_id)->DoSend(sizeof(packet), &packet);
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



void PacketManager::End()
{
	MoveObjManager::GetInst()->DestroyObject();
	MoveObjManager::DestroyInst();

}

void PacketManager::Disconnect(int c_id)
{
	MoveObjManager::GetInst()->Disconnect(c_id);
	Player* cl = MoveObjManager::GetInst()->GetPlayer(c_id);
	
	lock_guard<mutex>state_guard(cl->state_lock);
	if (cl->GetRoomID() == -1)
		cl->SetState(STATE::ST_FREE);
	
}


void PacketManager::CallStateMachine(int enemy_id, int room_id, const Vector3& base_pos)//목표설정 함수 분할
{
	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);
	Room* room = m_room_manager->GetRoom(room_id);
	map<float, int>distance_map;

	float base_dist = sqrt(pow(abs(base_pos.x - enemy->GetPos().x), 2) + pow(abs(base_pos.z - enemy->GetPos().z), 2));
	distance_map.try_emplace(base_dist,BASE_ID);
	Player* player = NULL;
	auto check_end_time = std::chrono::system_clock::now();
	auto& check_time = enemy->GetCheckTime();
	int target_id =enemy->GetTargetId();
	if (check_time <= check_end_time) {
		for (auto pl : room->GetObjList())
		{
			if (false == MoveObjManager::GetInst()->IsPlayer(pl))continue;

			if (true == MoveObjManager::GetInst()->IsNear(pl, enemy_id))//시야범위안에 있는지 확인
			{
				player = MoveObjManager::GetInst()->GetPlayer(pl);
				if (false == m_map_manager->CheckInRange(player->GetPos(),OBJ_TYPE::OT_ACTIViTY_AREA)) continue;
				auto fail_obj = distance_map.try_emplace(MoveObjManager::GetInst()->ObjDistance(pl, enemy_id), pl);
			}
		}
		auto nealist = distance_map.begin();
		target_id = nealist->second;
		check_time = check_end_time + 1s;
	}

	
	lua_State* L = enemy->GetLua();
	enemy->lua_lock.lock();
	lua_getglobal(L, "state_machine");
	lua_pushnumber(L, target_id);
	int err = lua_pcall(L, 1, 0, 0);
	if (err)
		MoveObjManager::LuaErrorDisplay(L, err);
	enemy->lua_lock.unlock();
}





//여기는 두고 코드정리



void PacketManager::StartGame(int room_id)
{
	Room*room=m_room_manager->GetRoom(room_id);
	//맵 오브젝트 정보는 보내줄 필요없음
	//npc와 player 초기화 및 보내주기
	const Vector3 base_pos = m_map_manager->GetMapObjectByType(OBJ_TYPE::OT_BASE).GetGroundPos();
	Enemy* e = NULL;
	Player* pl = NULL;
	Vector3 pos = Vector3(0.0f, 300.0f, 0.0f);//npc 초기화용 위치 추후수정
	vector<int>obj_list{ room->GetObjList().begin(),room->GetObjList().end() };
	for (int i=0; i<obj_list.size(); ++i )
	{
		if (i<room->GetMaxUser())
		{
			pl = MoveObjManager::GetInst()->GetPlayer(obj_list[i]);
			pl->SetPos(m_map_manager->PLAYER_SPAWN_POINT[i]);
			pl->SetColorType(COLOR_TYPE(i + 1));
			continue;
		}
		e = MoveObjManager::GetInst()->GetEnemy(obj_list[i]);
		const Vector3&base_pos=m_map_manager->GetMapObjectByType(OBJ_TYPE::OT_BASE).GetGroundPos();
		if (i<room->GetMaxUser() * SORDIER_PER_USER)
		{
			
			e->InitEnemy(OBJ_TYPE::OT_NPC_SKULL, room->GetRoomID(), 
				SKULL_HP, pos, SORDIER_DAMAGE,"Skull Soldier");
			MoveObjManager::GetInst()->InitLua("src/lua/sclipt/enemy_sordier.lua",e->GetID(),base_pos);
			e->SetCollision(BoxCollision(pos, SOLDIER_LOCAL_POS, SOLDIER_EXTENT, SOLDIER_SCALE));
			e->SetPrevCollision(e->GetCollision());
			e->SetPrevTestPos(Vector3{ base_pos.x,e->GetPos().y,e->GetPos().z });
			
		}
		else
		{
			e->InitEnemy(OBJ_TYPE::OT_NPC_SKULLKING, room->GetRoomID(), 
				SKULLKING_HP, pos,KING_DAMAGE, "Skull King");
			MoveObjManager::GetInst()->InitLua("src/lua/sclipt/enemy_king.lua",e->GetID(),base_pos);
			e->SetCollision(BoxCollision(pos, KING_LOCAL_POS, KING_EXTENT, KING_SCALE));
			e->SetPrevCollision(e->GetCollision());
			e->SetPrevTestPos(Vector3{ base_pos.x,e->GetPos().y,e->GetPos().z });
		}
	}

	
	//주위객체 정보 보내주기는 event로 
	//플레이어에게 플레이어 보내주기
	int next_round = 1;
	for (auto c_id : room->GetObjList())
	{
		if (false == MoveObjManager::GetInst()->IsPlayer(c_id))
			continue;
	
		pl = MoveObjManager::GetInst()->GetPlayer(c_id);
		SendObjInfo(c_id, c_id);//자기자신
		for (auto other_id : room->GetObjList())
		{
			if (false == MoveObjManager::GetInst()->IsPlayer(other_id))
				continue;
			if (c_id == other_id)continue;
			SendObjInfo(c_id, other_id);
		}
		SendBaseStatus(c_id, room->GetRoomID());
		
		SendWaveInfo(c_id, next_round, room->GetMaxUser() * next_round, room->GetMaxUser() * (next_round + 1));
	}
	
	room->SetRoundTime(ROUND_TIME);
	g_timer_queue.push(SetTimerEvent(room->GetRoomID(), room->GetRoomID(), room->GetRoomID(),
		EVENT_TYPE::EVENT_TIME, 1000));
	
}

//DB쪽으로 옮기기



void PacketManager::CreateDBThread()
{

	db_thread=std::thread([this]() {DBThread(); });
}
void PacketManager::JoinDBThread()
{
	db_thread.join();
}

//이벤트쪽으로 옮기기
void PacketManager::ProcessTimer(HANDLE hiocp)
{
	timer_event ev;
	while (true) {
		while (true) {
			
			if (!g_timer_queue.try_pop(ev))continue;
			
			auto start_t = chrono::system_clock::now();
			if (ev.start_time <= start_t) {
				ProcessEvent(hiocp,ev);
			}
			else if (10ms >= ev.start_time - start_t)
			{
				this_thread::sleep_for(ev.start_time - start_t);
				ProcessEvent(hiocp, ev);
			}
			else {
				g_timer_queue.push(ev);
				break;
			}
		}

		this_thread::sleep_for(10ms);
	}
}
void PacketManager::ProcessEvent(HANDLE hiocp,timer_event& ev)
{
	
	EXP_OVER* ex_over = new EXP_OVER;
	
	switch (ev.ev) {
	case EVENT_TYPE::EVENT_NPC_SPAWN:
	{
		ex_over->_comp_op = COMP_OP::OP_NPC_SPAWN;
		ex_over->target_id = ev.target_id;
		ex_over->room_id = ev.room_id;
		PostQueuedCompletionStatus(hiocp, 1, ev.obj_id, &ex_over->_wsa_over);
		break;
	}
	case EVENT_TYPE::EVENT_NPC_TIMER_SPAWN:
	{
		ex_over->_comp_op = COMP_OP::OP_NPC_TIMER_SPAWN;
		ex_over->target_id = ev.target_id;
		ex_over->room_id = ev.room_id;
		PostQueuedCompletionStatus(hiocp, 1, ev.obj_id, &ex_over->_wsa_over);
		break;
	}
	case EVENT_TYPE::EVENT_NPC_MOVE: {
		ex_over->_comp_op = COMP_OP::OP_NPC_MOVE;
		ex_over->room_id = ev.room_id;
		ex_over->target_id = ev.target_id;

		PostQueuedCompletionStatus(hiocp, 1, ev.obj_id, &ex_over->_wsa_over);
		break;
	}
	case EVENT_TYPE::EVENT_NPC_ATTACK: {
		ex_over->_comp_op = COMP_OP::OP_NPC_ATTACK;
		ex_over->room_id = ev.room_id;
		ex_over->target_id = ev.target_id;

		PostQueuedCompletionStatus(hiocp, 1, ev.obj_id, &ex_over->_wsa_over);
		break;
	}
	case EVENT_TYPE::EVENT_TIME: {
		ex_over->_comp_op = COMP_OP::OP_COUNT_TIME;
		ex_over->room_id = ev.obj_id;

		PostQueuedCompletionStatus(hiocp, 1, ev.obj_id, &ex_over->_wsa_over);
		break;
	}
	case EVENT_TYPE::EVENT_REFRESH_ROOM: {
		delete ex_over;
		Room* room = m_room_manager->GetRoom(ev.room_id);
		room->ResetRoom();
		room->m_state_lock.lock();
		room->SetState(ROOM_STATE::RT_FREE);
		room->m_state_lock.unlock();
		break;
	}
	case EVENT_TYPE::EVENT_BASE_ATTACK: {
		ex_over->_comp_op = COMP_OP::OP_BASE_ATTACK;
		ex_over->room_id = ev.room_id;

		PostQueuedCompletionStatus(hiocp, 1, ev.obj_id, &ex_over->_wsa_over);
		break;
	}
	case EVENT_TYPE::EVENT_HEAL: {
		ex_over->_comp_op = COMP_OP::OP_HEAL;
		ex_over->room_id = ev.room_id;
		ex_over->target_id = ev.target_id;
		PostQueuedCompletionStatus(hiocp, 1, ev.obj_id, &ex_over->_wsa_over);
		break;
	}

	}
}
