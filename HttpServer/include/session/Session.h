#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>

namespace http
{
namespace session
{
class SessionManager;
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(const std::string & sessionId,SessionManager *sessionManager,int maxAge=3600);

    const std::string &getId() const{
        return sessionId_;
    }
    
}
}
}

#endif // HTTP_SESSION_H

