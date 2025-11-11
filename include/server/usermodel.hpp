#ifndef __USERMODEL_H__
#define __USERMODEL_H__

#include "user.hpp"

// User表的数据操作类
class UserModel {
public:
    // User表的增加操作
    bool insert(User &user);

};

#endif // __USERMODEL_H__