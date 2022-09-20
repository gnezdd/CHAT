//
// Created by gnezd on 2022-09-20.
//

#ifndef CHAT_FRIENDMODEL_HPP
#define CHAT_FRIENDMODEL_HPP

#include <vector>

#include "user.hpp"

using namespace std;

// 维护好友信息的操作接口
class FriendModel {
public:
    // 添加好友列表
    void insert(int userid,int friendid);

    // 返回用户好友列表
    vector<User> query(int userid);

};

#endif //CHAT_FRIENDMODEL_HPP
