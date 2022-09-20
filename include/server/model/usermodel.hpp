//
// Created by gnezd on 2022-09-20.
//

#ifndef CHAT_USERMODEL_HPP
#define CHAT_USERMODEL_HPP

#include "user.hpp"

// User表的数据操作类
class UserModel {
public:
    // User表的增加方法
    bool insert(User &user);

    // 根据用户号码查询用户信息
    User query(int id);

    // 更新用户状态信息
    bool updateState(User user);
private:
};

#endif //CHAT_USERMODEL_HPP
