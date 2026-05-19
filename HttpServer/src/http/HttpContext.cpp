#include "../../include/http/HttpContext.h"

using namespace muduo;
using namespace muduo::net;

namespace http
{
    //将报文解析出来将关键信息封装到HttpRequest对象里面
bool HttpContext::parseRequest(Buffer *buf,Timestamp receiveTime)
{
    bool ok = true; //解析每行请求格式是否正确
    bool hasMore = true;
    while(hasMore)
    {
        if(state_ == ExpectRequestLine)
        {
            const char *crlf = buf->findCRLF(); //注意这个返回值边界可能不准确，需要进一步处理
            if(crlf)
            {
                ok = processRequestLine(buf->peek(),crlf);
                if(ok){
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf+2); //将请求行及CRLF都移除
                    state_ = ExpectHeaders;
                }
                else{
                    hasMore = false;
                }
            }
        }
        else if(state_ == ExpectHeaders)
        {
            const char *crlf = buf->findCRLF();
            if(crlf)
            {
                const char *colon = std::find(buf->peek(),crlf,':');
                if(colon && colon < crlf)
                {
                    request_.addHeader(buf->peek(),colon,crlf);
                }
                else if(buf->peek() == crlf)
                {
                    //空行,结束Header
                    //根据请求方法和Content-Length判断是否需要继续读取body
                    if(request_.method() == HttpRequest::Post || request_.method() == HttpRequest::Put)
                    {
                        std::string contentLength = request_.getHeader("Content-Length");
                        if (!contentLength.empty())
                        {
                            request_.setContentLength(std::atoi(contentLength.c_str()));
                            if(request_.contentLength() > 0)
                            {
                                state_ = ExpectBody;
                            }
                            else{
                                state_ = GotAll;
                                hasMore = false;
                            }
                        }
                        else{
                            ok = false;
                            hasMore = false;
                        }
                    }else{
                        state_ = GotAll;
                        hasMore = false;
                    }
                }
                else{
                    ok = false;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf+2); //将Header及CRLF都移除
            }
        }
        else if(state_ == ExpectBody)
        {
            if(buf->readableBytes() < request_.contentLength())
            {
                hasMore = false;
                return true;
            }
            //只读取content-length指定的长度
            std::string body(buf->peek(),buf->peek()+request_.contentLength());
            request_.setBody(body);

            //准确移动读指针
            buf->retrieve(request_.contentLength());
            state_ = GotAll;
            hasMore = false;
        }
    }
    return ok;
}

// 解析请求行
bool HttpContext::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start,end,' ');
    if (space != end && request_.setMethod(start, space))
    {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {
            const char *argumentStart = std::find(start, space, '?');
            if (argumentStart != space) // 请求带参数
            {
                request_.setPath(start, argumentStart); // 注意这些返回值边界
                request_.setQueryParameters(argumentStart + 1, space);
            }
            else // 请求不带参数
            {
                request_.setPath(start, space);
            }

            start = space + 1;
            succeed = ((end - start == 8) && std::equal(start, end - 1, "HTTP/1."));
            if (succeed)
            {
                if (*(end - 1) == '1')
                {
                    request_.setVersion("HTTP/1.1");
                }
                else if (*(end - 1) == '0')
                {
                    request_.setVersion("HTTP/1.0");
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}
}
