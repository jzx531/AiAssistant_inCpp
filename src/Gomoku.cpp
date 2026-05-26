#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include "../AIApps/GomokuServer/include/GomokuServer.h"


const std::string RABBITMQ_HOST = "localhost";
const int THREAD_NUM = 3;

int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << getpid();
	std::string serverName = "GomokuServer";
    int port = 80;

    int opt;

    const char* str = "p:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
    muduo::Logger::setLogLevel(muduo::Logger::WARN);
    GomokuServer server(port, serverName);
    server.setThreadNum(THREAD_NUM);

    std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for the server to start


    server.start();
}