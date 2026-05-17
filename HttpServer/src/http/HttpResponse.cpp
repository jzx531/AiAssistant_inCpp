#include "../../include/http/HttpResponse.h"

namespace http
{
void HttpResponse::appendToBuffer(muduo::net::Buffer* outputBuf) const
{
    //HttpResponse封装的信息格式化输出
    char buf[32];
    // 为什么不把状态信息放入格式化字符串中，因为状态信息有长有短，不方便定义一个固定大小的内存存储
    snprintf(buf, sizeof buf, "%s %d ", httpVersion_.c_str(), statusCode_);
    outputBuf->append(buf);
    outputBuf->append(statusMessage_);
    outputBuf->append("\r\n");

    if(closeConnection_)
    {
        outputBuf->append("Connection: close\r\n");
    }
    else{
        outputBuf->append("Connection: keep-alive\r\n");
    }

    for(const auto& header : headers_)
    {
        outputBuf->append(header.first);
        outputBuf->append(": ");
        outputBuf->append(header.second);
        outputBuf->append("\r\n");
    }
    outputBuf->append("\r\n");
    outputBuf->append(body_);
}

void HttpResponse::setStatusLine(
    const std::string & version,
    HttpStatusCode statusCode,
    const std::string & statusMessage
){
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
}

}



