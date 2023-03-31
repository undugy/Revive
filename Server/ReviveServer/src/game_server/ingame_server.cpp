#include "ingame_server.h"
#include "pch.h"
#include"packet_manager.h"
#include"room/room.h"
#include"object/moveobj_manager.h"
#include"database/db.h"
#include"database/DBManager.h"
#include"room/room_manager.h"
#include"map/map_manager.h"
#include"event/EventHelper.h"

using namespace std;


InGameServer::InGameServer()
{
	MoveObjManager::GetInst();
	m_PacketManager = make_unique<PacketManager>();
	m_room_manager = make_unique<RoomManager>(); 
	m_map_manager  = make_unique<MapManager>();
	m_db_manager   = make_unique<DBManager>();
	m_event_helper = make_unique<EventHelper>();

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
	if (num_bytes == 0){
		Disconnect(c_id);
	}
	m_PacketManager->ProcessRecv(c_id, exp_over, num_bytes);
	return true;
}

void InGameServer::OnEvent(int c_id,EXP_OVER* exp_over)
{
	if (exp_over->room_id != -1 &&false == m_room_manager->IsRoomInGame(exp_over->room_id))
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
		SpawnEnemy(exp_over->room_id);
		
		break;
	}
	case COMP_OP::OP_NPC_MOVE: {
		DoEnemyMove(exp_over->room_id, c_id);
		break;
	}
	case COMP_OP::OP_COUNT_TIME: {
		CountTime(exp_over);

		break;
	}
	case COMP_OP::OP_NPC_ATTACK: {
		DoEnemyAttack(c_id,exp_over->target_id,exp_over->room_id);

		break;
	}
	case COMP_OP::OP_NPC_TIMER_SPAWN: {
		SpawnEnemyByTime(c_id, exp_over->room_id);
	
		break;
	}
	case COMP_OP::OP_BASE_ATTACK: {
		BaseAttackByTime(exp_over->room_id, c_id);

		break;
	}
	case COMP_OP::OP_HEAL: {
		ActivateHealEvent(exp_over->room_id, c_id);
		
		break;
	}
	case COMP_OP::OP_LOGIN: {
		db_login_result* result = reinterpret_cast<db_login_result*>(exp_over->_net_buf);
		
		Player*pl=MoveObjManager::GetInst()->GetPlayer(result->obj_id);
		if (result->result == LOGINFAIL_TYPE::OK)
		{
			lock_guard<std::mutex>guard(pl->state_lock);
			if (pl->GetState() == STATE::ST_ACCEPT)
				pl->SetState(STATE::ST_LOGIN);

			strcpy_s(pl->GetName(), MAX_NAME_SIZE, result->user_id);
			strcpy_s(pl->GetPassword(), MAX_NAME_SIZE, result->user_password);
			m_PacketManager->SendSignInOK(pl->GetID());
		}
		else
		{
			m_PacketManager->SendLoginFailPacket(result->obj_id, static_cast<int>(result->result));
			
		}
		
		break;
	}
	case COMP_OP::OP_SIGNUP: {
		db_result* result = reinterpret_cast<db_result*>(exp_over->_net_buf);
		Player* pl = MoveObjManager::GetInst()->GetPlayer(result->obj_id);
		if (result->result == LOGINFAIL_TYPE::SIGN_UP_OK)
		{
		//cout << "OP SIGNUP" << endl;
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
	MoveObjManager::GetInst()->Disconnect(c_id);
	Player* cl = MoveObjManager::GetInst()->GetPlayer(c_id);

	lock_guard<mutex>state_guard(cl->state_lock);
	if (cl->GetRoomID() == -1)
		cl->SetState(STATE::ST_FREE);

}

bool InGameServer::StartServer()
{
	RegisterProcessFunc();
	MoveObjManager::GetInst()->InitPlayer();
	MoveObjManager::GetInst()->InitNPC();
	m_room_manager->InitRoom();
	m_map_manager->LoadMap("src/map/map.txt");
	CreateWorker();
	return false;
}

void InGameServer::TimerThread()
{
	m_event_helper->ProcessTimer();
}

void InGameServer::DBThread()
{
	m_db_manager->DBThread();
}



void InGameServer::CreateOtherThered()
{
	m_worker_threads.emplace_back(&InGameServer::TimerThread, this);
	m_worker_threads.emplace_back(&InGameServer::DBThread, this);
	
}



void InGameServer::Run()
{
	StartServer();
	CreateOtherThered();
	JoinThread();
}

void InGameServer::End()
{
	MoveObjManager::GetInst()->DestroyObject();
	MoveObjManager::DestroyInst();
	CloseHandle(IOCP_GLOBAL::g_hiocp);
	closesocket(m_s_socket);

}

void InGameServer::RegisterProcessFunc()
{
	m_PacketManager->RegisterRecvFunction(CS_PACKET_SIGN_IN, [this](int c_id, unsigned char* p) {ProcessSignIn(c_id, p); });
	m_PacketManager->RegisterRecvFunction(CS_PACKET_SIGN_UP, [this](int c_id, unsigned char* p) {ProcessSignUp(c_id, p); });
	m_PacketManager->RegisterRecvFunction(CS_PACKET_MOVE, [this](int c_id, unsigned char* p) {ProcessMove(c_id, p); });
	m_PacketManager->RegisterRecvFunction(CS_PACKET_ATTACK, [this](int c_id, unsigned char* p) {ProcessAttack(c_id, p); } );
	m_PacketManager->RegisterRecvFunction(CS_PACKET_MATCHING, [this](int c_id, unsigned char* p) {ProcessMatching(c_id, p); });
	m_PacketManager->RegisterRecvFunction(CS_PACKET_HIT, [this](int c_id, unsigned char* p) {ProcessHit(c_id, p); });
	m_PacketManager->RegisterRecvFunction(CS_PACKET_GAME_START, [this](int c_id, unsigned char* p) {ProcessGameStart(c_id, p); });
	m_PacketManager->RegisterRecvFunction(CS_PACKET_DAMAGE_CHEAT, [this](int c_id, unsigned char* p) {ProcessDamageCheat(c_id, p); });
}

void InGameServer::ProcessSignIn(int c_id, unsigned char* p)
{
	cs_packet_sign_in* packet = reinterpret_cast<cs_packet_sign_in*>(p);
	
	if (MoveObjManager::GetInst()->CheckLoginUser(packet->name) == false)
	{
		cout << "이미 접속한 유저입니다" << endl;
		m_PacketManager->SendLoginFailPacket(c_id, static_cast<int>(LOGINFAIL_TYPE::AREADY_SIGHN_IN));
		return;
	}
	db_task dt;
	dt.dt = DB_TASK_TYPE::SIGN_IN;
	dt.obj_id = c_id;
	strcpy_s(dt.user_id, packet->name);
	strcpy_s(dt.user_password, packet->password);

	m_db_manager->PushTask(move(dt));
}

void InGameServer::ProcessSignUp(int c_id, unsigned char* p)
{
	cs_packet_sign_up* packet = reinterpret_cast<cs_packet_sign_up*>(p);
	db_task dt;
	dt.dt = DB_TASK_TYPE::SIGN_UP;
	dt.obj_id = c_id;
	strcpy_s(dt.user_id, packet->name);
	strcpy_s(dt.user_password, packet->password);
	m_db_manager->PushTask(move(dt));
}

void InGameServer::ProcessAttack(int c_id, unsigned char* p)
{
	cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(p);
	Player* player = MoveObjManager::GetInst()->GetPlayer(c_id);
	if (player->GetRoomID() == -1)return;

	Room* room = m_room_manager->GetRoom(player->GetRoomID());

	auto& atk_packet = m_PacketManager->SendAttackPacket(c_id, Vector3{ packet->f_x,packet->f_y,packet->f_z });
	room->RouteToOther(atk_packet,c_id);
}

void InGameServer::ProcessMove(int c_id, unsigned char* p)
{
	cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p);
	Player* cl = MoveObjManager::GetInst()->GetPlayer(c_id);
	Room* room = m_room_manager->GetRoom(cl->GetRoomID());

	Vector3 pos{ packet->x,packet->y,packet->z };
	cl->m_last_move_time = packet->move_time;
	cl->SetPos(pos);

	if (isnan(cl->GetPosX()) || isnan(cl->GetPosY()) || isnan(cl->GetPosZ()))return;
	//여기서 힐존 검사하기
	TryHealEvent(cl);
	auto& move_packet = m_PacketManager->SendMovePacket(cl);
	room->RouteToAll(move_packet);
		
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
			if (e->InUseCAS(false,true))
				room->InsertEnemy(e);
			//if (e->in_use == false)
			//{
			//	e->in_use = true;
			//	room->InsertEnemy(e);
			//}

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
	Room* room = m_room_manager->GetRoom(attacker->GetRoomID());
	
	
	if (victim->GetRoomID() == -1 || attacker->GetRoomID() == -1)return;
	
	float hp = victim->Hit(attacker->GetDamge());
	auto& status_packet = m_PacketManager->SendStatusChange(packet->victim_id, hp);

	room->RouteToAll(status_packet);

	
	if (hp <= 0.0f)
	{
		victim->SetIsActive(false);
		auto& dead_packet = m_PacketManager->SendDead(packet->victim_id);
		room->RouteToAll(dead_packet);

		if (victim->GetType() == OBJ_TYPE::OT_PLAYER)
		{
			auto& defeat_packet = m_PacketManager->SendGameDefeat();
			room->RouteToAll(defeat_packet);
			room->GameEnd();
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
	atomic_thread_fence(memory_order_seq_cst);
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
	GS_GLOBAL::g_timer_queue.push(timer_event{ room_id, room_id, room_id,
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
			GS_GLOBAL::g_timer_queue.push(timer_event{ room_id,
				room_id, room_id, EVENT_TYPE::EVENT_NPC_SPAWN, 30 });
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
	if (false == CheckMoveOK(enemy_id, room_id))
	{
		enemy->SetToPrevPos();
		Vector3 best_way;
		float best_dist = FLT_MAX;
		for (const Vector3& way : enemy->MakeWays(target_vec))
		{
			enemy->DoMove(way);
			if (CheckMoveOK(enemy_id, room_id) == true)
			{
				float now_dist = enemy->GetPos().Dist2d(target_pos) -
					((enemy->GetPos().Dist2d(enemy->GetPrevTestPos()) + CONST_VALUE::HEURISTICS));
				if (best_dist > now_dist)
				{
					best_dist = now_dist;
					best_way = way;
				}
			}
			enemy->SetToPrevPos();
		}
		if (best_dist != FLT_MAX)
		{
			enemy->SetPrevTestPos(enemy->GetPrevPos());
			enemy->DoMove(best_way);
		}

	}
	auto& packet = m_PacketManager->SendMovePacket(enemy);
	room->RouteToAll(packet);
	CallStateMachine(enemy_id, room_id);
}

void InGameServer::DoEnemyAttack(int enemy_id, int target_id, int room_id)
{
	Room* room = m_room_manager->GetRoom(room_id);
	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);

	if (false == enemy->GetIsActive())return;

	if (target_id == BASE_ID)
	{
		Vector3 base_pos = m_map_manager->GetMapObjectByType(OBJ_TYPE::OT_BASE).GetPos();
		int base_attack_t = CONST_VALUE::ATTACK_INTERVAL;
		if (enemy->GetType() == OBJ_TYPE::OT_NPC_SKULL) {
			float dist = enemy->GetPos().Dist(base_pos);
			base_attack_t =static_cast<int>( (dist / 1500.0f) * CONST_VALUE::ATTACK_INTERVAL);
		}
		GS_GLOBAL::g_timer_queue.push(timer_event(enemy_id, enemy_id, room_id, EVENT_TYPE::EVENT_BASE_ATTACK,
			base_attack_t));
	}
	auto& packet = m_PacketManager->SendNPCAttackPacket(enemy_id, target_id);
	room->RouteToAll(packet);

	enemy->SetAttackTime();

	CallStateMachine(enemy_id, room_id);
}

void InGameServer::BaseAttackByTime(int room_id, int enemy_id)
{
	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);
	Room* room = m_room_manager->GetRoom(room_id);
	room->m_base_hp_lock.lock();
	
	float base_hp = room->GetBaseHp() - enemy->GetDamge();
	room->SetBaseHp(base_hp);
	room->m_base_hp_lock.unlock();
	
	auto& base_packet = m_PacketManager->SendBaseStatus(room_id, base_hp);
	room->RouteToAll(base_packet);
	
	if (base_hp <= 0.0f)
	{
		auto& defeat_packet = m_PacketManager->SendGameDefeat();
		room->RouteToAll(defeat_packet);
		room->GameEnd();
	}
}

void InGameServer::ActivateHealEvent(int room_id, int player_id)
{
	Player* player = MoveObjManager::GetInst()->GetPlayer(player_id);
	Room* room = m_room_manager->GetRoom(room_id);

	float now_hp=player->Heal();
	player->SetIsHeal(false);

	auto& status_packet = m_PacketManager->SendStatusChange(player_id, now_hp);
	room->RouteToAll(status_packet);

	TryHealEvent(player);
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

void InGameServer::TryHealEvent(Player* player)
{
	if (false == player->IsDamaged())
		return;
	if (false == m_map_manager->CheckInRange(player->GetPos(), OBJ_TYPE::OT_HEAL_ZONE) &&
		false == player->GetIsHeal())
		return;
	
	int id = player->GetID();
	player->SetIsHeal(true);
	GS_GLOBAL::g_timer_queue.push(timer_event(id, id, player->GetRoomID(),
		EVENT_TYPE::EVENT_HEAL, CONST_VALUE::HEAL_TIME));
}

void InGameServer::CallStateMachine(int enemy_id, int room_id)
{
	Enemy* enemy = MoveObjManager::GetInst()->GetEnemy(enemy_id);
	Room* room = m_room_manager->GetRoom(room_id);
	float comp_dist = sqrt(pow(abs(CONST_VALUE::g_ground_base_pos.x - enemy->GetPos().x), 2)
		+ pow(abs(CONST_VALUE::g_ground_base_pos.z - enemy->GetPos().z), 2));
	int comp_target_id = BASE_ID;

	if (enemy->CheckTargetChangeTime() == true)
	{
		for (auto pl : room->GetObjList())
		{
			if (false == MoveObjManager::GetInst()->IsNear(pl->GetID(), enemy_id))//시야범위안에 있는지 확인
				continue;
			if (false == m_map_manager->CheckInRange(pl->GetPos(), OBJ_TYPE::OT_ACTIViTY_AREA)) continue;
			float now_dist = MoveObjManager::GetInst()->ObjDistance(pl->GetID(), enemy_id);
			if (comp_dist >= now_dist)
			{
				comp_dist = now_dist;
				comp_target_id = pl->GetID();
			}
			
		}
		enemy->SetTargetId(comp_target_id);
	}

	enemy->CallLuaStateMachine();
}
