#pragma once
#include"util/state.h"
#include<chrono>
class timer_event {
public:
	timer_event() {}
	timer_event(int _obj_id, int _target_id, EVENT_TYPE _ev, int _seconds);
	timer_event(int _obj_id, int _target_id, int _room_id, EVENT_TYPE _ev, int _seconds);
	int obj_id;
	int room_id;
	std::chrono::system_clock::time_point	start_time;
	EVENT_TYPE ev;
	int target_id;
	constexpr bool operator < (const timer_event& _Left) const
	{
		return (start_time > _Left.start_time);
	}
};

