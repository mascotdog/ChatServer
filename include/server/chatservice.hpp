#ifndef __CHATSERVICE_H__
#define __CHATSERVICE_H__

#include "json.hpp"

#include <functional>
#include <muduo/net/TcpConnection.h>
#include <unordered_map>

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

using MsgHandler =
    std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

// 聊天服务器业务类
class ChatService {
public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

private:
    ChatService();
    // 存储消息id和其对应的业务处理方法
    std::unordered_map<int, MsgHandler> msgHandlerMap_;
};

#endif // __CHATSERVICE_H__