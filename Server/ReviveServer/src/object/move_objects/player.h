#pragma once
#include "move_object.h"
#include"define.h"
class Player :
    public MoveObj
{
public:
    Player():
        m_prev_size(0), m_socket(INVALID_SOCKET)
    {
        m_last_move_time=0 ;
        m_type = OBJ_TYPE::OT_PLAYER;
        m_state = STATE::ST_FREE;
        m_room_id = -1;
        m_damage = PLAYER_DAMAGE;
        ZeroMemory(m_password, MAX_PASSWORD_SIZE + 1);
        m_hp = PLAYER_HP;//추후 밸런스 조정
        m_maxhp = m_hp;
        
    }
   virtual ~Player()=default;
   std::mutex state_lock;
   int		m_prev_size;
    
private:
    
    EXP_OVER m_recv_over;
    SOCKET  m_socket;
    STATE m_state;
    std::atomic_bool m_is_ready = false;
    std::atomic_bool m_is_heal = false;
    char m_password[MAX_PASSWORD_SIZE + 1];
    short m_mach_user_size = 0;
public:

    
    void SetMatchingState(int room_id);
    STATE GetState()const { return m_state; }
    void SetState(STATE val) { m_state = val; }
    void DoRecv();
    void DoSend(int num_bytes, void* mess);
    SOCKET& GetSock() { return m_socket; }
    void Init(SOCKET&);
    void ResetPlayer();
    virtual void Reset()override;
    void SetIsReady(bool val) { m_is_ready = val; }
    bool GetIsReady() { return m_is_ready; }
    void SetIsHeal(bool val) { m_is_heal = val; }
    bool GetIsHeal() { return m_is_heal; }
    bool IsDamaged();
    float Heal();
    char* GetPassword() { return m_password; }
    short GetMatchUserSize() { return m_mach_user_size; }
    void SetMatchUserSize(short val) { m_mach_user_size = val; }
};

