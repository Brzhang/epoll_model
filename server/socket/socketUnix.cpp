#include "socketUnix.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

using namespace WG;

int socketUnix::socket()
{
	return m_socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
}

int socketUnix::bind(std::string addr)
{
	if (m_socket > 0)
	{
		struct sockaddr_un unaddr;
		memset(&unaddr, 0, sizeof(sockaddr_un));
		unaddr.sun_family = AF_UNIX;
		strcpy(unaddr.sun_path, addr.c_str());
		size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(unaddr.sun_path);
		unlink(unaddr.sun_path);
		return ::bind(m_socket, (struct sockaddr*)&unaddr, size);
	}
	return -1;
}

int socketUnix::connect(int socket, std::string addr, unsigned int port)
{
	if (socket > 0)
	{
		struct sockaddr_un serverAddr;
		memset(&serverAddr, 0, sizeof(sockaddr_un));
		serverAddr.sun_family = AF_UNIX;
		strcpy(serverAddr.sun_path, addr.c_str());
		size_t size = offsetof(struct sockaddr_un, sun_path) + strlen(serverAddr.sun_path);
		return ::connect(socket, (struct sockaddr*)&serverAddr, size);
	}
	return -1;
}
