#include "pch.h"
#include "ingame_server.h"
#include"packet_manager.h"
#include"database/db.h"
#include"room/room.h"
#include"object/moveobj_manager.h"
#include"map/map_manager.h"
#include"define.h"
using namespace std;
InGameServer::InGameServer()
{
	m_PacketManager = std::make_unique<PacketManager>();
	m_PacketManager->Init();
	m_PacketManager->RegisterRecvFunction(SC_PACKET_MOVE, ProcessSignIn);
}

InGameServer::~InGameServer()
{
}


bool InGameServer::OnAccept(EXP_OVER* exp_over)
{
	m_PacketManager->ProcessAccept(IOCP_GLOBAL::g_hiocp, m_s_socket, exp_over);
	return true;
}

bool InGameServer::OnRecv(int c_id, EXP_OVER* exp_over, DWORD num_bytes)
{
	m_PacketManager->ProcessRecv(c_id, exp_over, num_bytes);
	return true;
}

void InGameServer::OnEvent(int c_id,EXP_OVER* exp_over)
{
	if (false == m_room_manager->IsRoomInGame(exp_over->room_id))
	{
		if (exp_over->_comp_op == COMP_OP::OP_FRESH_ROOM)
		{
			m_room_manager->GetRoom(exp_over->room_id)->ResetRoom();
		}
		delete exp_over;
		return;
	}
	switch (exp_over->_comp_op)
	{
	case COMP_OP::OP_NPC_SPAWN: {
		m_PacketManager->SpawnEnemy(exp_over->room_id);
		
		break;
	}
	case COMP_OP::OP_NPC_MOVE: {
		m_PacketManager->DoEnemyMove(exp_over->room_id, c_id);

		break;
	}
	case COMP_OP::OP_COUNT_TIME: {
		m_PacketManager->CountTime(exp_over->room_id);

		break;
	}
	case COMP_OP::OP_NPC_ATTACK: {
		m_PacketManager->DoEnemyAttack(c_id,exp_over->target_id,exp_over->room_id);

		break;
	}
	case COMP_OP::OP_NPC_TIMER_SPAWN: {
		m_PacketManager->SpawnEnemyByTime(c_id, exp_over->room_id);
	
		break;
	}
	case COMP_OP::OP_BASE_ATTACK: {
		m_PacketManager->BaseAttackByTime(exp_over->room_id, c_id);

		break;
	}
	case COMP_OP::OP_HEAL: {
		m_PacketManager->ActivateHealEvent(exp_over->room_id, c_id);
		
		break;
	}
	case COMP_OP::OP_LOGIN: {
		db_login_result* result = reinterpret_cast<db_login_result*>(exp_over->_net_buf);
		Player*pl=MoveObjManager::GetInst()->GetPlayer(result->obj_id);
		if (result->result == LOGINFAIL_TYPE::OK)
		{
			lock_guard<std::mutex>(pl->state_lock);
			if (pl->GetState() == STATE::ST_ACCEPT)
				pl->SetState(STATE::ST_LOGIN);
		}
		else
		{
			m_PacketManager->SendLoginFailPacket(result->obj_id, static_cast<int>(result->result));
			break;
		}
		strcpy_s(pl->GetName(), MAX_NAME_SIZE, result->user_id);
		strcpy_s(pl->GetPassword(), MAX_NAME_SIZE, result->user_password);
		m_PacketManager->SendSignInOK(pl->GetID());
		break;
	}
	case COMP_OP::OP_SIGNUP: {
		db_login_result* result = reinterpret_cast<db_login_result*>(exp_over->_net_buf);
		Player* pl = MoveObjManager::GetInst()->GetPlayer(result->obj_id);
		if (result->result == LOGINFAIL_TYPE::SIGN_UP_OK)
		{
			m_PacketManager->SendSignUpOK(pl->GetID());
		}
		else
		{
			m_PacketManager->SendLoginFailPacket(result->obj_id, 6);
		}
		break;
	}
	}
	delete exp_over;
}

void InGameServer::Disconnect(int c_id)
{
	m_PacketManager->Disconnect(c_id);
}

void InGameServer::DoTimer(HANDLE hiocp)
{
	m_PacketManager->ProcessTimer(hiocp);
}



void InGameServer::CreateTimer()
{
	m_worker_threads.emplace_back(&InGameServer::DoTimer,this, IOCP_GLOBAL::g_hiocp);
	
}



void InGameServer::Run()
{
	

	StartServer();
	CreateTimer();
	m_PacketManager->CreateDBThread();
	JoinThread();
	m_PacketManager->JoinDBThread();
}

void InGameServer::End()
{
	m_PacketManager->End();
	
	CloseHandle(IOCP_GLOBAL::g_hiocp);
	closesocket(m_s_socket);

}

void InGameServer::ProcessSignIn(int c_id, unsigned char* p)
{
	cs_packet_sign_in* packet = reinterpret_cast<cs_packet_sign_in*>(p);
	if (MoveObjManager::GetInst()->CheckLoginUser(packet->name) == false)
	{
		m_PacketManager->SendLoginFailPacket(c_id, static_cast<int>(LOGINFAIL_TYPE::AREADY_SIGHN_IN));
		return;
	}
	db_task dt;
	dt.dt = DB_TASK_TYPE::SIGN_IN;
	dt.obj_id = c_id;
	strcpy_s(dt.user_id, packet->name);
	strcpy_s(dt.user_password, packet->password);

	m_db_manager->m_db_queue.push(move(dt));
}

void InGameServer::ProcessSignUp(int c_id, unsigned char* p)
{
	cs_packet_sign_up* packet = reinterpret_cast<cs_packet_sign_up*>(p);
	db_task dt;
	dt.dt = DB_TASK_TYPE::SIGN_UP;
	dt.obj_id = c_id;
	strcpy_s(dt.user_id, packet->name);
	strcpy_s(dt.user_password, packet->password);
	m_db_queue.push(move(dt));
}

void InGameServer::ProcessAttack(int c_id, unsigned char* p)
{
	cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(p);
	Player* player = MoveObjManager::GetInst()->GetPlayer(c_id);
	if (player->GetRoomID() == -1)return;

	Room* room = m_room_manager->GetRoom(player->GetRoomID());
	Vector3 forward_vec{ packet->f_x,packet->f_y,packet->f_z };

	for (int pl : room->GetObjList())
	{
		if (false == MoveObjManager::GetInst()->IsPlayer(pl))continue;
		if (pl == c_id)continue;
		SendAttackPacket(pl, c_id, forward_vec);
	}
}

void InGameServer::ProcessMove(int c_id, unsigned char* p)
{
	cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p);
	Player* cl = MoveObjManager::GetInst()->GetPlayer(c_id);
	Vector3 pos{ packet->x,packet->y,packet->z };
	cl->state_lock.lock();
	if (cl->GetState() != STATE::ST_INGAME)
	{
		cl->state_lock.unlock();
		return;
	}
	else cl->state_lock.unlock();
	Room* room = m_room_manager->GetRoom(cl->GetRoomID());

	cl->m_last_move_time = packet->move_time;

	cl->SetPos(pos);
	if (isnan(cl->GetPosX()) || isnan(cl->GetPosY()) || isnan(cl->GetPosZ()))return;
	//여기서 힐존 검사하기
	cl->m_hp_lock.lock();
	if (cl->GetHP() < cl->GetMaxHP()) {
		cl->m_hp_lock.unlock();
		if (true == m_map_manager->CheckInRange(cl->GetPos(), OBJ_TYPE::OT_HEAL_ZONE) && false == cl->GetIsHeal())
		{
			cout << "힐존검사는 오케";
			cl->SetIsHeal(true);
			g_timer_queue.push(SetTimerEvent(c_id, c_id, cl->GetRoomID(), EVENT_TYPE::EVENT_HEAL, HEAL_TIME));
		}
	}
	else
		cl->m_hp_lock.unlock();
	for (auto other_pl : room->GetObjList())
	{
		if (false == MoveObjManager::GetInst()->IsPlayer(other_pl))
			continue;
		SendMovePacket(other_pl, c_id);
	}
}

void InGameServer::ProcessMatching(int c_id, unsigned char* p)
{
	cs_packet_matching* packet = reinterpret_cast<cs_packet_matching*>(p);
	Player* pl = MoveObjManager::GetInst()->GetPlayer(c_id);
	pl->SetMatchUserSize(packet->user_num);

	auto result = m_room_manager->SearchMatchingRoom(packet->user_num, pl);
	switch (result)
	{
	case NONE:
	{

	}
		break;
	case COMPLETE:
	{
		int r_id = pl->GetRoomID();
		Room* room = m_room_manager->GetRoom(r_id);
		room->RouteToAll(m_PacketManager->SendMatchingOK());
		room->CompleteMatching();
		for (int i = NPC_ID_START; i <= NPC_ID_END; ++i)
		{
			if (room->GetCurrentEnemySize() == room->GetMaxEnemy())
				break;

			Enemy*e = MoveObjManager::GetInst()->GetEnemy(i);

			if (false == e->in_use)
			{
				e->in_use = true;
				room->InsertEnemy(e);
			}

		}
		if (room->GetCurrentEnemySize() < room->GetMaxEnemy())
			cout << "할당할 수 있는 적 객체 수가 모자랍니다" << endl;
	}
		break;
	case NO_ROOM:
	{
		if (-1 == m_room_manager->CreateMatchingRoom(packet->user_num, pl))
		{
			std::cout << "빈방이 없습니다!" << std::endl;
		}
	}
		break;
	
	}
	
}

void InGameServer::ProcessHit(int c_id, unsigned char* p)
{
	cs_packet_hit* packet = reinterpret_cast<cs_packet_hit*>(p);
	MoveObj* victim = MoveObjManager::GetInst()->GetMoveObj(packet->victim_id);
	MoveObj* attacker = MoveObjManager::GetInst()->GetMoveObj(packet->attacker_id);
	if (victim->GetRoomID() == -1 || attacker->GetRoomID() == -1)return;
	Room* room = m_room_manager->GetRoom(attacker->GetRoomID());
	float hp;
	victim->m_hp_lock.lock();
	victim->SetHP(victim->GetHP() - attacker->GetDamge());
	hp = victim->GetHP();
	victim->m_hp_lock.unlock();
	for (int obj_id : room->GetObjList())
	{
		if (false == MoveObjManager::GetInst()->IsPlayer(obj_id))continue;
		//if (victim->GetID() == obj_id||attacker->GetID()==obj_id)continue;
		SendStatusChange(obj_id, victim->GetID(), victim->GetHP());
	}

	if (hp <= 0.0f)
	{
		victim->SetIsActive(false);
		for (int obj_id : room->GetObjList())
		{
			if (false == MoveObjManager::GetInst()->IsPlayer(obj_id))continue;
			SendDead(obj_id, victim->GetID());
		}
		if (victim->GetType() == OBJ_TYPE::OT_PLAYER)
		{
			for (int obj_id : room->GetObjList())
			{
				if (false == MoveObjManager::GetInst()->IsPlayer(obj_id))continue;
				SendGameDefeat(obj_id);
			}
			EndGame(victim->GetRoomID());
		}
	}
}

void InGameServer::ProcessGameStart(int c_id, unsigned char* p)
{
	cs_packet_game_start* packet = reinterpret_cast<cs_packet_game_start*>(p);
	Player* player = MoveObjManager::GetInst()->GetPlayer(c_id);
	if (player->GetRoomID() == -1)return;
	player->SetIsReady(true);
	Room* room = m_room_manager->GetRoom(player->GetRoomID());
	if(room->CheckPlayerReady()==true)
		StartGame(room->GetRoomID());
}

void InGameServer::ProcessDamageCheat(int c_id, unsigned char* p)
{
	Player* player = MoveObjManager::GetInst()->GetPlayer(c_id);
	if (player->GetDamge() == PLAYER_DAMAGE)
		player->SetDamge(100.0f);
	else
		player->SetDamge(PLAYER_DAMAGE);
}

void InGameServer::StartGame(int room_id)
{
	Room* room = m_room_manager->GetRoom(room_id);
	room->InitializeObject();

	//주위객체 정보 보내주기는 event로 
	//플레이어에게 플레이어 보내주기
	int next_round = 1;
	for (auto pl : room->GetObjList())
	{
		auto& obj_packet = m_PacketManager->SendObjInfo(pl);//자기자신
		room->RouteToAll(obj_packet);
	}
	auto& base_packet = m_PacketManager->SendBaseStatus(room_id, room->GetBaseHp());
	room->RouteToAll(base_packet);
		
	auto& wave_packet= m_PacketManager->SendWaveInfo(next_round, 
		room->GetMaxUser() * next_round, room->GetMaxUser() * (next_round + 1));
	room->RouteToAll(wave_packet);

	room->SetRoundTime(CONST_VALUE::ROUND_TIME);
	GS_GLOBAL::g_timer_queue.push(timer_event{ room->GetRoomID(), room->GetRoomID(), room->GetRoomID(),
		EVENT_TYPE::EVENT_TIME, 1000 });
}

void InGameServer::CountTime(EXP_OVER* exp_over)
{
	int room_id = exp_over->room_id;
	Room* room = m_room_manager->GetRoom(room_id);
	auto end_time = std::chrono::system_clock::now();
	std::chrono::duration<float> elapsed = room->GetRoundTime() - end_time;
	auto& time_packet = m_PacketManager->SendTime(elapsed.count());
	room->RouteToAll(move(time_packet));

	if (end_time >= room->GetRoundTime())
	{
		room->SetRoundTime(CONST_VALUE::ROUND_TIME);

		if (room->GetRound() < CONST_VALUE::ROUND_MAX)
		{
			room->SetRound(room->GetRound() + 1);
			GS_GLOBAL::g_timer_queue.push(timer_event{ room->GetRoomID(),
				room->GetRoomID(), room->GetRoomID(), EVENT_TYPE::EVENT_NPC_SPAWN, 30 });
		}

	}
	if (room->IsGameEnd() == true)
	{
		auto& win_packet = m_PacketManager->SendGameWin();
		room->RouteToAll(win_packet);
		room->GameEnd();
		GS_GLOBAL::g_timer_queue.push(timer_event{ room_id, room_id, room_id,
			EVENT_TYPE::EVENT_REFRESH_ROOM, 60000 });
	}

	GS_GLOBAL::g_timer_queue.push(timer_event{ room_id, room_id, room_id,
		EVENT_TYPE::EVENT_TIME, 1000 });
}

void InGameServer::SpawnEnemy(int room_id)
{
	Room* room = m_room_manager->GetRoom(room_id);
	int curr_round = room->GetRound();
	int sordier_num = room->GetMaxUser() * (curr_round + 1);
	int king_num = room->GetMaxUser() * curr_round;
	
	if (curr_round < CONST_VALUE::ROUND_MAX) {
		auto& wave_packet=m_PacketManager->SendWaveInfo(curr_round + 1, 
			room->GetMaxUser() * (curr_round + 1), room->GetMaxUser() * (curr_round + 2));
	}

	int interval = 0;
	for (auto& en : room->GetWaveEnemyList(sordier_num, king_num))
	{
		GS_GLOBAL::g_timer_queue.push(timer_event{en->GetID(),en->GetID(), room_id,
			EVENT_TYPE::EVENT_NPC_TIMER_SPAWN,(1500 * (++interval)) });
	}
}

void InGameServer::SpawnEnemyByTime(int enemy_id, int room_id)
{
	Room* room = m_room_manager->GetRoom(room_id);
	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);
	Vector2 spawn_point = m_map_manager->GetRandomSpawnPoint();
	enemy->SetSpawnPoint(spawn_point.x, spawn_point.z);
	
	auto& objInfoPacket = m_PacketManager->SendObjInfo(enemy);
	room->RouteToAll(objInfoPacket);

	GS_GLOBAL::g_timer_queue.push(timer_event{ enemy_id, enemy_id, room_id, 
		EVENT_TYPE::EVENT_NPC_MOVE, 30 });
}

void InGameServer::DoEnemyMove(int room_id, int enemy_id)
{
	Room* room = m_room_manager->GetRoom(room_id);
	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);
	if (false == enemy->GetIsActive())return;
	Vector3 target_pos;
	const Vector3 base_pos = m_map_manager->GetMapObjectByType(OBJ_TYPE::OT_BASE).GetGroundPos();
	if (enemy->GetTargetId() == BASE_ID)
		target_pos = CONST_VALUE::g_ground_base_pos;
	else
		target_pos = MoveObjManager::GetInst()->GetPlayer(enemy->GetTargetId())->GetPos();
	
	Vector3 target_vec{ target_pos - enemy->GetPos() };
	enemy->DoMove(target_vec);
}

bool InGameServer::CheckMoveOK(int enemy_id, int room_id)
{
	Room* room = m_room_manager->GetRoom(room_id);

	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);
	
	if (false == m_map_manager->CheckInRange(enemy->GetCollision()))
		return false;
	//이 부분도 가운데로 가도록
	if (true == m_map_manager->CheckCollision(enemy->GetCollision()))
		return false;

	return room->EnemyCollisionCheck(enemy);

}
