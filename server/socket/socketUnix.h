#pragma once

#include "socketBase.h"

namespace WG
{
	class socketUnix :public socketBase
	{
	public:
		int socket() override;
		virtual int bind(std::string addr) override;
		virtual int bind(unsigned short port) override { return -1; };
		virtual int connect(int socket, std::string addr, unsigned int port) override;
	};
}