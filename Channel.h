#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 设置 fd 相应的事件状态
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回 fd 当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    // 防止当 channel 被手动删除掉，channel 还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    int set_revents(int revt) { revents_ = revt; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* ownerLoop() { return loop_; }
    void remove();

    void handleEvent(Timestamp receiveTime);  

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;                 // 事件循环
    const int fd_;                    // 封装的 fd，受到 Poller 监听。
    int events_;                      // 注册 fd 感兴趣的事件
    int revents_;                     // Poller 返回的具体发生的事件。
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 由于 channel 中能够得到 fd 发生的具体的事件 revents，所以它负责调用具体事件的回调操作。
    ReadEventCallback readCallback_;  // 可读事件回调函数对象
    EventCallback writeCallback_;     // 可写事件回调函数对象
    EventCallback closeCallback_;     // 关闭事件回调函数对象
    EventCallback errorCallback_;     // 错误事件回调函数对象
};

