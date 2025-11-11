#include "chatservice.hpp"
#include "public.hpp"

#include <muduo/base/Logging.h>
#include <vector>

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
        {LOGINOUT_MSG,
         std::bind(&ChatService::loginout, this, std::placeholders::_1,
                   std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert(
        {REG_MSG, std::bind(&ChatService::reg, this, std::placeholders::_1,
                            std::placeholders::_2, std::placeholders::_3)});

    msgHandlerMap_.insert(
        {ONE_CHAT_MSG,
         std::bind(&ChatService::oneChat, this, std::placeholders::_1,
                   std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert(
        {ADD_FRIEND_MSG,
         std::bind(&ChatService::addFriend, this, std::placeholders::_1,
                   std::placeholders::_2, std::placeholders::_3)});

    msgHandlerMap_.insert(
        {CREATE_GROUP_MSG,
         std::bind(&ChatService::createGroup, this, std::placeholders::_1,
                   std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert(
        {ADD_GROUP_MSG,
         std::bind(&ChatService::addGroup, this, std::placeholders::_1,
                   std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert(
        {GROUP_CHAT_MSG,
         std::bind(&ChatService::groupChat, this, std::placeholders::_1,
                   std::placeholders::_2, std::placeholders::_3)});
}

void ChatService::reset() {
    // 把online状态的用户，设置成offline
    userModel_.resetState();
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
            response["errmsg"] = "this account is using, input another";
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
            // 查询该用户是否有离线消息
            std::vector<std::string> vec = offlineMsgModel_.query(id);
            if (!vec.empty()) {
                response["offlinemsg"] = vec;
                // 读取后，把该用户的所有离线消息删除
                offlineMsgModel_.remove(id);
            }
            // 查询该用户的好友信息并返回
            std::vector<User> userVec = friendModel_.query(id);
            if (!userVec.empty()) {
                std::vector<std::string> user2jsVec;
                for (User &user : userVec) {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    user2jsVec.push_back(js.dump());
                }
                response["friends"] = user2jsVec;
            }

            conn->send(response.dump());
        }

    } else {
        // 该用户不存在，登录失败
        // 用户存在但是密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
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

void ChatService::loginout(const TcpConnectionPtr &conn, json &js,
                           Timestamp time) {
    int userid = js["id"].get<int>();

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        auto it = userConnectionMap_.find(userid);
        if (it != userConnectionMap_.end()) {
            userConnectionMap_.erase(it);
        }
    }

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    userModel_.updateState(user);
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
    User user;
    {
        std::lock_guard<std::mutex> lock(connMutex_);

        for (auto it = userConnectionMap_.begin();
             it != userConnectionMap_.end(); it++) {
            if (it->second == conn) {
                // 从map中删除用户的连接信息
                user.setId(it->first);
                userConnectionMap_.erase(it);
                break;
            }
        }
    }

    // 更新用户的状态信息
    if (user.getId() != -1) {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js,
                          Timestamp time) {
    int toid = js["to"].get<int>();

    {
        std::lock_guard<std::mutex> lock(connMutex_);
        auto it = userConnectionMap_.find(toid);
        if (it != userConnectionMap_.end()) {
            // 对方在线，转发消息
            // 服务器主动推送消息给对方
            it->second->send(js.dump());
            return;
        }
    }

    // 对方不在线，存储离线消息
    offlineMsgModel_.insert(toid, js.dump());
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js,
                            Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    friendModel_.insert(userid, friendid);
}

void ChatService::createGroup(const TcpConnectionPtr &conn, json &js,
                              Timestamp time) {
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (groupModel_.createGroup(group)) {
        // 存储群组创建人信息
        groupModel_.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js,
                           Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js,
                            Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    std::vector<int> useridVec = groupModel_.queryGroupUsers(userid, groupid);

    std::lock_guard<std::mutex> lock(connMutex_);
    for (int id : useridVec) {
        auto it = userConnectionMap_.find(id);
        if (it != userConnectionMap_.end()) {
            // 转发群消息
            it->second->send(js.dump());
        } else {
            // 存储离线群消息
            offlineMsgModel_.insert(id, js.dump());
        }
    }
}