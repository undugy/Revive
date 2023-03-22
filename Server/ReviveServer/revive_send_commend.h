#pragma once
#include"SendCommand.h"
#include"protocol.h"


class MoveObj;
class SendMoveCommand
	:public SendCommand
{
	SendMoveCommand(MoveObj* mover);

private:
	sc_packet_move _packet;
};

class SendNPCAttackCommand
	:public SendCommand
{
	SendNPCAttackCommand(int obj_id,int target_id);

private:
	sc_packet_npc_attack _packet;
};

class SendLoginFailCommand
	:public SendCommand
{
	SendLoginFailCommand(int reason);

private:
	sc_packet_login_fail _packet;
};

