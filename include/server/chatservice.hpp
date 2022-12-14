//
// Created by gnezd on 2022-09-20.
//

#ifndef CHAT_CHATSERVICE_HPP
#define CHAT_CHATSERVICE_HPP

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 消息id对应的事件回调类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

// 聊天服务器业务类 单例模式
class ChatService {
public:
    // 获取单例实例
    static ChatService* instance();
    // 登录
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 注销
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 注册
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 服务器异常业务重置方法
    void reset();

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgId);

    // redis的上报消息回调
    void handleRedisSubscribeMessage(int userid,string msg);
private:
    // 将构造函数私有化
    ChatService();

    // 消息id与业务处理handler的对应集合
    unordered_map<int,MsgHandler> msgHandlerMap_;

    // 存储在线用户的通信连接 线程安全问题 上线下线
    unordered_map<int,TcpConnectionPtr> userConnMap_;

    // 定义互斥锁保证userConnMap的线程安全
    mutex connMutex_;

    // 数据操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

    // redis操作对象
    Redis redis_;

};

#endif //CHAT_CHATSERVICE_HPP
