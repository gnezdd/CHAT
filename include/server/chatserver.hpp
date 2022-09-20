//
// Created by gnezd on 2022-09-20.
//

#ifndef CHAT_CHATSERVER_HPP
#define CHAT_CHATSERVER_HPP

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

// 聊天服务器
class ChatServer {
public:
    // 初始化聊天服务器
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg);

    // 启动服务
    void start();
private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &);

    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &, Buffer *, Timestamp);

    TcpServer server_; // 组合的muduo库 实现服务器功能的类对象
    EventLoop *loop_; // 事件循环
};
#endif //CHAT_CHATSERVER_HPP
