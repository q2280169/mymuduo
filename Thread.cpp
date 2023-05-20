#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

// 构造函数。一个 Thread 对象，记录的就是一个新线程的详细信息。
Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}

// 析构函数
Thread::~Thread()
{
    if (started_ && !joined_)
    {
        // 使用 thread 类提供的设置分离线程的方法，避免线程资源浪费。
        thread_->detach(); 
    }
}

// 启动线程
void Thread::start()  
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    // 开启一个新线程，专门执行该线程函数
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_(); 
    }));

    // 这里必须等待获取上面新创建的线程的 tid 值
    sem_wait(&sem);
}

// 封装 std::thread 的 join 函数。join 函数的作用是将调用线程挂起，等待被调用的线程对象执行完成后，继续执行调用线程。
void Thread::join()
{
    joined_ = true;
    thread_->join();
}

// 设置线程默认的名称
void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}