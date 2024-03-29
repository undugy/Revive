#include "moveobj_manager.h"
#include"define.h"

MoveObjManager* MoveObjManager::m_pInst = nullptr;
using namespace std;



bool MoveObjManager::IsNear(int a, int b)
{
	Vector3& obj_a = m_moveobj_arr[a]->GetPos();
	Vector3& obj_b = m_moveobj_arr[b]->GetPos();
	return (FOV_RANGE > sqrt(pow(abs(obj_a.x - obj_b.x), 2) + pow(abs(obj_a.z - obj_b.z), 2)));
	
}

float MoveObjManager::ObjDistance(int a, int b)
{
	Vector3& obj_a = m_moveobj_arr[a]->GetPos();
	Vector3& obj_b = m_moveobj_arr[b]->GetPos();
	return sqrt(pow(abs(obj_a.x - obj_b.x), 2) + pow(abs(obj_a.z - obj_b.z), 2));
}




void MoveObjManager::LuaErrorDisplay(lua_State* L,int err_num)
{
	
		cout << "Error : " << lua_tostring(L, -1)<<endl; 
		lua_pop(L, 1); 
	
}

int MoveObjManager::GetNewID()
{
    Player* cl = NULL;
    for (int i = 0; i < MAX_USER; ++i)
    {
		
        cl = GetPlayer(i);
		
       cl->state_lock.lock();
        if (STATE::ST_FREE == cl->GetState())
        {
            cl->SetState(STATE::ST_ACCEPT);
           cl->state_lock.unlock();
            return i;
        }
		cl->state_lock.unlock();
    }
    cout << "남는아이디 없음" << endl;
    return -1;
}

void MoveObjManager::Disconnect(int c_id)
{
	Player* cl = GetPlayer(c_id);
	cl->state_lock.lock();
	closesocket(cl->GetSock());
	cl->ResetPlayer();
	cl->state_lock.unlock();
}

void MoveObjManager::InitPlayer()
{
	for (int i = 0; i < MAX_USER; ++i)
		m_moveobj_arr[i] = new Player;

	
}

void MoveObjManager::InitNPC()
{
	for (int i = NPC_ID_START; i <= NPC_ID_END; ++i)
		m_moveobj_arr[i] = new Enemy(i);
}

void MoveObjManager::DestroyObject()
{
	for (auto obj : m_moveobj_arr)
	{
		if (true == IsPlayer(obj->GetID()))
		{
			if (GetPlayer(obj->GetID())->GetState() != STATE::ST_FREE)
				Disconnect(obj->GetID());
		}
		if (obj)
		{
			delete obj;
			obj = NULL;
		}
	}
}

bool MoveObjManager::CheckLoginUser(char * user_id)
{
	for (int i = 0; i < MAX_USER; ++i)
	{
		Player* other_pl = reinterpret_cast<Player*>(m_moveobj_arr[i]);
		other_pl->state_lock.lock();
		if (other_pl->GetState() == STATE::ST_FREE)
		{
			other_pl->state_lock.unlock();
			continue;
		}
		other_pl->state_lock.unlock();
		if (strcmp(other_pl->GetName(), user_id) == 0)
			return false;
		
	}
	return true;
}


