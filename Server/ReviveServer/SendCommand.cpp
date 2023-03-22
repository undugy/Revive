#include "SendCommand.h"
#include"object/move_objects/player.h"
void SendCommand::SendMsgTo(Player* player)
{
	player->DoSend(_size, _mess);
}
