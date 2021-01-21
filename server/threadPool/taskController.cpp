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
    //std::list<int>* sockets = tc->getSendSocketList();
    epoll_event events[1024] = {};
    int epfd = tc->getEpollFD();
    int listenfd = tc->getSocket();
	std::unordered_map<int, std::list<taskData*>>* datas = tc->getTaskList();
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
                    tc_wtmtx.lock();
                    if( datas->size() > 0 )
                    {
						taskData* data = NULL;
                        std::list<taskData*>* datalist = &((*datas)[clientSocket]);
                        if(datalist && datalist->size() > 0)
                        {
                            data = datalist->front();
                            datalist->pop_front();
                        }
                        tc_wtmtx.unlock();
                        //send the data
                        if(data)
                        {
							SECLOG(secsdk::INFO) << "before send " << clientSocket;
                            int len = send(clientSocket, data->m_databuf, data->m_len, MSG_NOSIGNAL);
							SECLOG(secsdk::INFO) << "end send: " << clientSocket;
                            //SECLOG(secsdk::INFO)<< "send to the client: " << socket << " datalen: " << len << "   data:" << data->m_databuf;
                            delete data;
                        }
						/*tc_wtmtx.lock();
						if (datalist->size() == 0)
						{
							datas->erase(clientSocket);
							SECLOG(secsdk::INFO) << "send erase clientsocket " << clientSocket;
						}
						tc_wtmtx.unlock();*/
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
    unsigned int bufSize = 1048576;
    char* buf = new char[bufSize];
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
                    memset(buf, 0, bufSize);
                    int revLen = recv(clientSocket, buf, bufSize, 0);
                    if(revLen <= 0)
                    {
                        SECLOG(secsdk::ERROR) << " recv data from client with errno: " << errno;
                        //break;
                    }
                    else
                    {
                        SECLOG(secsdk::INFO) << "recv data len:" << revLen;
                        tc->makeNewTask(clientSocket, (char*)buf, revLen);
                    }
                }
            }
        }
    }
    if(buf)
    {
        delete[] buf;
    }
}

taskController::taskController(int epollfd, int socket):m_epollfd(epollfd), m_socket(socket)
{
    threadPool::getInstance()->initTaskPool(8,2);
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
    //usleep(80000);
    tc_wtmtx.lock();
    m_taskDatas[data->m_clientId].push_back(data);
    tc_wtmtx.unlock();
    //SECLOG(secsdk::INFO) << "will leave";
}

void taskController::makeNewTask(int clientID, char* buf, unsigned int len)
{
    //SECLOG(secsdk::INFO) <<  "will add new task with bufSize: " << len;
    char* revBuf = new char[len+1];
    if(!revBuf)
    {
        SECLOG(secsdk::ERROR) << "make new task failed while malloc momery";
        return;
    }
    memcpy(revBuf, buf, len);
    taskData* data = new taskData(clientID, revBuf, len);
    std::shared_ptr<threadTask> taskdata = std::make_shared<threadTask>(this, (void*)data);
    threadPool::getInstance()->addTask(taskdata);
}
