#include "revive_send_commend.h"
#include"object/move_objects/move_object.h"
SendMoveCommand::SendMoveCommand(MoveObj* mover)
	:SendCommand(sizeof(_packet),&_packet)
{
	_packet.id = mover->GetID();
	_packet.size = sizeof(_packet);
	_packet.type = SC_PACKET_MOVE;
	
	_packet.x = mover->GetPosX();
	_packet.y = mover->GetPosY();
	_packet.z = mover->GetPosZ();
	_packet.move_time = mover->m_last_move_time;
}

SendNPCAttackCommand::SendNPCAttackCommand(int obj_id, int target_id)
	:SendCommand(sizeof(_packet), &_packet)
{
	_packet.size = sizeof(_packet);
	_packet.type = SC_PACKET_NPC_ATTACK;
	_packet.obj_id = obj_id;
	_packet.target_id = target_id;
}

SendLoginFailCommand::SendLoginFailCommand(int reason)
	:SendCommand(sizeof(_packet), &_packet)
{
	_packet.size = sizeof(_packet);
	_packet.type = SC_PACKET_LOGIN_FAIL;
	_packet.reason = reason;
}
