#ifndef GOMOKU_MOVE_HANDLER_H
#define GOMOKU_MOVE_HANDLER_H

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"

class GomokuMoveHandler : public http::router::RouterHandler
{
public:
    explicit GomokuMoveHandler(GomokuServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:

private:
    GomokuServer* server_;  
};

#endif

