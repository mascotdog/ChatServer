#include "chatservice.hpp"
#include "public.hpp"

#include <muduo/base/Logging.h>

using namespace muduo;

ChatService *ChatService::instance() {
    static ChatService service;
    return &service;
}

// 注册消息以及对应的handler回调操作
ChatService::ChatService() {
    msgHandlerMap_.insert(
        {LOGIN_MSG, std::bind(&ChatService::login, this, std::placeholders::_1,
                              std::placeholders::_2, std::placeholders::_3)});

    msgHandlerMap_.insert(
        {REG_MSG, std::bind(&ChatService::reg, this, std::placeholders::_1,
                            std::placeholders::_2, std::placeholders::_3)});
}

MsgHandler ChatService::getHandler(int msgid) {
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end()) {
        return [=](auto a, auto b, auto c) {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    } else {
        return msgHandlerMap_[msgid];
    }
}

void ChatService::login(const TcpConnectionPtr &conn, json &js,
                        Timestamp time) {
    int id = js["id"].get<int>();
    std::string pwd = js["password"];
    User user = userModel_.query(id);
    if (user.getId() == id && user.getPassword() == pwd) {
        if (user.getState() == "online") {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录，请重新输入新账号";
            conn->send(response.dump());
        } else {
            // 登录成功

            // 记录用户连接信息
            {
                std::lock_guard<std::mutex> lock(connMutex_);
                userConnectionMap_.insert({id, conn});
            }

            // 更新用户状态信息 state offline->online
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
        // 该用户不存在，登录失败
        // 用户存在但是密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或者密码错误";
        conn->send(response.dump());
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = userModel_.insert(user);
    if (state) {
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
