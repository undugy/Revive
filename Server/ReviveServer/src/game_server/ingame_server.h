#pragma once
#include "iocp/iocp_server.h"

class DBManager;
class PacketManager;
class EventHelper;
class RoomManager;
class MapManager;
class EXP_OVER;
class Player;

class InGameServer :
    public IOCPServer
{
public:
    InGameServer();
    virtual ~InGameServer();
    
   
    virtual bool OnAccept( EXP_OVER* exp_over) override;
    virtual bool OnRecv(int c_id, EXP_OVER* exp_over,DWORD num_bytes) override;
    virtual void OnEvent(int c_id,EXP_OVER* exp_over)override;
    virtual void Disconnect(int c_id)override;
    virtual bool StartServer()override;


    void CreateOtherThered();
    void TimerThread();
    void DBThread();
    
    void Run();
    void End();

private:
    void RegisterProcessFunc();
    void ProcessSignIn(int c_id, unsigned char* p);
    void ProcessSignUp(int c_id, unsigned char* p);
    void ProcessAttack(int c_id, unsigned char* p);
    void ProcessMove(int c_id, unsigned char* p);
    void ProcessMatching(int c_id, unsigned char* p);
    void ProcessHit(int c_id, unsigned char* p);
    void ProcessGameStart(int c_id, unsigned char* p);
    void ProcessDamageCheat(int c_id, unsigned char* p);
    
    
    
    
    void StartGame(int room_id);
    void CountTime(EXP_OVER* exp_over);
    void SpawnEnemy(int room_id);
    void SpawnEnemyByTime(int enemy_id, int room_id);
    void DoEnemyMove(int room_id, int enemy_id);
    void DoEnemyAttack(int enemy_id, int target_id, int room_id);
    void BaseAttackByTime(int room_id, int enemy_id);
    void ActivateHealEvent(int room_id, int player_id);
   
    
    bool CheckMoveOK(int enemy_id, int room_id);
    void TryHealEvent(Player* player);
    void CallStateMachine(int enemy_id, int room_id);
    std::unique_ptr<PacketManager> m_PacketManager;
    std::unique_ptr<RoomManager>   m_room_manager;
    std::unique_ptr<MapManager>    m_map_manager;
    std::unique_ptr<DBManager>     m_db_manager;
    std::unique_ptr<EventHelper>   m_event_helper;
};

