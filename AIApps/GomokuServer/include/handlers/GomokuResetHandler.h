#ifndef GOMOKU_RESET_HANDLER_H
#define GOMOKU_RESET_HANDLER_H

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"

class GomokuResetHandler : public http::router::RouterHandler
{
public:
    explicit GomokuResetHandler(GomokuServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:

private:
    GomokuServer* server_;  
};

#endif

