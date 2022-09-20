//
// Created by gnezd on 2022-09-20.
//

#include "chatservice.hpp"
#include "public.hpp"

#include <muduo/base/Logging.h>
#include <string>

using namespace std;
using namespace muduo;

ChatService *ChatService::instance() {
    static ChatService service;
    return &service;
}

// 注册消息以及对应的handler回调操作
ChatService::ChatService() {
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(&ChatService::login,this,_1,_2,_3)});
    msgHandlerMap_.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    msgHandlerMap_.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
}

// 获取处理器
MsgHandler ChatService::getHandler(int msgId) {
    // 记录错误日志 msgid没有对应的事件回调
    auto it = msgHandlerMap_.find(msgId);
    if (it == msgHandlerMap_.end()) {
        // 找不到则返回一个默认的处理器 输出错误日志
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid:" << msgId << "can not find handler!";
        };
    } else {
        return msgHandlerMap_[msgId];
    }
}

// 登录
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = userModel_.query(id);
    if (user.getId() != -1 && user.getPwd() == pwd) {
        if (user.getState() == "online") {
            // 用户已经登录 不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录";
            conn->send(response.dump());
        } else {
            // 登录成功

            // 记录用户连接信息
            {
                lock_guard<mutex> lock(connMutex_);
                userConnMap_.insert({id,conn});
            }

            // 更新用户状态信息
            user.setState("online");
            userModel_.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            conn->send(response.dump());
        }
    } else {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

// 注册
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool res = userModel_.insert(user);
    if (res) {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    } else {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
    User user;
    // 找到客户的对应id
    {
        lock_guard<mutex> lock(connMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); it++) {
            if (it->second == conn) {
                user.setId(it->first);
                // 从map删除用户连接信息
                userConnMap_.erase(it);
                break;
            }
        }
    }

    // 更新用户的状态信息
    if (user.getId() != -1) {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int toId = js["to"].get<int>();

    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(toId);
        if (it != userConnMap_.end()) {
            // 在线 服务器主动推送消息给用户
            it->second->send(js.dump());
            return;
        }
    }
}