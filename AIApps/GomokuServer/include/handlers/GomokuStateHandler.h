#ifndef GOMOKU_STATE_HANDLER_H
#define GOMOKU_STATE_HANDLER_H

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"

class GomokuStateHandler : public http::router::RouterHandler
{
public:
    explicit GomokuStateHandler(GomokuServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:

private:
    GomokuServer* server_;  
};

#endif

