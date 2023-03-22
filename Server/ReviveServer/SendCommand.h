#pragma once
class Player;


class SendCommand
{
public:
	SendCommand(int size, void* mess)
		:_size(size),_mess(mess)
	{

	}
	virtual ~SendCommand() {};

	void SendMsgTo(Player* player);
private:
	int _size;
	void* _mess;
};

