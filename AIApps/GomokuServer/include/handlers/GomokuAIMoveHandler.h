#ifndef GOMOKU_AIMOVE_HANDLER_H
#define GOMOKU_AIMOVE_HANDLER_H

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"

class GomokuAIMoveHandler : public http::router::RouterHandler
{
public:
    explicit GomokuAIMoveHandler(GomokuServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:

private:
    GomokuServer* server_;  
};

#endif

