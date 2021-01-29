#include <iostream>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <thread>
#include "socketClient.h"
#include "protocol.h"

#include "seclog.h"
#include "hexChar.h"

using namespace WG;
using namespace wg_protocol;

void getKey();
void uploadData();

void createClient(int index)
{
	socketClient so("127.0.0.1", 8080);
	std::string response;
	std::string req;
	
	//API
	char* cmddata = new char[1024 * 1024];
	memset(cmddata, 'A', 1024 * 1024);
	if (0 == protocol::getInstance()->makeAPIRequest(EAC_API1, cmddata, 1024*1024, req))
	{
		so.doConnect(req, response);
		unsigned int msgId = 0;
		int cmd = 0;
		void* cmdData = NULL;
		unsigned int cmdDataLen = 0;
		int result = -1;
		if (0 == protocol::getInstance()->parseResponse(response.data(), response.size(), msgId, cmd, result, cmdData, cmdDataLen))
		{
			std::string returnData((char*)cmdData, cmdDataLen);
			SECLOG(secsdk::INFO) << "api1 result: " << result << "  with Data: " << returnData;
		}
		else
		{
			SECLOG(secsdk::ERROR) << " parse api1 result failed ";
		}
	}
}

int main()
{
    const char* name = "Client";
    google::InitGoogleLogging(name);
    google::SetStderrLogging(google::INFO);

    std::thread runth[100];
    for(int i = 0; i<50; ++i)
   	{
        runth[i] = std::thread(createClient,i);
        //runth[i].detach();
	}

    for(int i=0; i<50; ++i)
    {
        runth[i].join();
    }
    //getchar();
    return 0;
}
