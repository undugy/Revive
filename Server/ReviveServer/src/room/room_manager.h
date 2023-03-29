#pragma once
#include<array>

#include"define.h"
class Room;
class Player;

enum MATCHING_RESULT
{
	NONE,
	COMPLETE,
	NO_ROOM
};


class RoomManager
{
public:
	RoomManager()=default;
	~RoomManager()
	{
		DestroyRoom();
	}

	void InitRoom();
	void DestroyRoom();
	bool IsRoomInGame(int room_id);
	int CreateMatchingRoom(int matchUserSize,Player*player);
	MATCHING_RESULT SearchMatchingRoom(int matchUserSize, Player*player);
	Room* GetRoom(int r_id) { return m_rooms[r_id]; }

private:
	std::array<Room*, MAX_ROOM_SIZE>m_rooms;

};

