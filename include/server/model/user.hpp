//
// Created by gnezd on 2022-09-20.
//

#ifndef CHAT_USER_HPP
#define CHAT_USER_HPP

#include <string>

using namespace std;

// 匹配User表的ORM类
class User {
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
        : id_(id), name_(name), password_(pwd), state_(state) {
    }

    void setId (int id) {id_ = id;}
    void setName(string name) {name_ = name;}
    void setPwd(string pwd) {password_ = pwd;}
    void setState(string state) {state_ = state;}

    int getId() const {return id_;}
    string getName() const {return name_;}
    string getPwd() const {return password_;}
    string getState() const {return state_;}

private:
    int id_;
    string name_;
    string password_;
    string state_;
};

#endif //CHAT_USER_HPP
