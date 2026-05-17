#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#include <iostream>

#include <muduo/net/TcpServer.h>

#include "HttpRequest.h"

namespace http
{
class HttpContext
{
public:
    enum HttpRequestParseState
    {
        ExpectRequestLine, //解析请求行
        ExpectHeaders, //解析请求头
        ExpectBody, //解析请求体
        GotAll, //解析完成
    };

    HttpContext():state_(ExpectRequestLine)
    {}

    bool parseRequest(muduo::net::Buffer *buf,muduo::Timestamp receiveTime);
    bool gotAll() const
    {
        return state_ == GotAll;
    }
    void reset()
    {
        state_ = ExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);
    }

    const HttpRequest& request() const
    {
        return request_;
    }
    HttpRequest& request()
    { return request_;}
private:
    bool processRequestLine(const char* begin,const char* end);
private:
    HttpRequestParseState state_;
    HttpRequest request_;
};

} // namespace http


#endif