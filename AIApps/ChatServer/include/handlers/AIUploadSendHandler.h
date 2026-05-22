#ifndef AIUPLOADSENDHANDLER_H
#define AIUPLOADSENDHANDLER_H

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../ChatServer.h"

class AIUploadSendHandler : public http::router::RouterHandler
{
public:
    explicit AIUploadSendHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    ChatServer* server_;
};

#endif // AIUPLOADSENDHANDLER_H

