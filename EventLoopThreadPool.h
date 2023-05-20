#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; 

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    EventLoop* getNextLoop();
    std::vector<EventLoop*> getAllLoops();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    EventLoop *baseLoop_;                                      // main loop 指针
    std::string name_;
    bool started_;
    int numThreads_;                                           // subloop 线程数量
    int next_;                                                 // 用于轮询算法
    std::vector<std::unique_ptr<EventLoopThread>> threads_;    // 运行 subloop 的线程数组
    std::vector<EventLoop*> loops_;                            // subloop 数组
};