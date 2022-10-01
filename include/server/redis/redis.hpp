//
// Created by gnezd on 2022-09-21.
//

#ifndef CHAT_REDIS_HPP
#define CHAT_REDIS_HPP

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;

class Redis {
public:
    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向redis指定的通道channel发布消息
    bool publish(int channel, string message);

    // 向redis指定的通道订阅消息
    bool subscribe(int channel);

    // 向redis指定的通道取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的消息
    void observerChannelMessage();

    // 获取序列号 一对一 seq:fromid+to+toid
    // 群聊 seq:group+id
    int getSeq(string channel);

    // ack:fromid+to+toid
    // ack:group+id+to+toid
    int getAck(string channel);

    bool setAck(string channel,int ack);

    // 初始化向业务层上报通道消息的回调对象
    void initNotifyHandler(function<void(int,string)> fn);

private:
    // 需要两个 因为subscribe会阻塞
    //hiredis同步上下文对象 负责publish消息 相当于一个redis客户端
    redisContext *publishContext_;
    // subscribe消息
    redisContext *subcribeContext_;

    // 序列号
    redisContext *seqContext_;

    // 回调操作 收到订阅的消息 给service层上报
    function<void(int,string)> notifyMessageHandler_;
};

#endif //CHAT_REDIS_HPP
