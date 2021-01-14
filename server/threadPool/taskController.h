#pragma once

#include "threadpool.h"
#include <unordered_map>
#include <list>
#include <condition_variable>

namespace WG{
    class taskData
    {
    public:
        taskData(int clientId, char* databuf, unsigned int len):m_clientId(clientId),m_databuf(databuf),m_len(len){};
        virtual ~taskData() {if(m_databuf){ delete m_databuf; m_databuf=NULL;}}
        int m_clientId;
        char* m_databuf;
        unsigned int m_len;
    };
    class taskController:public threadTaskWorker
    {
    public:
        taskController(int epollfd, int socket);
        virtual ~taskController();
        virtual void onTaskProcessor(void* param) override;
        void makeNewTask(int clientID, char* buf, unsigned int len);
        //std::condition_variable* getRDCondition(){return &m_rdCond;};
        //std::condition_variable* getWTCondition(){return &m_wtCond;};
        //std::list<int>* getRecvSocketList() {return &m_recvSocketList;};
        //std::list<int>* getSendSocketList() {return &m_sendSocketList;};
        //void setEpollFD(int epollfd){m_epollfd = epollfd;};
        int getEpollFD(){return m_epollfd;};
        //void setSocket(int socket){m_socket = socket;};
        int getSocket(){return m_socket;};
        std::unordered_map<int,std::list<taskData*>>* getTaskList() {return &m_taskDatas;};
    private:
        int m_epollfd;
        int m_socket;
        //std::list<int> m_recvSocketList;
        //std::list<int> m_sendSocketList;
        //std::condition_variable m_rdCond;
        //std::condition_variable m_wtCond;
        std::unordered_map<int,std::list<taskData*>> m_taskDatas;

        std::shared_ptr<std::thread> m_sendThread;
        std::shared_ptr<std::thread> m_recvThread;
    };
}