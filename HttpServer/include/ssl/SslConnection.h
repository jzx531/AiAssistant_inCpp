#ifndef SSLCONNECTION_H
#define SSLCONNECTION_H

#include "SslContext.h"
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/noncopyable.h>
#include <openssl/ssl.h>
#include <memory>

namespace ssl
{
//添加消息回调函数类型定义
using MessageCallback = std::function<void(const std::shared_ptr<muduo::net::TcpConnection>&,
                                        muduo::net::Buffer*,
                                        muduo::Timestamp)>;

class SslConnection : muduo::noncopyable
{
public:
    using TcpConnectionPtr = std::shared_ptr<muduo::net::TcpConnection>;
    using BufferPtr = std::shared_ptr<muduo::net::TcpConnection>;

    SslConnection(const TcpConnectionPtr &conn,SslContext *ctx);
    ~SslConnection();

    
};
}

#endif // SSLCONNECTION_H

