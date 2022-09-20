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

int main() {

    signal(SIGINT,restHandler);

    EventLoop loop;
    InetAddress addr("127.0.0.1",8000);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop();

    return 0;
}