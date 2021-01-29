#include "protocol.h"
#include <cstring>

using namespace wg_protocol;

protocol* protocol::m_instance = NULL;
unsigned int protocol::m_msgId = 0;
std::mutex protocol::m_mutex;

protocol* protocol::getInstance()
{
	m_mutex.lock();
	if (m_instance == NULL)
	{
		m_instance = new protocol();
	}
	m_mutex.unlock();
	return m_instance;
}

void protocol::delInstance()
{
	m_mutex.lock();
	if (m_instance)
	{
		delete m_instance;
		m_instance = NULL;
	}
	m_mutex.unlock();
}

//[totalLen:4][msgId:4][command:4][DataLen:4][Data:X]
int protocol::makeAPIRequest(ENUM_API_COMMAND cmd, void* buf, unsigned int size, std::string& request)
{
	if (buf == NULL || size == 0)
	{
		return -1;
	}
	unsigned int totoleSize = sizeof(unsigned int) * 4 + size;
	request.resize(totoleSize);
	memcpy((char*)request.data(), &(totoleSize), sizeof(unsigned int));
	memcpy((char*)request.data() + sizeof(unsigned int), &(++m_msgId), sizeof(unsigned int));
	memcpy((char*)request.data() + sizeof(unsigned int) * 2, &cmd, sizeof(cmd));
	memcpy((char*)request.data() + sizeof(unsigned int) * 3, &size, sizeof(unsigned int));
	memcpy((char*)request.data() + sizeof(unsigned int) * 4, buf, size);
	return 0;
}


int protocol::parseRequest(const char* reqbuf, unsigned int bufLen, unsigned int& msgId, int& cmd, void*& cmdData, unsigned int& cmdDataLen)
{
	if (reqbuf == NULL || bufLen <  sizeof(unsigned int) * 4)
	{
		return -1;
	}
	memcpy(&msgId, reqbuf + sizeof(unsigned int), sizeof(unsigned int));
	memcpy(&cmd, reqbuf + sizeof(unsigned int) * 2, sizeof(int));
	memcpy(&cmdDataLen, reqbuf + sizeof(unsigned int) * 3, sizeof(unsigned int));
	cmdData = (char*)reqbuf + sizeof(unsigned int) * 4;
	return 0;
}

//[totalLen:4][msgId:4][command:4][result:4][dataLen:4][Data:x]
int protocol::makeAPIResponse(unsigned int msgId, ENUM_API_COMMAND cmd, void* data, unsigned int dataLen, std::string& resp)
{
	unsigned int totoleSize = sizeof(unsigned int) * 5 + dataLen;
	resp.resize(totoleSize);
	memcpy((char*)resp.data(), &(totoleSize), sizeof(unsigned int));
	memcpy((char*)resp.data() + sizeof(unsigned int), &msgId, sizeof(unsigned int));
	memcpy((char*)resp.data() + sizeof(unsigned int) * 2, &cmd, sizeof(cmd));
	memset((char*)resp.data() + sizeof(unsigned int) * 3, 0, sizeof(int));
	memcpy((char*)resp.data() + sizeof(unsigned int) * 4, &dataLen, sizeof(unsigned int));
	memcpy((char*)resp.data() + sizeof(unsigned int) * 5, data, dataLen);
	return 0;
}

int protocol::parseResponse(const char* respbuf, unsigned int bufLen, unsigned int& msgId, int& cmd, int& result, void*& cmdData, unsigned int& cmdDataLen)
{
	if (respbuf == NULL || bufLen < sizeof(unsigned int) * 4)
	{
		return -1;
	}
	memcpy(&msgId, respbuf + sizeof(unsigned int), sizeof(unsigned int));
	memcpy(&cmd, respbuf + sizeof(unsigned int) * 2, sizeof(int));
	memcpy(&result, respbuf + sizeof(int) * 3, sizeof(int));
	if (result == 0)
	{
		memcpy(&cmdDataLen, respbuf + sizeof(unsigned int) * 4, sizeof(unsigned int));
		cmdData = (char*)respbuf + sizeof(unsigned int) * 5;
	}
	else
	{
		cmdData = NULL;
		cmdDataLen = 0;
	}
	return 0;
}

int protocol::makeErrResponse(unsigned int msgId, ENUM_API_COMMAND cmd, ENUM_PROTOCOL_ERROR errCode, std::string& resp)
{
	unsigned int totoleSize = sizeof(unsigned int) * 4;
	resp.resize(totoleSize);
	memcpy((char*)resp.data(), &(totoleSize), sizeof(unsigned int));
	memcpy((char*)resp.data() + sizeof(unsigned int), &msgId, sizeof(unsigned int));
	memcpy((char*)resp.data() + sizeof(unsigned int) * 2, &cmd, sizeof(cmd));
	memcpy((char*)resp.data() + sizeof(unsigned int) * 3, &errCode, sizeof(int));
	return 0;
}