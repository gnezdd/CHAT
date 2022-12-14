//
// Created by gnezd on 2022-09-20.
//

#include "chatserver.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <signal.h>

using namespace std;

// 处理服务器ctrl+c结束后重置user的状态信息
void restHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv) {

    if (argc < 3) {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT,restHandler);

    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"ChatServer",10);

    server.start();
    loop.loop();

    return 0;
}