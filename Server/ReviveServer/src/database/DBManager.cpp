#include "DBManager.h"
#include"database/db.h"
#include<thread>
using namespace std;
void DBManager::DBThread()
{
	while (true)
	{
		db_task dt;
		if (!m_db_queue.try_pop(dt))
		{
			this_thread::sleep_for(10ms);
			continue;
		}
		ProcessDBTask(dt);
	}
}

void DBManager::ProcessDBTask(db_task& dt)
{
	
	switch (dt.dt)
	{
	case DB_TASK_TYPE::SIGN_IN:
	{

		db_login_result result;
		result.obj_id = dt.obj_id;
		result.result = m_account_db->CheckLoginData(dt.user_id, dt.user_password);
		if (result.result == LOGINFAIL_TYPE::OK)
		{
			
			
			//접속꽉찬거는 accept 쪽에서 보내기주기
			//pl->state_lock.lock();
			//if (STATE::ST_FREE == pl->GetState() || STATE::ST_ACCEPT == pl->GetState())
			//{
			//	pl->SetState(STATE::ST_LOGIN);
			//	pl->state_lock.unlock();
			//}
			//else
			//	pl->state_lock.unlock();
			strcpy_s(result.user_id, MAX_NAME_SIZE, dt.user_id);
			strcpy_s(result.user_password, MAX_NAME_SIZE, dt.user_password);
			//여기오면 성공패킷 보내주기
			//SendSignInOK(pl->GetID());
		}
		else
		{
			//로그인 실패 패킷보내기
			//SendLoginFailPacket(dt.obj_id, static_cast<int>(ret));
		}
		EXP_OVER* ex_over = new EXP_OVER(COMP_OP::OP_LOGIN, sizeof(result), &result);
		PostQueuedCompletionStatus(IOCP_GLOBAL::g_hiocp, 1, result.obj_id, &ex_over->_wsa_over);
		break;
	}
	case DB_TASK_TYPE::SIGN_UP:
	{
		db_result result;
		result.obj_id = dt.obj_id;
		LOGINFAIL_TYPE ret = m_account_db->CheckLoginData(dt.user_id, dt.user_password);
		if (ret == LOGINFAIL_TYPE::NO_ID || ret == LOGINFAIL_TYPE::WRONG_PASSWORD)
		{

			result.result = m_account_db->SaveData(dt.user_id, dt.user_password);
			//SendSignUpOK(dt.obj_id);
		}
		else
			result.result = LOGINFAIL_TYPE::SIGN_UP_FAIL;
		EXP_OVER* ex_over = new EXP_OVER(COMP_OP::OP_SIGNUP, sizeof(result), &result);
		PostQueuedCompletionStatus(IOCP_GLOBAL::g_hiocp, 1, result.obj_id, &ex_over->_wsa_over);
			// 아이디가 있어 회원가입 불가능 패킷 보내기
			//SendLoginFailPacket(dt.obj_id, 6);
		break;
	}
	}
}
