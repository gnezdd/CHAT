//
// Created by gnezd on 2022-09-20.
//

#include "chatservice.hpp"
#include "public.hpp"

#include <muduo/base/Logging.h>
#include <string>
#include <vector>

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
    msgHandlerMap_.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    msgHandlerMap_.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    msgHandlerMap_.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    msgHandlerMap_.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});

    // 初始化redis
    if (redis_.connect()) {
        // 设置上报消息回调
        redis_.initNotifyHandler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
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

// 服务器异常业务重置
void ChatService::reset() {
    // 将onlind状态的用户置为offline
    userModel_.resetState();
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
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        } else {
            // 登录成功

            // 记录用户连接信息
            {
                lock_guard<mutex> lock(connMutex_);
                userConnMap_.insert({id,conn});
            }

            // 向redis订阅
            redis_.subscribe(id);

            // 更新用户状态信息
            user.setState("online");
            userModel_.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询用户是否有离线消息
            vector<string> msgs = offlineMsgModel_.query(id);
            if (!msgs.empty()) {
                response["offlinemsg"] = msgs;
                // 将用户所有离线消息删除
                offlineMsgModel_.remove(id);
            }

            // 查询该用户的好友信息
            vector<User> friendVec = friendModel_.query(id);
            if (!friendVec.empty()) {
                vector<string> friends;
                for (User &f : friendVec) {
                    json js;
                    js["id"] = f.getId();
                    js["name"] = f.getName();
                    js["state"] = f.getState();
                    friends.push_back(js.dump());
                }
                response["friends"] = friends;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = groupModel_.queryGroups(id);
            if (!groupuserVec.empty()) {
                vector<string> groupV;
                for (Group &group : groupuserVec) {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers()) {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        }
    } else {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
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

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(userid);
        if (it != userConnMap_.end()) {
            userConnMap_.erase(it);
        }
    }

    // 向redis取消订阅
    redis_.unsubscribe(userid);

    // 更新用户状态信息
    User user(userid,"","","offline");
    userModel_.updateState(user);
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

    redis_.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1) {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int toId = js["toid"].get<int>();

    // 在同一台服务器上登录
    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(toId);
        if (it != userConnMap_.end()) {
            // 在线 服务器主动推送消息给用户
            it->second->send(js.dump());
            return;
        }
    }

    // 在不同服务器上登录
    User user = userModel_.query(toId);
    if (user.getState() == "online") {
        redis_.publish(toId,js.dump());
        return;
    }

    // 存储离线消息
    offlineMsgModel_.insert(toId,js.dump());
}

// 添加好友
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    friendModel_.insert(userid,friendid);
}

// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1,name,desc);
    if (groupModel_.createGroup(group)) {
        groupModel_.addGroup(userid,group.getId(),"creator");
    }
}

// 加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid,groupid,"normal");
}

// 群组聊天
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    vector<int> useridVec = groupModel_.queryGroupUsers(userid,groupid);
    lock_guard<mutex> lock(connMutex_);
    for (int id : useridVec) {
        auto it = userConnMap_.find(id);
        if (it != userConnMap_.end()) {
            it->second->send(js.dump());
        } else {
            // 在其他服务器上登录
            User user = userModel_.query(id);
            if (user.getState() == "online") {
                redis_.publish(id,js.dump());
            } else {
                offlineMsgModel_.insert(id,js.dump());
            }
        }
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg) {
    lock_guard<mutex> lock(connMutex_);
    auto it = userConnMap_.find(userid);
    if (it != userConnMap_.end()) {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    offlineMsgModel_.insert(userid,msg);
}