#include "pch.h"
#include "room_manager.h"
#include"room.h"
void RoomManager::InitRoom()
{
	for (int i = 0; i < MAX_ROOM_SIZE; ++i)
		m_rooms[i] = new Room(i);
}

void RoomManager::DestroyRoom()
{
	for (auto r : m_rooms)
		if (r)delete r;
}

bool RoomManager::IsRoomInGame(int room_id)
{
	Room* r = m_rooms[room_id];
	std::lock_guard<std::mutex>guard(r->m_state_lock);
	if (r->GetState() == ROOM_STATE::RT_INGAME)
		return true;
	return false;
}

int RoomManager::CreateMatchingRoom(int matchUserSize, Player* player)
{
	for (auto r : m_rooms)
	{
		r->m_state_lock.lock();
		if (ROOM_STATE::RT_FREE == r->GetState())
		{
			r->Init(matchUserSize);
			r->EnterRoom(player);
			r->SetState(ROOM_STATE::RT_MATCHING);
			r->m_state_lock.unlock();
			return r->GetRoomID();
		}
		r->m_state_lock.unlock();
	}
	
	return -1;
}

MATCHING_RESULT RoomManager::SearchMatchingRoom(int matchUserSize,Player*player)
{
	MATCHING_RESULT ret ;
	for (Room* room : m_rooms)
	{
		if (room->GetMaxUser() != matchUserSize)
			continue;
		std::lock_guard<std::mutex>guard(room->m_state_lock);
		if (room->GetState() == ROOM_STATE::RT_MATCHING)
		{
			room->SetState(ROOM_STATE::RT_INGAME);
			

			if (room->GetCurrentUserSize() + 1 < matchUserSize)
			{
				room->EnterRoom(player);
				
				room->SetState(ROOM_STATE::RT_MATCHING);
				

				ret= MATCHING_RESULT::NONE;
				break;
			}
			else if (room->GetCurrentUserSize() + 1 == matchUserSize)
			{
				room->EnterRoom(player);
				ret= MATCHING_RESULT::COMPLETE;
				break;
			}
			
		}
		
	}
	
	return ret;
}
