#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

// 时间循环类  主要包含了两个大模块 Channel   Poller（epoll的抽象）
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;
    using ChannelList = std::vector<Channel*>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }
    
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    void wakeup();

    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断 EventLoop 对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ ==  CurrentThread::tid(); }

private:
    void handleRead();         
    void doPendingFunctors(); 

    std::atomic_bool looping_;                  // 标志处于循环中
    std::atomic_bool quit_;                     // 标志退出循环
    
    const pid_t threadId_;                      // 记录当前 loop 所在线程的 id

    Timestamp pollReturnTime_;                  // poller 返回发生事件的 channels 的时间点

    std::unique_ptr<Poller> poller_;            // EventLoop 包含的 poller 指针。 
    ChannelList activeChannels_;                // 每轮循环中发生事件的 channel 列表。

    int wakeupFd_;                              // 用户唤醒处于阻塞状态的 EventLoop
    std::unique_ptr<Channel> wakeupChannel_;    // 封装 wakeupfd 的 channel 对象。

    std::atomic_bool callingPendingFunctors_;   // 标识当前 EventLoop 是否正在执行函数队列中的函数。
    std::vector<Functor> pendingFunctors_;      // 存储 EventLoop 需要执行的函数队列。
    std::mutex mutex_;                          // 互斥锁，用来保护上面 vector 容器的线程安全操作
};