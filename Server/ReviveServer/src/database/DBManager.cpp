#include "DBManager.h"
#include"database/db.h"
#include<thread>
using namespace std;
DBManager::DBManager()
{
	
		m_account_db = new DB;
		m_account_db->Init();
	
}
DBManager::~DBManager()
{
	if(m_account_db) {
		delete m_account_db;
	}
}
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
			strcpy_s(result.user_id, MAX_NAME_SIZE, dt.user_id);
			strcpy_s(result.user_password, MAX_NAME_SIZE, dt.user_password);
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
		}
		else
			result.result = LOGINFAIL_TYPE::SIGN_UP_FAIL;
		EXP_OVER* ex_over = new EXP_OVER(COMP_OP::OP_SIGNUP, sizeof(result), &result);
		PostQueuedCompletionStatus(IOCP_GLOBAL::g_hiocp, 1, result.obj_id, &ex_over->_wsa_over);
		break;
	}
	}
}
