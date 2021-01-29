#include "socketBase.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "seclog.h"

using namespace WG;

const int THREAD_CHECK_INTERVAL = 10; //10s
const int EPOLL_EVENT_SIZE = 1024;
const int SOCKET_RECV_BUFFER_LEN = 32 * 1024;

void socketBase::threadListenFun(void* controller)
{
	socketBase* socket = (socketBase*)controller;
	if (!socket)
	{
		return;
	}
	epoll_event events[EPOLL_EVENT_SIZE] = {};
	while (true)
	{
		int ret = epoll_wait(socket->m_epfd, events, EPOLL_EVENT_SIZE, -1);
		if (ret < 0)
		{
			SECLOG(secsdk::ERROR) << "error while epoll wait with errno: " << errno;
			break;
		}
		for (int i = 0; i < ret; ++i)
		{
			if (events[i].data.fd == socket->m_socket)
			{
				// accept
				struct sockaddr  clientAddr;
				int clientSocket = socket->accept(clientAddr);
				if (clientSocket < 0)
				{
					SECLOG(secsdk::ERROR) << "Error : accept";
					continue;
				}

				//set non-blocking
				int flags = fcntl(clientSocket, F_GETFL, 0);
				int ret = fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

				SECLOG(secsdk::INFO) << "...connect  client: " << clientSocket;
				epoll_event ev;
				ev.events = EPOLLIN | EPOLLOUT;
				ev.data.fd = clientSocket;
				if (-1 == epoll_ctl(socket->m_epfd, EPOLL_CTL_ADD, clientSocket, &ev))
				{
					SECLOG(secsdk::ERROR) << "epoll socket failed";
					close(clientSocket);
				}
			}
			else
			{
				if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))
				{
					int clientSocket = events[i].data.fd;
					socket->m_wtmtx.lock();
					socket->m_sendDataList.erase(clientSocket);
					socket->m_wtmtx.unlock();
					SECLOG(secsdk::INFO) << "client will close with err. client: " << clientSocket;
					//remove from clntConn list
					close(clientSocket);
					epoll_ctl(socket->m_epfd, EPOLL_CTL_DEL, clientSocket, NULL);
					continue;
				}
			}
		}
	}
	SECLOG(secsdk::INFO) << "stop listening";
}

void socketBase::threadSendFun(void* controller)
{
    socketBase* socket = (socketBase*)controller;
    if(!socket)
    {
        return;
    }
    //std::list<int>* sockets = tc->getSendSocketList();
    epoll_event events[EPOLL_EVENT_SIZE] = {};
    while(true)
    {
        int ret = epoll_wait(socket->m_epfd, events, EPOLL_EVENT_SIZE, -1);
        if(ret < 0)
        {
            SECLOG(secsdk::ERROR) << "error while epoll wait with errno: " << errno << "  epfd: " << socket->m_epfd;
            break;
        }
        for(int i = 0; i < ret; ++i)
        {
            if( events[i].data.fd != socket->m_socket )
            {
                if(events[i].events & EPOLLOUT)
                {
                    int clientSocket = events[i].data.fd;
                    socket->m_wtmtx.lock();
                    if( socket->m_sendDataList.size() > 0 )
                    {
						sendData* data = NULL;
                        std::list<sendData*>* datalist = &((socket->m_sendDataList)[clientSocket]);
                        if(datalist && datalist->size() > 0)
                        {
                            data = datalist->front();
                            datalist->pop_front();
                        }
                        socket->m_wtmtx.unlock();
                        //send the data
                        if(data)
                        {
							SECLOG(secsdk::INFO) << "before send " << clientSocket << "  len:  " << data->m_len;
                            int len = ::send(clientSocket, data->m_databuf, data->m_len, MSG_NOSIGNAL);
                            //SECLOG(secsdk::INFO)<< "send to the client: " << socket << " datalen: " << len << "   data:" << data->m_databuf;
                            delete data;
                        }
                    }
                    else
                    {
                        socket->m_wtmtx.unlock();
                    }
                }
            }
        }
    }
}

void socketBase::threadRecvFun(void* controller)
{
    socketBase* socket = (socketBase*)controller;
    if(!socket)
    {
        return;
    }
    char* buf = new char[SOCKET_RECV_BUFFER_LEN];
    epoll_event events[EPOLL_EVENT_SIZE] = {};
    while(true)
    {
        int ret = epoll_wait(socket->m_epfd, events, EPOLL_EVENT_SIZE, -1);
        if(ret < 0)
        {
            SECLOG(secsdk::ERROR) << "error while epoll wait with errno: " << errno;
            break;
        }

        for(int i = 0; i < ret; ++i)
        {
            if(events[i].data.fd != socket->m_socket )
            {
                if(events[i].events & EPOLLIN)
                {
                    //data comes
                    int clientSocket = events[i].data.fd;
                    memset(buf, 0, SOCKET_RECV_BUFFER_LEN);
                    int revLen = ::recv(clientSocket, buf, SOCKET_RECV_BUFFER_LEN, 0);
                    if(revLen <= 0)
                    {
                        SECLOG(secsdk::ERROR) << " recv data from client with errno: " << errno;
                        //break;
                    }
                    else
                    {
                        SECLOG(secsdk::INFO) << "recv data len:" << revLen;
                        if(socket->m_listener)
                        {
							socket->m_listener->onRecv(clientSocket, buf, revLen);
                        }
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

//public method
socketBase::socketBase(bool mutilThread) : m_listener(NULL),m_mutilThread(mutilThread)
{
	m_socket = -1;
};

socketBase::~socketBase()
{
	m_socket = -1;
}

void socketBase::setListener(socketListener* listener)
{
	m_listener = listener;
}

int socketBase::listen(int backlog)
{
	if (m_socket > 0)
	{
		// listen
		if (::listen(m_socket, backlog) == -1) {
			SECLOG(secsdk::ERROR) << "Error: listen";
			return -1;
		}
		m_epfd = epoll_create(EPOLL_EVENT_SIZE);//2.6.8 later,  the size is uselsss, and managed by the system
		if (m_epfd == -1)
		{
			SECLOG(secsdk::ERROR) << "epoll create failed";
			return -1;
		}
		epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = m_socket;
		if (-1 == epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_socket, &ev))
		{
			SECLOG(secsdk::ERROR) << "epoll socket failed";
			return -1;
		}
		if (m_mutilThread)
		{
			m_listenThread = std::make_shared<std::thread>(socketBase::threadListenFun, (void*)this);
			m_sendThread = std::make_shared<std::thread>(socketBase::threadSendFun, (void*)this);
			m_recvThread = std::make_shared<std::thread>(socketBase::threadRecvFun, (void*)this);
		}
		return 0;
	}
	return -1;
}

int socketBase::send(int socket, void* buf, unsigned int len)
{
	//add to the send data list
	if (socket > 0)
	{
		char* sendBuf = new char[len];
		memset(sendBuf, 0, len);
		memcpy(sendBuf, buf, len);
		sendData* data = new sendData(socket, sendBuf, len);
		m_wtmtx.lock();
		m_sendDataList[socket].push_back(data);
		m_wtmtx.unlock();
	}
    return -1;
}

int socketBase::closeSocket()
{
	//remove event and close socket
	::close(m_epfd);
	::close(m_socket);
	if (m_mutilThread)
	{
		if (m_listenThread && m_listenThread->joinable())
		{
			m_listenThread->join();
		}
		if (m_sendThread && m_sendThread->joinable())
		{
			m_sendThread->join();
		}
		if (m_recvThread && m_recvThread->joinable())
		{
			m_recvThread->join();
		}
	}
	return 0;
}

/*int socketBase::recv(int socket, void* buf, unsigned int len)
{
	if (socket > 0)
	{
		return recv(m_socket, buf, len);
	}
    return -1;
}*/

//private method
int socketBase::accept(struct sockaddr& addrObj)
{
	if (m_socket > 0)
	{
		unsigned int len = sizeof(sockaddr);
		return ::accept(m_socket, (struct sockaddr*)&addrObj, &len);
	}
	return -1;
}