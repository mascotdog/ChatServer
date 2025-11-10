#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <functional>
#include <string>

using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr,
                       const string &nameArg)
    : server_(loop, listenAddr, nameArg), loop_(loop) {
    // 注册连接回调
    server_.setConnectionCallback(
        std::bind(&ChatServer::onConnection, this, std::placeholders::_1));

    // 注册消息回调
    server_.setMessageCallback(
        std::bind(&ChatServer::onMessage, this, std::placeholders::_1,
                  std::placeholders::_2, std::placeholders::_3));

    server_.setThreadNum(3);
}

void ChatServer::start() { server_.start(); }

void ChatServer::onConnection(const TcpConnectionPtr &conn) {
    // 客户端断开连接
    if (!conn->connected()) {
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer,
                           Timestamp time) {
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);
    // 通过js["msgid"] 获取 => 业务handler => conn js time
    // 达到的目的：完全解耦网络模块的代码和业务模块的代码

    auto msgHandler =
        ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}
