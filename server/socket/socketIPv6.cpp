#include "socketIPv6.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace WG;

int socketIPv6::socket()
{
	return m_socket = ::socket(AF_INET6, SOCK_STREAM, 0);
}

int socketIPv6::bind(unsigned short port)
{
	if (m_socket > 0)
	{
		struct sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET6;
		serverAddr.sin_port = htons(port);
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		return ::bind(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	}
	return -1;
}

int socketIPv6::connect(int socket, std::string addr, unsigned int port)
{
	if (socket > 0)
	{
		struct sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET6;
		serverAddr.sin_port = htons(port);
		serverAddr.sin_addr.s_addr = inet_addr(addr.c_str());
		return ::connect(socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	}
	return -1;
}
