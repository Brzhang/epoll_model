#include "taskController.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <mutex>
#include <cstring>
#include "../seclog.h"

using namespace WG;
std::mutex tc_rdmtx;
std::mutex tc_wtmtx;
const int THREAD_CHECK_INTERVAL = 10; //10s
void threadSendFun(void* controller)
{
    taskController* tc = (taskController*)controller;
    if(!tc)
    {
        return;
    }
    std::unordered_map<int,std::list<taskData*>>* datas = tc->getTaskList();
    //std::list<int>* sockets = tc->getSendSocketList();
    epoll_event events[1024] = {};
    int epfd = tc->getEpollFD();
    int listenfd = tc->getSocket();
    while(true)
    {
        int ret = epoll_wait(epfd, events, 1024, -1);
        if(ret < 0)
        {
            SECLOG(secsdk::ERROR) << "error while epoll wait with errno: " << errno << "  epfd: " << epfd;
            break;
        }

        for(int i = 0; i < ret; ++i)
        {
            if(events[i].data.fd != listenfd )
            {
                if(events[i].events & EPOLLOUT)
                {
                    int clientSocket = events[i].data.fd;
                    taskData* data = NULL;
                    tc_wtmtx.lock();
                    if( datas->size() > 0 )
                    {
                        std::list<taskData*>* datalist = &(*datas)[clientSocket];
                        if(datalist->size() > 0)
                        {
                            data = datalist->front();
                            datalist->pop_front();
                        }
                        tc_wtmtx.unlock();
                        //send the data
                        if(data)
                        {
                            int len = send(clientSocket, data->m_databuf, data->m_len, MSG_NOSIGNAL);
                            //SECLOG(secsdk::INFO)<< "send to the client: " << socket << " datalen: " << len << "   data:" << data->m_databuf;
                            delete data;
                        }
                    }
                    else
                    {
                        tc_wtmtx.unlock();
                    }
                }
            }
        }
    }
}

void threadRecvFun(void* controller)
{
    taskController* tc = (taskController*)controller;
    if(!tc)
    {
        return;
    }
    char* buf[4096] = {0};
    epoll_event events[1024] = {};
    int epfd = tc->getEpollFD();
    int listenfd = tc->getSocket();
    while(true)
    {
        int ret = epoll_wait(epfd, events, 1024, -1);
        if(ret < 0)
        {
            SECLOG(secsdk::ERROR) << "error while epoll wait with errno: " << errno;
            break;
        }

        for(int i = 0; i < ret; ++i)
        {
            if(events[i].data.fd != listenfd )
            {
                if(events[i].events & EPOLLIN)
                {
                    //data comes
                    int clientSocket = events[i].data.fd;
                    memset(buf, 0, 4096);
                    int revLen = recv(clientSocket, buf, 4096, 0);
                    if(revLen <= 0)
                    {
                        //SECLOG(secsdk::ERROR) << " recv data from client with errno: " << errno;
                        //break;
                    }
                    else
                    {
                        //SECLOG(secsdk::INFO) << "recv data: " << (char*)buf  << "   len:" << revLen;
                        tc->makeNewTask(clientSocket, (char*)buf, revLen);
                    }
                }
            }
        }
    }
}

taskController::taskController(int epollfd, int socket):m_epollfd(epollfd), m_socket(socket)
{
    threadPool::getInstance()->initTaskPool(8,8);
    m_sendThread = std::make_shared<std::thread>(threadSendFun,(void*)this);
    m_recvThread = std::make_shared<std::thread>(threadRecvFun,(void*)this);
}

taskController::~taskController()
{
    threadPool::delInstance();
    if(m_sendThread && m_sendThread->joinable())
    {
        m_sendThread->join();
    }
    if(m_recvThread && m_recvThread->joinable())
    {
        m_recvThread->join();
    }
    google::ShutdownGoogleLogging();
}

void taskController::onTaskProcessor(void* param)
{
    //SECLOG(secsdk::INFO) << "called";
    taskData* data = (taskData*)param;
    usleep(80000);
    tc_wtmtx.lock();
    m_taskDatas[data->m_clientId].push_back(data);
    tc_wtmtx.unlock();
    //SECLOG(secsdk::INFO) << "will leave";
}

void taskController::makeNewTask(int clientID, char* buf, unsigned int len)
{
    char* revBuf = new char[len+1];
    memcpy(revBuf, buf, len);
    taskData* data = new taskData(clientID, revBuf, len);
    std::shared_ptr<threadTask> taskdata = std::make_shared<threadTask>(this, (void*)data);
    threadPool::getInstance()->addTask(taskdata);
}