#pragma once

#include <vector>
#include <string>
#include <algorithm>


//    +-----------------------+-------------------------------+---------------------------+
//    |    prependable bytes  |       readable bytes          |      writable bytes       |
//    |                       |          (content)            |                           |
//    +-----------------------+-------------------------------+---------------------------+
//    |                       |                               |                           |
//    0        <=        readerIndex        <=           writerIndex         <=         size


// 网络库底层的缓冲区类型
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;    // 缓冲区初始大小

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    // 获取可读数据字节数
    size_t readableBytes() const 
    {
        return writerIndex_ - readerIndex_;
    }

    // 获取可写缓冲区长度
    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            // 应用只读取了刻度缓冲区数据的一部分，就是len，还剩下 readerIndex_ += len -> writerIndex_
            readerIndex_ += len; 
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把 onMessage 函数上报的 Buffer 数据，转成 string 类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        // 上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
        retrieve(len); 
        return result;
    }

    // buffer_.size() - writerIndex_    len
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }

    // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从 fd 上读取数据
    ssize_t readFd(int fd, int* saveErrno);
    // 通过 fd 发送数据
    ssize_t writeFd(int fd, int* saveErrno);
private:
    // 获取缓冲区数组的起始地址
    char* begin()
    {
        // vector 底层数组首元素的地址，也就是数组的起始地址
        return &*buffer_.begin();  
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    // 扩充缓冲区数组大小
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readalbe = readableBytes();
            std::copy(begin() + readerIndex_, 
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readalbe;
        }
    }

    std::vector<char> buffer_;    // 
    size_t readerIndex_;          // 读下标
    size_t writerIndex_;          // 写下标
};