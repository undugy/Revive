#pragma once
#include<concurrent_queue.h>
#include"define.h"
class DB;
class DBManager
{
public:
	DBManager();
	~DBManager();
	


	void DBThread();
	void ProcessDBTask(db_task& dt);
	void PushTask(const db_task& dt)
	{
		m_db_queue.push(dt);
	}
private:
	concurrency::concurrent_queue<db_task>m_db_queue;
	DB* m_account_db;
};

