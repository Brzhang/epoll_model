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

#include "socketClient.h"
#include "../server/seclog.h"

const int SOCKETBUFFER_SIZE = 4096;
#define CLIENT_SOCKET "/tmp/socket_wg_tde_server"

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
        std::cout << "Error: socket" << std::endl;
        return -1;
    }
    // connect
    struct sockaddr_un serverAddr;
    serverAddr.sun_family = AF_UNIX;
    strcpy(serverAddr.sun_path,CLIENT_SOCKET);
    //serverAddr.sun_path[0] = 0;
    size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(serverAddr.sun_path);
    /*if(bind(client, (struct sockaddr*)&serverAddr, size)==-1){
        std::cout << "bind failed" << std::endl;
        return -1;
        }*/
    if (connect(client, (struct sockaddr*)&serverAddr,size) < 0) {
        std::cout << "Error: connect with errno: " << errno << std::endl;
        return -1;
    }

    //set non-blocking
    int flags = fcntl(client, F_GETFL, 0);
    int ret = fcntl(client, F_SETFL, flags|O_NONBLOCK);

    //std::cout << "...connect" << std::endl;
    char buf[SOCKETBUFFER_SIZE] = {0};
    static int cmdid = 1;
    ++cmdid;

    std::string request(req);
    request =  request;// + std::to_string(cmdid);

    if(0 >= send(client, (void*)request.c_str(), request.size(), MSG_NOSIGNAL))
    {
        std::cout << "send msg error with errno: " << errno << std::endl;
        close(client);
        return -1;
    }
    //std::cout << "send to the server success: " << request << std::endl;
    while(true)
    {
        int len = recv(client, buf, SOCKETBUFFER_SIZE, 0);
        if(len <=0 && resp.size() == request.size())
        {
            close(client);
            SECLOG(secsdk::INFO) << "client closed";
            break;
            }
        if(len <= 0)
        {
            //SECLOG(secsdk::INFO) << "client recv errno: " << errno << "   errno:" << EWOULDBLOCK<<" "<<EAGAIN ;
            if (EWOULDBLOCK == errno || EAGAIN == errno)
            {
                usleep(50000);
                continue;
            }
            else
            {
                close(client);
                break;
            }
        }
        resp.append(buf, len);
        //SECLOG(secsdk::INFO) << "get resp:" << resp;
        //std::cout << "recv from server: len: " << len << std::endl;
    }
    //SECLOG(secsdk::INFO) << "close, resp:" << resp;
    close(client);
    return 0;
}
