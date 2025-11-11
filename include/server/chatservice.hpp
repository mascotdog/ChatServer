#ifndef __CHATSERVICE_H__
#define __CHATSERVICE_H__

#include "json.hpp"
#include "offlinemessagemodel.hpp"
#include "usermodel.hpp"

#include <functional>
#include <muduo/net/TcpConnection.h>
#include <mutex>
#include <unordered_map>

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
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
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 服务器异常，业务重置方法
    void reset();

private:
    ChatService();
    // 存储消息id和其对应的业务处理方法
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储在线用户的通信连接
    std::unordered_map<int, TcpConnectionPtr> userConnectionMap_;

    // 互斥锁，保证userConnectionMap_的线程安全
    std::mutex connMutex_;

    // 数据操作类对象
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
};

#endif // __CHATSERVICE_H__