#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <sys/stat.h>
//#include <netinet/in.h>
#include <unistd.h>
//#include <arpa/inet.h>
#include <fcntl.h>

#include "./threadPool/taskController.h"
#include "seclog.h"

#define SERVER_SOCKET "/tmp/socket_wg_tde_server"

int main()
{
    const char* name = "Server";
//    google::InitGoogleLogging(name);
    google::SetStderrLogging(google::INFO);

    WG::taskController tc;
    SECLOG(secsdk::INFO) << "This is server";
    // socket
    int listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenfd == -1) {
        SECLOG(secsdk::ERROR) << "Error: socket";
        return -1;
    }
    // bind
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_SOCKET);
    //addr.sun_path[0] = 0;
    size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);
    unlink(SERVER_SOCKET);

    if (bind(listenfd, (struct sockaddr*)&addr, size) == -1) {
        SECLOG(secsdk::ERROR) << "Error: bind";
        return -1;
    }
    // listen
    if(listen(listenfd, 5) == -1) {
        SECLOG(secsdk::ERROR) << "Error: listen";
        return -1;
    }
    
    int epfd = epoll_create(1024);//2.6.8 later,  the size is uselsss, and managed by the system
    if(epfd == -1)
    {
        SECLOG(secsdk::ERROR) << "epoll create failed";
        return -1;
    }
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if( -1 == epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev))
    {
        SECLOG(secsdk::ERROR) << "epoll socket failed";
        return -1;
    }
    epoll_event events[1024] = {};
    char buf[4096] = {0};
    size_t revLen = 0;
    while (true) {
        //SECLOG(secsdk::INFO) << "...listening";
        int ret = epoll_wait(epfd, events, 1024, -1);
        if(ret < 0)
        {
            SECLOG(secsdk::ERROR) << "error while epoll wait with errno: " << errno;
            break;
        }

        for(int i = 0; i < ret; ++i)
        {
            //std::cout << "epoll wait ret :" << ret << std::endl;
            //std::cout << "event: " << events[i].data.fd << "   ev:" << events[i].events << std::endl;
            if(events[i].data.fd == listenfd)
            {
                // accept
                struct sockaddr_un  clientAddr;
                struct stat         statbuf;
                unsigned int len = sizeof(clientAddr);
                int conn = accept(listenfd, (struct sockaddr *)&clientAddr, &len);
                if(conn < 0)
                {
                    SECLOG(secsdk::ERROR) << "Error : accept";
                    continue;
                }

                //set non-blocking
                int flags = fcntl(conn, F_GETFL, 0);
                int ret = fcntl(conn, F_SETFL, flags|O_NONBLOCK);

                SECLOG(secsdk::INFO) << "...connect  client: " << conn;
                epoll_event ev;
                ev.events = EPOLLIN | EPOLLOUT;
                ev.data.fd = conn;
                if( -1 == epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev))
                {
                    SECLOG(secsdk::ERROR) << "epoll socket failed";
                    close(conn);
                }
            }
            else
            {
                if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))
                {
                    int clientSocket = events[i].data.fd;
                    SECLOG(secsdk::INFO) << "client will close with err. client: " << clientSocket;
                    //remove from clntConn list
                    close(clientSocket);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientSocket, NULL);
                    continue;
                }
                if(events[i].events & EPOLLIN)
                {
                    //data comes
                    int clientSocket = events[i].data.fd;
                    //SECLOG(secsdk::INFO) << "get epoll in form the socketfd: " << clientSocket;
                    tc.getRecvSocketList()->push_back(clientSocket);
                    tc.getRDCondition()->notify_all();
                    /*memset(buf, 0, 4096);
                    revLen = recv(clientsocket, buf, 4096, 0);
                    if(revLen <= 0)
                    {
                        std::cout << " recv data from client with errno: " << errno << std::endl;
                    }
                    else
                    {
                        buf[revLen] = '\0';
                        //std::cout << "....recv data: " << buf  << "   len:" << revLen << std::endl;
                        }*/
                }
                if(events[i].events & EPOLLOUT)
                {
                    int clientSocket = events[i].data.fd;
                    //SECLOG(secsdk::INFO) << "get the out epoll: " << clientSocket;
                    tc.getSendSocketList()->push_back(clientSocket);
                    tc.getWTCondition()->notify_all();
                    /*int len = send(clientsocket, buf, revLen, 0);
                      std::cout << "send to the client: " << len << std::endl;*/
                }
            }
        }
    }
    close(epfd);
    close(listenfd);
    return 0;
}