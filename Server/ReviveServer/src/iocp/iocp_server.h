#pragma once
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")
#include"define.h"
#include <thread>
#include <vector>
#include<array>

class EXP_OVER;
class IOCPServer
{
public:
	IOCPServer();
	virtual ~IOCPServer();

	bool Init(const int);
	bool BindListen(const int);

	virtual void Disconnect(int c_id) {}
	virtual bool OnRecv(int c_id, EXP_OVER* exp_over, DWORD num_bytes) { return true; };
	virtual bool OnAccept( EXP_OVER* exp_over) { return false; };
	virtual void OnEvent(int c_id,EXP_OVER* exp_over) { return; };
	static void error_display(int err_no);
	virtual bool StartServer();

	void JoinThread();
public:

	void CreateWorker();

	void Worker();
	
protected:
	EXP_OVER	accept_ex;
	SOCKET m_s_socket;

	int m_worker_num;
	std::vector <std::thread> m_worker_threads;
	

};

