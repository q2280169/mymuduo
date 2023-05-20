#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "Timestamp.h"

class Channel;

/**
 * epoll的使用  
 * epoll_create
 * epoll_ctl   add/mod/del
 * epoll_wait
 */ 
class EPollPoller : public Poller
{
public:
    using EventList = std::vector<epoll_event>;

    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写基类 Poller 的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
    
private:
    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    void update(int operation, Channel *channel);

    int epollfd_;                              // epollfd 文件描述符。
    EventList events_;                         // epoll 返回的发生事件列表。
    static const int kInitEventListSize = 16;
};