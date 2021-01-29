#pragma once

#include <string>
#include <mutex>

namespace wg_protocol {
	enum ENUM_API_COMMAND
	{
		EAC_API1 = 0,
		EAC_API2,
	};

	enum ENUM_PROTOCOL_ERROR
	{
		EPE_Parse_ERROR = 500,
		EPE_Make_ERROR,
	};

	class protocol
	{
	public:
		static protocol* getInstance();
		static void delInstance();
		//[totalLen:4][msgId:4][command:4][DataLen:4][Data:X]
		int makeAPIRequest(ENUM_API_COMMAND cmd, void* buf, unsigned int size, std::string& request);
		
		int parseRequest(const char* reqbuf, unsigned int bufLen, unsigned int& msgId, int& cmd, void*& cmdData, unsigned int& cmdDataLen);

		//[totalLen:4][msgId:4][command:4][result:4][dataLen:4][Data:x]
		int makeAPIResponse(unsigned int msgId, ENUM_API_COMMAND cmd, void* data, unsigned int dataLen, std::string& resp);
		int parseResponse(const char* respbuf, unsigned int bufLen, unsigned int& msgId, int& cmd, int& result, void*& cmdData, unsigned int& cmdDataLen);

		int makeErrResponse(unsigned int msgId, ENUM_API_COMMAND cmd, ENUM_PROTOCOL_ERROR errCode, std::string& resp);

	private:
		static protocol* m_instance;
		static unsigned int m_msgId;
		static std::mutex m_mutex;
		protocol() {};
		~protocol() {};
	};
}
