#pragma once

/**
 * 用户使用muduo编写服务器程序
 */ 
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

// 面向用户的服务器编程使用的类
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
                const InetAddress &listenAddr,
                const std::string &nameArg,
                Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitcallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    void setThreadNum(int numThreads);
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    EventLoop *loop_;    // baseLoop，用户定义的 loop

    const std::string ipPort_;    // 服务器端口
    const std::string name_;      // 服务器名称

    std::unique_ptr<Acceptor> acceptor_;              // 运行在 mainLoop，任务是监听新连接事件。

    std::shared_ptr<EventLoopThreadPool> threadPool_; // 线程池，

    ConnectionCallback connectionCallback_;           // 有新连接时的回调
    MessageCallback messageCallback_;                 // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;     // 消息发送完成以后的回调
    ThreadInitCallback threadInitCallback_;           // loop 线程初始化的回调

    std::atomic_int started_;

    int nextConnId_;               
    ConnectionMap connections_;                       // 保存所有的连接
};