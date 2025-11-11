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
                        Timestamp time) {}

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
        response["id"] = user.getId();
        conn->send(response.dump());
    }
}
