//
// Created by gnezd on 2022-09-20.
//

#ifndef CHAT_CHATSERVER_HPP
#define CHAT_CHATSERVER_HPP

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

#include <unordered_set>
#include <boost/circular_buffer.hpp>

using namespace muduo;
using namespace muduo::net;

// 聊天服务器
class ChatServer {
public:
    // 初始化聊天服务器
    ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg, int idleSeconds);

    // 启动服务
    void start();
private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &);

    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &, Buffer *, Timestamp);

    // 管理超时连接的回调函数
    void onTimer();

    TcpServer server_; // 组合的muduo库 实现服务器功能的类对象
    EventLoop *loop_; // 事件循环

    // 管理空闲连接
    using WeakTcpConnectionPtr = std::weak_ptr<TcpConnection>;
    struct Entry {
        explicit Entry(const WeakTcpConnectionPtr& weakConn) : weakConn_(weakConn) {}

        // 如果连接还存在就断开连接
        ~Entry() {
            TcpConnectionPtr conn = weakConn_.lock();
            if (conn) {
                conn->shutdown();
            }
        }

        WeakTcpConnectionPtr weakConn_;
    };

    using EntryPtr = std::shared_ptr<Entry>;
    using WeakEntryPtr = std::weak_ptr<Entry>;
    using Bucket = std::unordered_set<EntryPtr>;
    using WeakConnectionList = boost::circular_buffer<Bucket>;

    WeakConnectionList connectionBuckets_;

    int idleSeconds_;
};
#endif //CHAT_CHATSERVER_HPP
