#pragma once
#include <thread>
#include <queue>
#include <list>
#include <atomic>

namespace WG
{
    //class who do something in the thread need inhert this class.
    class threadTaskWorker
    {
    public:
        virtual ~threadTaskWorker(){};
        virtual void onTaskProcessor(void* param) = 0;
    };

    //managers hold the list of task and send to the thread which is free.
    class threadTask
    {
    public:
        threadTask(threadTaskWorker* worker, void* param):m_worker(worker), m_param(param){};
        virtual ~threadTask(){};
        threadTaskWorker* m_worker;
        void* m_param;
    };

    class wgThread
    {
    public:
        wgThread():m_pthread(NULL),m_state(false){};
        std::shared_ptr<std::thread> m_pthread;
        std::atomic<short> m_state;//0-will create, 1-idle/ready, 2-busy
    };

    class threadPool
    {
    public:
        static threadPool* getInstance();
        static void delInstance();
        int initTaskPool(int maxNum, int minNum=2);
        int addTask(std::shared_ptr<threadTask> task);
        int releaseTaskThread(std::thread::id threadid);
        int releaseTaskPool();
        //void setTaskWorker(const threadTaskWorker* worker);
        std::atomic<bool> m_tpState;
        std::queue<std::shared_ptr<threadTask>> m_tasks; //the task queue

    private:
        threadPool(){};
        virtual ~threadPool(){};
        threadPool(threadPool& instance){};
        threadPool& operator=(const threadPool& instance){return *this;};

        int createNewThread();
        unsigned int m_maxNum; //the max number of thread will be created
        unsigned int m_minNum;
        std::list<wgThread*> m_threads; //store the state of each thread
        //threadTaskWorker* m_threadWorker;
        static threadPool* m_instance;
    };
}