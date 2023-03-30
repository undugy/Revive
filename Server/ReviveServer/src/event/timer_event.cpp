#include "timer_event.h"


using namespace std;
timer_event::timer_event(int _obj_id, int _target_id, EVENT_TYPE _ev, int _seconds)
	:obj_id(_obj_id), target_id(_target_id), ev(_ev),
	start_time(std::chrono::system_clock::now() + (1ms * _seconds))
{

}

timer_event::timer_event(int _obj_id, int _target_id, int _room_id, EVENT_TYPE _ev, int _seconds)
	:obj_id(_obj_id), target_id(_target_id), room_id(_room_id), ev(_ev),
	start_time(std::chrono::system_clock::now() + (1ms * _seconds))
{

}