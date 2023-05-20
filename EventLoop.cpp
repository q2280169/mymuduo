#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

// 防止一个线程创建多个 EventLoop。
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的 Poller IO 复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建 wakeupfd，用来唤醒 subReactor 处理新来的 channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

// 构造函数
EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置 wakeupfd 的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个 eventloop 都将监听 wakeupchannel 的 EPOLLIN 读事件。
    wakeupChannel_->enableReading();
}

// 析构函数
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        // 监听两类 fd：一种是 listenfd/connectfd，一种是 wakeupfd。
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller 监听哪些 channel 发生事件了，然后上报给 EventLoop，通知 channel 处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        
        // 执行当前 EventLoop 事件循环需要处理的回调操作
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环
void EventLoop::quit()
{
    quit_ = true;

    // 如果是在其它线程中，调用的quit   在一个subloop(woker)中，调用了mainLoop(IO)的quit
    if (!isInLoopThread())  
    {
        wakeup();
    }
}

// 在 EventLoop 中执行回调函数
void EventLoop::runInLoop(Functor cb)
{
    // 若处于 EventLoop 线程中，直接执行回调函数。
    if (isInLoopThread()) 
    {
        cb();
    }
    // 若处于非 EventLoop 线程中，则唤醒 EventLoop 所在的线程，再执行回调函数。
    else 
    {
        queueInLoop(cb);
    }
}

// 把回调函数对象放入队列中，唤醒 loop 所在的线程，执行 cb。
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒运行 EventLoop 的线程。callingPendingFunctors_ 的意思是：当前 loop 正在执行回调，但是 loop 又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); 
    }
}

// wakeupfd 接收到唤醒数据后的回调处理函数。
void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
  }
}

// 唤醒阻塞中的 EventLoop 线程。
void EventLoop::wakeup()
{
    // 向 wakeupfd_ 写一个数据，wakeupChannel 会发生读事件，当前阻塞线程就会被唤醒。
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// 更新 EventLoop 的 Poller 中的 Channel 对象。
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

// 将 EventLoop 的 Poller 中的 Channel 对象删除。
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

// 判断 EventLoop 的 Poller 中是否含有 Channel 对象。
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

// 执行函数队列中的函数
void EventLoop::doPendingFunctors() 
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前 loop 需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}