#ifndef GOMOKU_SERVER_H
#define GOMOKU_SERVER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "../../../HttpServer/include/http/HttpServer.h"
#include "../../../HttpServer/include/utils/FileUtil.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"


#include "../include/aigame/GomokuAI.h"
#include "aigame/GomokuGame.h"

class GomokuMoveHandler;
class GomokuStateHandler;
class GomokuResetHandler;
class GomokuEntryHandler;
class GomokuAIMoveHandler;

class GomokuServer {
public:

    GomokuServer(int port,
		const std::string& name,
		muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

	void setThreadNum(int numThreads);
	void start();


private:
    friend class GomokuMoveHandler;
    friend class GomokuStateHandler;
    friend class GomokuResetHandler;
    friend class GomokuEntryHandler;
    friend class GomokuAIMoveHandler;

private:
    void initialize();
	void initializeSession();
	void initializeRouter();
	void initializeMiddleware();

    void packageResp(const std::string& version, http::HttpResponse::HttpStatusCode statusCode,
		const std::string& statusMsg, bool close, const std::string& contentType,
		int contentLen, const std::string& body, http::HttpResponse* resp);

    void setSessionManager(std::unique_ptr<http::session::SessionManager> manager)
	{
		httpServer_.setSessionManager(std::move(manager));
	}
    
    http::session::SessionManager* getSessionManager() const
    {
        return httpServer_.getSessionManager();
    }

private:
    http::HttpServer httpServer_;
    std::unordered_map<std::string, GomokuGame> boardMap;
    std::mutex boardMapMutex;

    std::unordered_map<std::string, std::shared_ptr<GomokuAI>> aiMap;
    std::mutex aiMapMutex;
};

#endif
