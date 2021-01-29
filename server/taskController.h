#pragma once

#include "threadpool.h"
#include <unordered_map>
#include "socketBase.h"

namespace WG
{
	class taskData
	{
	public:
		taskData(int clientId, char* databuf, unsigned int len) :m_clientId(clientId), m_databuf(databuf), m_len(len) { m_curLen = 0; };
		virtual ~taskData() { if (m_databuf) { delete m_databuf; m_databuf = NULL; } }
		int m_clientId;
		char* m_databuf;
		unsigned int m_len;
		unsigned int m_curLen;
	};

    class taskController : public threadTaskWorker, public socketListener
    {
    public:
        taskController(socketBase* socket);
        virtual ~taskController();
		//implement threadTaskWorer
        virtual void onTaskProcessor(void* param) override;
		//implement socketListener
		virtual void onRecv(int socket, void* data, unsigned int len) override;
        void makeNewTask(int clientID, char* buf, unsigned int len);
        
    private:
		socketBase* m_socket;
		std::unordered_map<int, taskData*> m_recvList; //socketId, msgId, data
    };
}