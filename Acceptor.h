#pragma once

#include <functional>

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    // 设置新连接的回调函数
    void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }

    bool listenning() const { return listenning_; }
    void listen();
    
private:
    void handleRead();
    
    EventLoop *loop_;                                // 运行 Acceptor 的 EventLoop，即 baseLoop，也称作 mainLoop。
    Socket acceptSocket_;                            // 监听连接的文件描述符
    Channel acceptChannel_;                          // 封装监听套接字的 Channel
    NewConnectionCallback newConnectionCallback_;    // 新连接处理回调函数对象
    bool listenning_;
};