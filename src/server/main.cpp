//
// Created by gnezd on 2022-09-20.
//

#include "chatserver.hpp"

#include <iostream>

using namespace std;

int main() {
    EventLoop loop;
    InetAddress addr("127.0.0.1",8000);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop();

    return 0;
}