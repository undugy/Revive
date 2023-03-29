#pragma once
#include "move_object.h"
#include "define.h"
#include"util/collision/collisioner.h"
#include"component/LuaComponent.h"
#include<mutex>
#include<atomic>
#include<vector>
const int SYNC_TIME = 30;
class CollisonComponent;
class Enemy :
    public MoveObj
{
public:
    Enemy(int i) :in_use(false),
        m_L(NULL),
        m_prev_test_pos(CONST_VALUE::g_ground_base_pos.x,
            CONST_VALUE::g_ground_base_pos.y,0.0f),
        target_id(BASE_ID)
        
    {
        m_id = i;
        m_color_type = COLOR_TYPE::CT_NONE;
        m_attack_time = std::chrono::system_clock::now();
        m_check_time= std::chrono::system_clock::now();
    };
    ~Enemy() 
    {
        if(m_L)
            lua_close(m_L);
    };
    

  
    bool Init(OBJ_TYPE type);
    void SetSpawnPoint(float x,float z);
    Vector3& GetLookVec()  { return m_look; }
    Vector3& GetPrevPos() { return m_prev_pos; }
    Vector3& GetPrevTestPos() { return m_prev_test_pos; }
    BoxCollision& GetCollision() { return m_collision; }
    void SetCollision(const BoxCollision& val) { m_collision = val; }
    void SetPrevTestPos(const Vector3& val) { m_prev_test_pos = val; }
    virtual void Reset()override;

   
    const int GetTargetId()const { return target_id; }
    void SetTargetId(const int val) { target_id = val; }
    std::chrono::system_clock::time_point& GetAttackTime() { return m_attack_time; }
    std::chrono::system_clock::time_point& GetCheckTime() { return m_check_time; }
    void DoMove(const Vector3& target_pos);
    void DoPrevMove(const Vector3& target_pos);
    void SetToPrevPos() { 
        m_pos = m_prev_pos; 
        m_collision.UpdateCollision(m_pos);
    }
    
    std::atomic_bool in_use;
    std::mutex lua_lock;
    Vector3 m_prev_test_pos{ 0.0f,0.0f,0.0f };
private:
    std::chrono::system_clock::time_point	m_attack_time;
    std::chrono::system_clock::time_point	m_check_time;
    BoxCollision m_collision;
    LuaComponent m_lua;
    Vector3 m_prev_pos{ 0.0f,0.0f,0.0f };
    lua_State* m_L;
    Vector3 m_look;
    std::atomic_int target_id;
};

