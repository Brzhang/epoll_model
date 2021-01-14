#include "threadpool.h"
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include "../seclog.h"

using namespace WG;

std::mutex mtx;
std::condition_variable cond;
threadPool* threadPool::m_instance = NULL;

const unsigned int THREAD_CHECK_INTERVAL = 10; //10s
const unsigned int THREAD_IDLE_TIME = 30;//30s
unsigned int max_checkTimes = THREAD_IDLE_TIME/THREAD_CHECK_INTERVAL;

void threadfun(void* param, std::atomic<short>* state)
{
    *state = 1;
    SECLOG(secsdk::INFO) << "into thread pool function";
    threadPool* pool = (threadPool*)param;
    if(!pool)
    {
        SECLOG(secsdk::ERROR) << "pool object is NULL";
        return;
    }
    unsigned short ifreeTime = 0;
    std::mutex cond_mtx;
    std::unique_lock<std::mutex> lck(cond_mtx);
    while(pool->m_tpState)// && ifreeTime < max_checkTimes)
    {
        //check task
        //SECLOG(secsdk::INFO) << "into loop";
        std::shared_ptr<threadTask> task = NULL;
        mtx.lock();
        if(pool->m_tasks.size() > 0)
        {
            task = pool->m_tasks.front();
            SECLOG(secsdk::INFO) << "m_tasks.size() before pop " << pool->m_tasks.size();
            //SECLOG(secsdk::INFO) << "task->m_worker: "<< &(task->m_worker) << "    task->param: "<< &(task->m_param);
            pool->m_tasks.pop();
            mtx.unlock();
            ifreeTime = 0;
        }
        else
        {
            mtx.unlock();
            ++ifreeTime;
        }

        if(task && task->m_worker)
        {
            *state = 2;
            task->m_worker->onTaskProcessor(task->m_param);
        }
        else if(ifreeTime == 0)
        {
            //log task error
            SECLOG(secsdk::ERROR) << "task is null or task->worker is null";
        }
        *state = 1;
        if(ifreeTime == 0)
        {
            continue;
        }
        if(cond.wait_for(lck, std::chrono::seconds(THREAD_CHECK_INTERVAL)) == std::cv_status::timeout)
        {
            //SECLOG(secsdk::INFO) << "condition wait time out";
        }
    }
    if(ifreeTime >= max_checkTimes)
    {
        pool->releaseTaskThread(std::this_thread::get_id());
    }
    SECLOG(secsdk::INFO) << "will leave thread function";
}

threadPool* threadPool::getInstance()
{
    mtx.lock();
    if(m_instance == NULL)
    {
        m_instance = new threadPool();
    }
    mtx.unlock();
    return m_instance;
}

void threadPool::delInstance()
{
    mtx.lock();
    if(m_instance != NULL)
    {
        delete m_instance;
        m_instance = NULL;
    }
    mtx.unlock();
}

int threadPool::initTaskPool(int maxNum, int minNum)
{
    mtx.lock();
    m_maxNum = maxNum;
    m_minNum = minNum;
    m_tpState = true;
    unsigned short tsize = minNum - m_threads.size();
    if(tsize > 0)
    {
        for(int i = 0; i < tsize; ++i)
        {
            createNewThread();
        }
    }
    mtx.unlock();
    return 0;
}

int threadPool::addTask(std::shared_ptr<threadTask> task)
{
    mtx.lock();
    //SECLOG(secsdk::INFO) << "task added";
    m_tasks.push(task);
    if(m_threads.size() < m_maxNum)
    {
        bool isPoolBusy = true;
        for(wgThread* th : m_threads)
        {
            if(th->m_state == 1)
            {
                isPoolBusy = false;
                SECLOG(secsdk::INFO) << "find idle thread, will not create new thread";
                break;
            }
        }
        if(isPoolBusy)
        {
            SECLOG(secsdk::INFO) << "will create new thread";
            createNewThread();
        }
    }
    mtx.unlock();
    cond.notify_all();
    return 0;
}

int threadPool::releaseTaskThread(std::thread::id threadid)
{
    mtx.lock();
    if(m_threads.size() <= m_minNum)
    {
        mtx.unlock();
        return 1;
    }
    SECLOG(secsdk::INFO) << "we have idle thread will be destoried";
    std::list<wgThread*>::iterator iter = m_threads.begin();
    while(iter != m_threads.end())
    {
        wgThread* th = (*iter);
        SECLOG(secsdk::INFO) << "iter is " << th << " iter->m_pthread: " << th->m_pthread;
        if(th->m_pthread->get_id() == threadid)
        {
            SECLOG(secsdk::INFO)<< "find the thread id";
            m_threads.erase(iter);
            delete th;
            break;
        }
        ++iter;
    }
    mtx.unlock();
    return 0;
}

int threadPool::releaseTaskPool()
{
    mtx.lock();
    if(m_tasks.size() > 0)
    {
        mtx.unlock();
        return 1;
    }
    if(m_tpState == false)
    {
        mtx.unlock();
        return 0;
    }
    m_tpState = false;
    for(wgThread* th : m_threads)
    {
        if( th->m_pthread->joinable())
        {
            th->m_pthread->join();
        }
        delete th;
        th == NULL;
    }
    m_threads.clear();
    mtx.unlock();
    return 2;
}

int threadPool::createNewThread()
{
    SECLOG(secsdk::INFO) << "will create new thread";
    wgThread* worker = new wgThread();
    if(worker)
    {
        worker->m_state = 0;
        worker->m_pthread = std::make_shared<std::thread>(threadfun,(void*)this, &(worker->m_state));
        if(worker->m_pthread)
        {
            int tryTimes = 0;
            while(worker->m_state == 0 && ++tryTimes < 11)
            {
                usleep(100000);
            }
            SECLOG(secsdk::INFO) << "new thread create successfull";
            m_threads.push_back(worker);
            return 0;
        }
    }
    return -1;
}


/*void threadPool::setTaskWorker(const threadTaskWorker* worker)
{
    mtx.lock();
    m_threadWorker = worker;
    mtx.unlock();
    }*/
