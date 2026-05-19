#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <muduo/net/TcpServer.h>

namespace http
{
class HttpResponse
{
public:
    enum HttpStatusCode
    {
        Unknown,
        Ok200 = 200,
        NoContent204 = 204,
        MovedPermanently301 = 301,
        BadRequest400 = 400,
        Unauthorized401 = 401,
        Forbidden403 = 403,
        NotFound404 = 404,
        Conflict409 = 409,
        InternalServerError500 = 500,
    };

    HttpResponse(bool close = true):statusCode_(Unknown),closeConnection_(close){}

    void setVersion(std::string version)
    { httpVersion_ = version; }
    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    HttpStatusCode getStatusCode() const
    { return statusCode_; }

    void setStatusMessage(const std::string message)
    { statusMessage_ = message; }

    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }

    void setContentType(const std::string& contentType)
    { addHeader("Content-Type", contentType); }

    void setContentLength(size_t length)
    { addHeader("Content-Length", std::to_string(length)); }

    void addHeader(const std::string& key, const std::string& value)
    { headers_[key] = value; }

    void setBody(const std::string& body)
    { body_ = body; }

    void setStatusLine(const std::string& version,
                         HttpStatusCode statusCode,
                         const std::string& statusMessage);

    void setErrorHeader(){}

    void appendToBuffer(muduo::net::Buffer* outputBuf) const;

private:
    std::string httpVersion_;
    HttpStatusCode statusCode_;
    std::string statusMessage_;
    bool closeConnection_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    bool isFile_;
};
}


#endif // HTTP_RESPONSE_H

