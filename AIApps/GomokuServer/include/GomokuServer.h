#ifndef GOMOKU_SERVER_H
#define GOMOKU_SERVER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../../../HttpServer/include/http/HttpServer.h"
#include "../../../HttpServer/include/utils/FileUtil.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"
#include "aigame/GomokuGame.h"

class GomokuMoveHandler;
class GomokuStateHandler;
class GomokuResetHandler;
class GomokuEntryHandler;
class GomokuAIMoveHandler;

class GomokuServer {
public:
    http::session::SessionManager* getSessionManager() const
    {
        return httpServer_.getSessionManager();
    }

private:
    friend class GomokuMoveHandler;
    friend class GomokuStateHandler;
    friend class GomokuResetHandler;
    friend class GomokuEntryHandler;
    friend class GomokuAIMoveHandler;

    http::HttpServer httpServer_;
    std::unordered_map<std::string, GomokuGame> boardMap;
    std::mutex boardMapMutex;
};

#endif
