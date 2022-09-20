//
// Created by gnezd on 2022-09-20.
//

#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <functional>
#include <string>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg)
    : server_(loop,listenAddr,nameArg)
    , loop_(loop) {
    // 注册链接回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));

    // 注册消息回调
    server_.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

    // 设置线程数量
    server_.setThreadNum(4);
}

void ChatServer::start() {
    server_.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn) {
    // 客户端连接断开
    if (!conn->connected()) {
        // 客户端异常退出处理
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time) {
    // 获取用户传递过来的数据
    string buf = buffer->retrieveAllAsString();
    // 将数据反序列化
    json js = json::parse(buf);

    // 完全解耦网络模块的代码与业务模块的代码 通过回调操作
    // js["msgid"]获取业务handler=>conn js time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn, js, time);
}