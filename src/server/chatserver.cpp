#include "chatserver.hpp"

#include <functional>

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

void ChatServer::onConnection(const TcpConnectionPtr &) {
    
}

void ChatServer::onMessage(const TcpConnectionPtr &, Buffer *, Timestamp) {

}
