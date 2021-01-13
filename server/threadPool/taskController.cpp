 #include "taskController.h"
#include <sys/socket.h>
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
    std::list<int>* sockets = tc->getSendSocketList();
    std::mutex cond_mtx;
    std::unique_lock<std::mutex> lck(cond_mtx);
    while(true)
    {
        bool hasData = false;
        taskData* data = NULL;
        tc_wtmtx.lock();
        if( datas->size() > 0 && sockets->size() > 0)
        {
            int socket = sockets->front();
            sockets->pop_front();
            std::list<taskData*>* datalist = &(*datas)[socket];
            if(datalist->size() > 0)
            {
                data = datalist->front();
                datalist->pop_front();
            }
            tc_wtmtx.unlock();
            //send the data
            if(data)
            {
                int len = send(socket, data->m_databuf, data->m_len, 0);
                //SECLOG(secsdk::INFO)<< "send to the client: " << socket << " datalen: " << len << "   data:" << data->m_databuf;
                delete data;
                hasData = true;
            }
        }
        else
        {
            tc_wtmtx.unlock();
        }
        if(!hasData)
        {
            if(tc->getWTCondition()->wait_for(lck, std::chrono::seconds(THREAD_CHECK_INTERVAL)) == std::cv_status::timeout)
            {
                //SECLOG(secsdk::INFO) << "condition wait time out";
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
    std::list<int>* sockets = tc->getRecvSocketList();
    std::mutex cond_mtx;
    std::unique_lock<std::mutex> lck(cond_mtx);
    char* buf[4096] = {0};
    while(true)
    {
        bool hasData = false;
        int socket = -1;
        tc_rdmtx.lock();
        if(sockets->size() > 0)
        {
            socket = sockets->front();
            sockets->pop_front();
            hasData = true;
        }
        tc_rdmtx.unlock();
        //read
        if(socket != -1)
        {
            while( true )
            {
                memset(buf, 0, 4096);
                int revLen = recv(socket, buf, 4096, 0);
                if(revLen <= 0)
                {
                    //SECLOG(secsdk::ERROR) << " recv data from client with errno: " << errno;
                    break;
                }
                else
                {
                    //SECLOG(secsdk::INFO) << "recv data: " << (char*)buf  << "   len:" << revLen;
                    tc->makeNewTask(socket, (char*)buf, revLen);
                }
            }
        }
        if(!hasData)
        {
            if(tc->getRDCondition()->wait_for(lck, std::chrono::seconds(THREAD_CHECK_INTERVAL)) == std::cv_status::timeout)
            {
                //SECLOG(secsdk::INFO) << "condition wait time out";
            }
        }
    }
}

taskController::taskController()
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
    SECLOG(secsdk::INFO) << "called";
    taskData* data = (taskData*)param;
    usleep(800000);
    tc_wtmtx.lock();
    m_taskDatas[data->m_clientId].push_back(data);
    tc_wtmtx.unlock();
    SECLOG(secsdk::INFO) << "will leave";
}

void taskController::makeNewTask(int clientID, char* buf, unsigned int len)
{
    char* revBuf = new char[len+1];
    memcpy(revBuf, buf, len);
    taskData* data = new taskData(clientID, revBuf, len);
    std::shared_ptr<threadTask> taskdata = std::make_shared<threadTask>(this, (void*)data);
    threadPool::getInstance()->addTask(taskdata);
}