#pragma once

#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <list>
#include <mutex>

namespace WG
{
    class socketListener
    {
    public:
        virtual void onRecv(int socket, void* data, unsigned int len) = 0;
	};

	class sendData
	{
	public:
		sendData(int clientId, char* databuf, unsigned int len) :m_clientId(clientId), m_databuf(databuf), m_len(len) {};
		virtual ~sendData() { if (m_databuf) { delete m_databuf; m_databuf = NULL; } }
		int m_clientId;
		char* m_databuf;
		unsigned int m_len;
	};

    class socketBase
    {
    public:
		socketBase(bool mutilThread=true);
		~socketBase();
		void setListener(socketListener* listener);
		virtual int socket() = 0;
		virtual int bind(std::string addr) = 0;
		virtual int bind(unsigned short port) = 0;
        int listen(int backlog);
		virtual int connect(int socket, std::string addr, unsigned int port) = 0;
        int send(int socket, void* buf, unsigned int len);
		int closeSocket();
        //int recv(int socket, void* buf, unsigned int len);
		//std::unordered_map<int, std::list<sendData*>>* getSendDataList() { return &m_sendDataList; };
	protected:
		int accept(struct sockaddr& addrObj);
		static void threadSendFun(void* controller);
		static void threadRecvFun(void* controller);
		static void threadListenFun(void* controller);

        socketListener* m_listener;
		bool m_mutilThread;
		int m_socket;
        int m_epfd;
		std::mutex m_wtmtx;
		std::shared_ptr<std::thread> m_listenThread;	// thread to listen and accept
		std::shared_ptr<std::thread> m_sendThread;		// thread to send data
		std::shared_ptr<std::thread> m_recvThread;		// thread to recv data
		std::unordered_map<int, std::list<sendData*>> m_sendDataList;
	};
}