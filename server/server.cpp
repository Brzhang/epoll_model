#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <vector>

#include "taskController.h"
#include "seclog.h"
#include "socketUnix.h"

#define SERVER_SOCKET "/tmp/socket_wg_tde_server"

int main()
{
    const char* name = "Server";
    google::InitGoogleLogging(name);
    google::SetStderrLogging(google::INFO);

    SECLOG(secsdk::INFO) << "This is server";
    // socket

	WG::socketUnix serverSocket;
	WG::taskController tc(&serverSocket);
	serverSocket.setListener(&tc);
	int socketfd = serverSocket.socket();
    if (socketfd == -1) {
        SECLOG(secsdk::ERROR) << "Error: socket";
        return -1;
    }

    // bind
    if (serverSocket.bind(SERVER_SOCKET) == -1) {
        SECLOG(secsdk::ERROR) << "Error: bind";
        return -1;
    }
    // listen
    if(serverSocket.listen(5) == -1) {
        SECLOG(secsdk::ERROR) << "Error: listen";
        return -1;
    }
	while (true)
	{
		sleep(10000000);
	}
    return 0;
}