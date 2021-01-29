#pragma once

#include "socketBase.h"

namespace WG
{
	class socketIPv6 :public socketBase
	{
	public:
		int socket() override;
		virtual int bind(std::string addr) override { return -1; };
		virtual int bind(unsigned short port) override;
		virtual int connect(int socket, std::string addr, unsigned int port) override;
	};
}