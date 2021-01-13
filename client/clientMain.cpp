#include <iostream>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <thread>
#include "socketClient.h"
#include "../server/seclog.h"

using namespace WG;

void createClient(int index)
{
    socketClient so("127.0.0.1" , 8080);
    std::string response;
    std::stringstream ss;
    std::string sindex;
    ss << std::dec;
    ss << std::setw(3) << std::setfill('0') << index;
    ss >> sindex;
    std::string req = "[" + sindex + "]";
    int i = 0;
    while(i<14)
    {
        req+=req;
        i++;
    }
    //SECLOG(secsdk::INFO) << "Send:"<< req;
    so.doConnect(req, response);
    //SECLOG(secsdk::INFO) << "Recv" << response;
}

int main() {
    const char* name = "Client";
    //google::InitGoogleLogging(name);
    google::SetStderrLogging(google::INFO);

    std::thread runth[1000];
    for(int i = 0; i<10; ++i)
    {
        runth[i] = std::thread(createClient,i);
    }
    getchar();
    return 0;
}
