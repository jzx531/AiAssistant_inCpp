#ifndef GOMOKU_ENTRY_HANDLER_H
#define GOMOKU_ENTRY_HANDLER_H

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"

class GomokuEntryHandler : public http::router::RouterHandler
{
public:
    explicit GomokuEntryHandler(GomokuServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:

private:
    GomokuServer* server_;  
};

#endif

