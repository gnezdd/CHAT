//
// Created by gnezd on 2022-09-20.
//

#ifndef CHAT_GROUP_HPP
#define CHAT_GROUP_HPP

#include <string>
#include <vector>

#include "groupuser.hpp"

using namespace std;

// Group表的ORM类
class Group {
public:
    Group(int id = -1, string name = "", string desc = "")
            : id_(id)
            , name_(name)
            , desc_(desc)
    {}

    void setId(int id) {id_ = id;}
    void setName(string name) {name_ = name;}
    void setDesc(string desc) {desc_ = desc;}

    int getId() const {return id_;}
    string getName() const {return name_;}
    string getDesc() const {return desc_;}
    vector<GroupUser> &getUsers() {return users_;}

private:
    int id_;
    string name_;
    string desc_;
    vector<GroupUser> users_;
};

#endif //CHAT_GROUP_HPP
