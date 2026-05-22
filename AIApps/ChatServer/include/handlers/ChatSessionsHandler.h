#ifndef CHAT_SESSIONS_HANDLER_H
#define CHAT_SESSIONS_HANDLER_H

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../../../HttpServer/include/utils/MysqlUtil.h"
#include "../ChatServer.h"

class ChatSessionsHandler : public http::router::RouterHandler
{
public:
    explicit ChatSessionsHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:

private:
    ChatServer* server_;
    http::MysqlUtil     mysqlUtil_;
};

#endif

