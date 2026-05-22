#ifndef AI_MENU_HANDLER_H
#define AI_MENU_HANDLER_H

#include "../../../HttpServer/include/router/RouterHandler.h"
#include "../ChatServer.h"


class AIMenuHandler : public http::router::RouterHandler
{
public:
    explicit AIMenuHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:
    ChatServer* server_;
};

#endif // AI_MENU_HANDLER_H

