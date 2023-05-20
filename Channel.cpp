#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

// channel 的 tie 方法什么时候调用过？一个 TcpConnection 新连接创建的时候 TcpConnection => Channel 
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// 当改变 channel 所表示 fd 的事件后，update 负责在 poller 里面更改 fd 相应的事件，实际上会调用 epoll_ctl 函数。 
void Channel::update()
{
    // 通过 channel 所属的 EventLoop，调用 poller 的相应方法。
    loop_->updateChannel(this);
}

// 在 channel 所属的 EventLoop 中， 将当前的 channel 对象删除。
void Channel::remove()
{
    loop_->removeChannel(this);
}

// fd 得到 poller 通知以后，线程安全地调用 handleEventWithGuard 函数。
void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据 poller 通知的 channel 发生的具体事件，由 channel 负责调用具体的回调操作。
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    // 处理关闭事件
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    // 处理错误事件
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    // 处理可读事件
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    // 处理可写事件
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}