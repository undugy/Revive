#include<chrono>
#include<thread>

#include "EventHelper.h"
#include"define.h"
using namespace std;
void EventHelper::ProcessTimer()
{
	timer_event ev;
	while (true) {
		while (true) {

			if (!GS_GLOBAL::g_timer_queue.try_pop(ev))continue;

			auto start_t = chrono::system_clock::now();
			if (ev.start_time <= start_t) {
				ProcessEvent(ev);
			}
			else if (10ms >= ev.start_time - start_t)
			{
				this_thread::sleep_for(ev.start_time - start_t);
				ProcessEvent(ev);
			}
			else {
				GS_GLOBAL::g_timer_queue.push(ev);
				break;
			}
		}

		this_thread::sleep_for(10ms);
	}
}

void EventHelper::ProcessEvent(const timer_event& ev)
{
	EXP_OVER* ex_over = new EXP_OVER;

	switch (ev.ev) {
	case EVENT_TYPE::EVENT_NPC_SPAWN:
	{
		ex_over->_comp_op = COMP_OP::OP_NPC_SPAWN;
		ex_over->target_id = ev.target_id;
		ex_over->room_id = ev.room_id;
		break;
	}
	case EVENT_TYPE::EVENT_NPC_TIMER_SPAWN:
	{
		ex_over->_comp_op = COMP_OP::OP_NPC_TIMER_SPAWN;
		ex_over->target_id = ev.target_id;
		ex_over->room_id = ev.room_id;
		break;
	}
	case EVENT_TYPE::EVENT_NPC_MOVE: {
		ex_over->_comp_op = COMP_OP::OP_NPC_MOVE;
		ex_over->room_id = ev.room_id;
		ex_over->target_id = ev.target_id;
		break;
	}
	case EVENT_TYPE::EVENT_NPC_ATTACK: {
		ex_over->_comp_op = COMP_OP::OP_NPC_ATTACK;
		ex_over->room_id = ev.room_id;
		ex_over->target_id = ev.target_id;
		break;
	}
	case EVENT_TYPE::EVENT_TIME: {
		ex_over->_comp_op = COMP_OP::OP_COUNT_TIME;
		ex_over->room_id = ev.obj_id;
		break;
	}
	case EVENT_TYPE::EVENT_REFRESH_ROOM: {
		
		ex_over->_comp_op = COMP_OP::OP_FRESH_ROOM;
		ex_over->room_id = ev.room_id;
		break;
	}
	case EVENT_TYPE::EVENT_BASE_ATTACK: {
		ex_over->_comp_op = COMP_OP::OP_BASE_ATTACK;
		ex_over->room_id = ev.room_id;

		break;
	}
	case EVENT_TYPE::EVENT_HEAL: {
		ex_over->_comp_op = COMP_OP::OP_HEAL;
		ex_over->room_id = ev.room_id;
		ex_over->target_id = ev.target_id;
		break;
	}

	}
		PostQueuedCompletionStatus(IOCP_GLOBAL::g_hiocp, 1, ev.obj_id, &ex_over->_wsa_over);
}
