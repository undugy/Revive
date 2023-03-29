#pragma once
#include"define.h"
#include<concurrent_queue.h>
class DB;
class DBManager
{
public:
	DBManager()
	{
		m_account_db = new DB;
	}
	~DBManager()
	{
		delete m_account_db;
	}


	void DBThread();
	void ProcessDBTask(db_task& dt);
	concurrency::concurrent_queue<db_task>m_db_queue;
	void PushTask(const db_task& dt)
	{
		m_db_queue.push(move(dt));
	}

	DB* m_account_db;
};

