#ifndef HTTP_DB_EXCEPTION_H
#define HTTP_DB_EXCEPTION_H

#include <stdexcept>
#include <string>

namespace http {
namespace db {

class DbException : public std::runtime_error 
{
public:
    explicit DbException(const std::string& message) 
        : std::runtime_error(message) {}
    
    explicit DbException(const char* message) 
        : std::runtime_error(message) {}
};

} // namespace db
} // namespace http

#endif // HTTP_DB_EXCEPTION_H
