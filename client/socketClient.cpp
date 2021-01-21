#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <thread>

#include "socketClient.h"
#include "../server/seclog.h"

const int SOCKETBUFFER_SIZE = 4096;
#define SERVER_SOCKET "/tmp/socket_wg_tde_server"
#define OWEN_SOCKET "/tmp/socket_wg_tde_client_"

using namespace WG;

socketClient::socketClient(const std::string& addr, const int port)
    : m_addr(addr), m_port(port)
{
}

int socketClient::doConnect(const std::string& req, std::string& resp)
{
       // socket
    int client = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client == -1) {
        SECLOG(secsdk::ERROR) << "Error: socket";
        return -1;
    }


	/* fill socket address structure with our address */
	struct sockaddr_un owenAddr;
	memset(&owenAddr, 0, sizeof(sockaddr_un));
	owenAddr.sun_family = AF_UNIX;
	sprintf(owenAddr.sun_path, "%s%05d", OWEN_SOCKET, std::this_thread::get_id());
	size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(owenAddr.sun_path);
	unlink(owenAddr.sun_path);        /* in case it already exists */
	if (bind(client, (struct sockaddr *)&owenAddr, size) < 0) {
		SECLOG(secsdk::ERROR) << "Error: bind failed with errno: " << errno;
		return -1;
	}

    // connect
    struct sockaddr_un serverAddr;
	memset(&serverAddr, 0, sizeof(sockaddr_un));
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path, SERVER_SOCKET);
    //serverAddr.sun_path[0] = 0;
	size = offsetof(struct sockaddr_un, sun_path) + strlen(serverAddr.sun_path);
    
    if (connect(client, (struct sockaddr*)&serverAddr,size) < 0) {
        SECLOG(secsdk::ERROR) << "Error: connect with errno: " << errno;
        return -1;
    }
    else
    {
        SECLOG(secsdk::INFO) << "socket connect: " << client;
    }

    //set non-blocking
    /*int flags = fcntl(client, F_GETFL, 0);
    int ret = fcntl(client, F_SETFL, flags|O_NONBLOCK);
	if (ret == -1)
	{
		SECLOG(secsdk::ERROR) << " set unblocking mode failed";
	}*/

    //std::cout << "...connect" << std::endl;
    char buf[SOCKETBUFFER_SIZE] = {0};
    static int cmdid = 1;
    ++cmdid;

    std::string request(req);
    request =  request;// + std::to_string(cmdid);
	int sendTimes = 1;
	int totolNums = 100 * 1024 * 1024/req.size()+1;
	while (sendTimes < totolNums)
	{
		if (0 >= send(client, (void*)request.c_str(), request.size(), MSG_NOSIGNAL))
		{
			SECLOG(secsdk::ERROR) << "send msg error with errno: " << errno;
			close(client);
			return -1;
		}
		SECLOG(secsdk::INFO) << "send to the server success: " << client;
		resp.clear();
		while (true)
		{
			int len = recv(client, buf, SOCKETBUFFER_SIZE, 0);
			if (len <= 0)
			{
				break;
			}
			/*if(len <= 0)
			{
				//SECLOG(secsdk::INFO) << "client recv errno: " << errno << "   errno:" << EWOULDBLOCK<<" "<<EAGAIN ;
				if (EWOULDBLOCK == errno || EAGAIN == errno)
				{
					usleep(50000);
					continue;
				}
				else
				{
					break;
				}
			}*/
			resp.append(buf, len);
			if (resp.size() == request.size())
			{
				break;
			}
			//SECLOG(secsdk::INFO) << "get resp:" << resp;
			//std::cout << "recv from server: len: " << len << std::endl;
		}
		++sendTimes;
	}
    SECLOG(secsdk::INFO) << "closed socket: " << client;
    close(client);
    return 0;
}
