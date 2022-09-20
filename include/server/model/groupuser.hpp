//
// Created by gnezd on 2022-09-20.
//

#ifndef CHAT_GROUPUSER_HPP
#define CHAT_GROUPUSER_HPP

#include "user.hpp"

// 群组用户
class GroupUser : public User {
public:
    void setRole(string role) {role_ = role;}
    string getRole() const {return role_;}

private:
    string role_;
};

#endif //CHAT_GROUPUSER_HPP
