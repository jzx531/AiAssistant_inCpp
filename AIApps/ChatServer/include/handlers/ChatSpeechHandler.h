#ifndef CHAT_SPEECH_HANDLER_H
#define CHAT_SPEECH_HANDLER_H

#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../ChatServer.h"

class ChatSpeechHandler : public http::router::RouterHandler
{
public:
    explicit ChatSpeechHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:

private:
    ChatServer* server_;
};

#endif
