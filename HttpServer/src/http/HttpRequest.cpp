#include "../../include/http/HttpRequest.h"

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <utility>

namespace http
{
void HttpRequest::setReceiveTime(muduo::Timestamp t)
{
    receiveTime_ = t;
}

bool HttpRequest::setMethod(const char *start,const char *end)
{
    assert(method_ == Invalid);
    std::string m(start,end);
    if(m == "GET")
    {
        method_ = Get;
    }
    else if(m == "POST")
    {
        method_ = Post;
    }
    else if(m == "HEAD")
    {
        method_ = Head;
    }
    else if(m == "PUT")
    {
        method_ = Put;
    }
    else if(m == "DELETE")
    {
        method_ = Delete;
    }
    else if(m == "OPTIONS")
    {
        method_ = Options;
    }
    else
    {
        method_ = Invalid;
    }
    return method_ != Invalid;
}

void HttpRequest::setPath(const char *start,const char *end)
{
    path_.assign(start,end);
}

void HttpRequest::setPathParameters(const std::string &key,const std::string &value)
{
    pathParameters_[key] = value;
}

std::string HttpRequest::getPathParameter(const std::string &key) const
{
    auto it = pathParameters_.find(key);
    if(it != pathParameters_.end())
    {
        return it->second;
    }
    return "";
}

std::string HttpRequest::getQueryParameter(const std::string &key) const
{
    auto it = queryParameters_.find(key);
    if (it != queryParameters_.end())
    {
        return it->second;
    }
    return "";
}

//从问号后分割参数
void HttpRequest::setQueryParameters(const char * start,const char *end)
{
    std::string argumentStr(start, end);
    std::string::size_type pos = 0;
    std::string::size_type prev = 0;

    //按 & 分割多个参数
    while((pos = argumentStr.find('&', prev)) != std::string::npos)
    {
        std::string pair = argumentStr.substr(prev, pos - prev);
        std::string::size_type equalPos = pair.find('=');
        if(equalPos != std::string::npos)
        {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            queryParameters_[key] = value;
        }
        prev = pos + 1;
    }
    //处理最后一个参数
    std::string pair = argumentStr.substr(prev);
    std::string::size_type equalPos = pair.find('=');
    if(equalPos != std::string::npos)
    {
        std::string key = pair.substr(0, equalPos);
        std::string value = pair.substr(equalPos + 1);
        queryParameters_[key] = value;
    }
}

void HttpRequest::addHeader(const char* start,const char* colon,const char* end)
{
    std::string key(start,colon);
    ++colon;
    while(colon < end && isspace(*colon))
    {
        ++colon;
    }
    std::string value(colon,end);
    while(!value.empty() && isspace(value[value.size() - 1]))
    {
        value.resize(value.size() - 1);
    }
    headers_[key] = value;
}

std::string HttpRequest::getHeader(const std::string &field) const{
    std::string result;
    auto it = headers_.find(field);
    if(it != headers_.end())
    {
        result = it->second;
    }
    return result;
}

void HttpRequest::swap(HttpRequest &that)
{
    std::swap(method_, that.method_);
    std::swap(path_, that.path_);
    std::swap(pathParameters_, that.pathParameters_);
    std::swap(queryParameters_, that.queryParameters_);
    std::swap(version_, that.version_);
    std::swap(headers_, that.headers_);
    std::swap(receiveTime_, that.receiveTime_);
}

}
