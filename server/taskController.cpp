#include "taskController.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <mutex>
#include <string>
#include <cstring>
#include "socketBase.h"
#include "protocol.h"

#include "seclog.h"
#include "hexChar.h"

using namespace WG;
using namespace wg_protocol;

taskController::taskController(socketBase* socket):m_socket(socket)
{
    threadPool::getInstance()->initTaskPool(8,2);
}

taskController::~taskController()
{
    threadPool::delInstance();
    google::ShutdownGoogleLogging();
}

void taskController::onTaskProcessor(void* param)
{
    taskData* data = (taskData*)param;
	if (!data)
	{
		return;
	}
	std::string resp;
	unsigned int msgId = 0;
	int cmd;
	void* cmdData = NULL;
	unsigned int cmdDataLen = 0;
	if (-1 == protocol::getInstance()->parseRequest(data->m_databuf, data->m_len, msgId, cmd, cmdData,cmdDataLen))
	{
		SECLOG(secsdk::ERROR) << "parse request failed";
		return;
	}
	switch ((ENUM_API_COMMAND)cmd)
	{
		case EAC_API1:
		{
			//call api1 method
			char apiReturnData[1024] = { 0 };
			memset(apiReturnData, 'A', 1024);
			if (0 != protocol::getInstance()->makeAPIResponse(msgId, EAC_API1, apiReturnData, 1024, resp))
			{
				SECLOG(secsdk::ERROR) << "make API1 Response failed";
				protocol::getInstance()->makeErrResponse(msgId, EAC_API1, EPE_Make_ERROR, resp);
			}
			break;
		}
		case EAC_API2:
		{
			//call api1 method
			char apiReturnData[1024] = { 0 };
			memset(apiReturnData, 'B', 1024);
			if (0 != protocol::getInstance()->makeAPIResponse(msgId, EAC_API2, apiReturnData, 1024, resp))
			{
				SECLOG(secsdk::ERROR) << "make API2 Response failed";
				protocol::getInstance()->makeErrResponse(msgId, EAC_API2, EPE_Make_ERROR, resp);
			}
			break;
		}
		default:
			break;
	}
	if (m_socket)
	{
		unsigned char outdata[1024] = { 0 };
		CharToHex(outdata, (unsigned char*)resp.c_str(), resp.size());
		SECLOG(secsdk::INFO) << " data will send : " << outdata;
		m_socket->send(data->m_clientId, (void*)resp.data(), resp.size());
	}
    SECLOG(secsdk::INFO) << "will leave";
}

void taskController::onRecv(int socket, void * data, unsigned int len)
{
	makeNewTask(socket, (char*)data, len);
}

void taskController::makeNewTask(int clientID, char* buf, unsigned int len)
{
	//check data received is competed
	auto iter = m_recvList.find(clientID);
	if (iter == m_recvList.end())
	{
		//parse protocol
		unsigned int totalLen = 0;
		memcpy(&totalLen, buf, sizeof(unsigned int));

		char* revBuf = new char[totalLen];
		if (!revBuf)
		{
			SECLOG(secsdk::ERROR) << "make new task failed while malloc momery";
			return;
		}

		taskData* data = new taskData(clientID, revBuf, totalLen);
		if (totalLen <= len)
		{
			memcpy(revBuf, buf, totalLen);
			std::shared_ptr<threadTask> taskdata = std::make_shared<threadTask>(this, (void*)data);
			threadPool::getInstance()->addTask(taskdata);
			if (totalLen < len)
			{
				makeNewTask(clientID, buf + totalLen, len - totalLen);
			}
		}
		else if(totalLen > len)
		{
			memcpy(revBuf, buf, len);
			data->m_curLen += len;
			m_recvList.insert(std::make_pair<int, taskData*>(std::move(clientID), std::move(data)));
		}
	}
	else //merge the buf which split while recviving because the recv buf.
	{
		taskData* data = iter->second;
		if (len + data->m_curLen < data->m_len)
		{
			memcpy(data->m_databuf + data->m_curLen, buf, len);
			data->m_curLen += len;
			SECLOG(secsdk::INFO) << " append data : " << data->m_curLen;
		}
		else
		{
			memcpy(data->m_databuf + data->m_curLen, buf, data->m_len - data->m_curLen);

			std::shared_ptr<threadTask> taskdata = std::make_shared<threadTask>(this, (void*)data);
			threadPool::getInstance()->addTask(taskdata);
			m_recvList.erase(iter);

			if (len + data->m_curLen > data->m_len)
			{
				makeNewTask(clientID, buf + data->m_len - data->m_curLen, len - (data->m_len - data->m_curLen));
			}
		}
	}    
}
