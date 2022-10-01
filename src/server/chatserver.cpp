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

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg, int idleSeconds)
    : server_(loop,listenAddr,nameArg)
    , loop_(loop)
    , idleSeconds_(idleSeconds)
    , connectionBuckets_(idleSeconds){
    // 注册链接回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));

    // 注册消息回调
    server_.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

    // 设置连接超时定时器
    loop->runEvery(1.0,std::bind(&ChatServer::onTimer,this));
    connectionBuckets_.resize(idleSeconds);

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

    if (conn->connected()) {
        // 创建一个元素
        EntryPtr entry(new Entry(conn));
        // 插入到最后一个桶中 引用计数+1
        connectionBuckets_.back().insert(entry);
        // 存放到TcpConnection中
        WeakEntryPtr weakEntryPtr(entry);
        conn->setContext(weakEntryPtr);
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

    // 获取TcpConnection中保存的Entry
    WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
    EntryPtr entry(weakEntry.lock());
    if (entry) {
        // 引用计数再次+1 即使前面的出去了也不会析构
        connectionBuckets_.back().insert(entry);
    }
}

void ChatServer::onTimer() {
    // 添加一个空白的 目的是为了将最前面的一个移出去
    connectionBuckets_.push_back(Bucket());
}