#pragma once
#include"../object.h"
#include<iostream>
#include<mutex>
#include<atomic>
//플레이어와 npc의 부모
class MoveObj :public Object
{
public:
	MoveObj() { 
		
	};

	virtual ~MoveObj() = default;


	

	float GetDamge()const { return m_damage; }
	float GetHP()const { return m_hp; }
	float GetMaxHP()const { return m_maxhp; }
	
	virtual void Reset()
	{
		m_is_active = false;
		ZeroMemory(m_name, MAX_NAME_SIZE + 1);
	}
	
	
	int GetRoomID()const { return m_room_id; }
	char* GetName() { return m_name; }
	COLOR_TYPE GetColorType()const { return m_color_type; }
	bool GetIsActive()const { return m_is_active.load(); }
	void SetIsActive(bool val) { m_is_active.store(val); }
	
	
	void SetDamge(float val) {  m_damage=val; }
	void SetHP(float val)    {  m_hp=val; }
	void SetMaxHP(float val) {  m_maxhp=val; }
	void SetColorType(COLOR_TYPE val) { m_color_type = val; }
	void SetRoomID(int val) { m_room_id = val; }
	float Hit(float damage)
	{
		std::lock_guard<std::mutex>guard(this->m_hp_lock);
		if (m_hp - damage < 0)
			m_hp = 0;
		SetHP(m_hp - damage);
		return m_hp;
	}



	std::mutex m_hp_lock;
	int		m_last_move_time=0;
protected:
	std::atomic_bool m_is_active = false; //죽어있는지 살아있는지
	int m_room_id;
	float m_damage;
	float m_hp, m_maxhp;                      
	COLOR_TYPE m_color_type;
	char m_name[MAX_NAME_SIZE + 1];

	Vector3 m_origin_pos;
};