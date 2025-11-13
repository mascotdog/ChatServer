# ChatServer 集群聊天系统

> 基于 C++11 + Muduo 的本地高并发集群聊天服务器，支持登录、注册、单聊、群聊、好友管理、离线消息。通过 Nginx 的 TCP 负载均衡与 Redis 发布/订阅实现跨节点消息路由，MySQL 持久化用户与关系数据，JSON 作为应用层协议。

## ⭐ 项目特性
- Reactor + 多线程：基于 Muduo 网络库，实现高并发非阻塞 TCP 服务器。
- 业务解耦：消息号 `msgid` 与回调函数映射（见 `ChatService::msgHandlerMap_`），新增业务无需改动网络层。
- 集群扩展：前端 Nginx 四层负载均衡分发连接；后端多台 ChatServer 通过 Redis Pub/Sub 互通消息。
- 离线消息：用户不在线时消息入库（`offlinemessagemodel`），登录时拉取并清理。
- 群聊支持：群组创建、加入、群消息转发与离线存储。
- 统一 JSON 协议：客户端与服务端均使用 `nlohmann::json` 序列化与反序列化。
- 日志与可观测：Muduo 提供时间戳、多线程安全日志（INFO/ERROR）。

## 🧱 技术栈
| 分类 | 使用 | 说明 |
|------|------|------|
| 语言 | C++11 | 主体实现 |
| 网络 | Muduo | 高性能 Reactor 模型 |
| 协议 | JSON (nlohmann/json) | 私有消息封装 |
| 负载均衡 | Nginx stream | TCP 四层分发，多实例扩展 |
| 中间件 | Redis (hiredis) | 发布/订阅跨节点转发消息 |
| 数据库 | MySQL | 用户 / 好友 / 群组 / 离线消息持久化 |
| 构建 | CMake | 跨平台构建脚本 |
| 并发 | pthread / Muduo | 线程与事件驱动 |

## 📂 目录结构
```
chat/
├── CMakeLists.txt
├── README.md
├── chat.sql                  # 数据库建表及初始化数据
├── include/
│   ├── public.hpp            # 公共消息号枚举
│   └── server/
│       ├── chatserver.hpp
│       ├── chatservice.hpp
│       ├── db/db.h
│       ├── model/*.hpp       # 模型：User/Friend/Group/OfflineMsg 等
│       ├── redis/redis.hpp   # Redis 通信封装
│       └── user.hpp          # 用户实体类
├── src/
│   ├── CMakeLists.txt
│   ├── client/
│   │   ├── main.cpp          # ChatClient 客户端
│   │   └── CMakeLists.txt
│   └── server/
│       ├── main.cpp          # Muduo 服务器启动入口
│       ├── chatserver.cpp    # 网络层封装（连接/消息回调）
│       ├── chatservice.cpp   # 业务层分发与处理
│       ├── db/db.cpp         # MySQL 简易封装
│       ├── model/*.cpp       # 数据模型实现
│       ├── redis/redis.cpp   # Redis 订阅/发布
│       └── CMakeLists.txt
├── thirdparty/
│   ├── json.hpp              # nlohmann/json 单头文件版本
│   └── nginx/                # 示例或相关配置目录（可自行扩展）
└── bin/                      # 编译产物(ChatServer/ChatClient)
```

## 🚀 快速开始
### 1. 安装依赖
请确保系统安装以下组件（Linux 环境推荐 Ubuntu/Debian/CentOS）：
- g++ (支持 C++11) / clang
- CMake ≥ 3.0
- MySQL Server & Client 库: `libmysqlclient-dev`
- Redis Server
- hiredis 开发库: `libhiredis-dev`
- Nginx （需要启用 `stream` 模块，用于 TCP 负载均衡）
- Muduo 库（需提前编译安装，确保 `muduo_net` `muduo_base` 可被链接）

示例（Ubuntu）安装命令：
```bash
sudo apt update
sudo apt install -y build-essential cmake libmysqlclient-dev redis-server libhiredis-dev nginx
```
Muduo 安装参考其官方仓库文档，安装完成后确保 `pkg-config --list-all | grep muduo` 有结果或库路径在系统默认搜索路径中。

### 2. 初始化数据库
启动 MySQL 后执行：
```bash
mysql -u root -p < chat.sql
```
或者手动步骤：
```bash
mysql -u root -p
# 创建/使用数据库
CREATE DATABASE IF NOT EXISTS chat DEFAULT CHARACTER SET utf8mb4;
USE chat;
# 导入表结构与初始数据
SOURCE /path/to/chat.sql;
```
> 建议统一字符集为 `utf8mb4` 避免中文或表情解析问题。如现有表含 latin1，可通过 ALTER / CONVERT 修改。

### 3. 编译项目
```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```
构建完成后，可执行文件位于 `bin/`：`ChatServer` 与 `ChatClient`。

### 4. 启动 Redis & Nginx
```bash
sudo service redis-server start
sudo service nginx start
```
示例 Nginx `stream` 配置（通常在 `/etc/nginx/nginx.conf` 中追加）：
```nginx
# nginx top loadbalance config
upstream MyServer {
    server 127.0.0.1:6000 weight=1 max_fails=3 fail_timeout=30s;
    server 127.0.0.1:6002 weight=1 max_fails=3 fail_timeout=30s;
}

server {
    proxy_connect_timeout 1s;
    proxy_read_timeout 3s;
    listen 8000;
    proxy_pass MyServer;
}
tcp_nodelay on;
```
修改后：
```bash
sudo nginx -t
sudo nginx -s reload
```

### 5. 启动后端服务器
在多个终端分别启动不同端口实例：
```bash
./bin/ChatServer 127.0.0.1 6000   
./bin/ChatServer 127.0.0.1 6002
```

### 6. 启动客户端
```bash
./bin/ChatClient 127.0.0.1 8000   # 连接 Nginx 统一入口
```


## 🧩 消息协议（`public.hpp`）
| msgid | 含义 |
|-------|------|
| 1 | LOGIN_MSG（登录请求） |
| 2 | LOGIN_MSG_ACK（登录响应） |
| 3 | LOGINOUT_MSG（注销） |
| 4 | REG_MSG（注册请求） |
| 5 | REG_MSG_ACK（注册响应） |
| 6 | ONE_CHAT_MSG（单聊） |
| 7 | ADD_FRIEND_MSG（添加好友） |
| 8 | CREATE_GROUP_MSG（创建群组） |
| 9 | ADD_GROUP_MSG（加入群组） |
| 10 | GROUP_CHAT_MSG（群聊） |

示例：单聊消息 JSON
```json
{
  "msgid": 6,
  "id": 13,
  "name": "zhang san",
  "toid": 21,
  "msg": "你好",
  "time": "2025-11-13 10:12:33"
}
```
> 注意：当前服务端 `oneChat` 使用字段 `js["to"]`，而客户端发送为 `toid`，需统一字段名，否则会导致消息无法路由。可以在客户端改为 `js["to"] = friendid;` 或在服务端改访问 `js["toid"]`。

## 🔄 Redis 发布/订阅
- 每个在线用户上线后，服务端订阅以用户 id 作为 channel。
- 若目标用户连接在其他 ChatServer 实例，消息通过 `publish(channel, message)` 投递，订阅方在独立线程回调中处理并下发。
- 服务器断开 / 用户注销时取消订阅，避免资源泄漏。

## 🗄️ MySQL 表概览
- `user`：用户基本信息（状态 online/offline）
- `friend`：好友关系（userid, friendid）
- `group` / `groupuser`：群组与成员角色（creator/normal）
- `offlinemessage`：离线消息临时存储（用户登录后读取 + 删除）

## ⚙️ 关键类职责
| 类 | 位置 | 职责摘要 |
|----|------|---------|
| `ChatServer` | `src/server/chatserver.cpp` | 建立连接、注册回调、接收数据并交给业务层 |
| `ChatService` | `src/server/chatservice.cpp` | 消息分发、用户状态、好友/群组/离线消息逻辑、Redis 集群通信 |
| `Redis` | `include/server/redis/redis.hpp` | 发布/订阅、跨节点消息传递回调 |
| `UserModel` 等 | `src/server/model/*` | 数据库 CRUD 封装 |
| `MySQL` | `src/server/db/db.cpp` | 连接与执行 SQL（当前使用简单封装，不是连接池） |

## 🛠️ 可能的改进方向
- 信息加密：采用 RSA+AES 混合加密方案，通过 RSA 安全传输 AES 密钥，后续核心数据（账号密码、消息正文）用 AES 加密传输，规避明文交互的安全隐患。
- 信息可靠传递：实现业务层 ACK 确认机制，消息发送端缓存消息并启动超时重传（3 次上限），接收端收到消息后反馈序列号确认，结合心跳检测确保连接有效性，避免消息丢失或传输失败。
- 引入数据库连接池：降低频繁连接开销，提升高并发场景吞吐。
- 安全增强：密码存储改为哈希（bcrypt/argon2）而非明文。
- 压测指标：JMeter + 自研脚本采集 TPS、P99 延迟、最大并发连接。

## ▶️ 示例交互（客户端命令）
```
help                       # 显示命令
chat:21:你好啊             # 单聊用户 21
addfriend:21               # 添加好友
creategroup:cpp群:讨论代码 # 创建群
addgroup:1                 # 加入群 1
groupchat:1:大家好         # 群聊发送
loginout                   # 注销
```
