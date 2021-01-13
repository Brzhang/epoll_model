#pragma once

#include <string.h>

namespace WG{
    class socketClient
    {
    public:
        socketClient(const std::string& addr, const int port);
        int doConnect(const std::string& req, std::string& resp);
    private:
        std::string m_addr;
        int m_port;
    };
};