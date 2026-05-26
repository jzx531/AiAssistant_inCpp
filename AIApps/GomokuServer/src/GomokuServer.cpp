#include "../include/handlers/GomokuMoveHandler.h"
#include "../include/handlers/GomokuStateHandler.h"
#include "../include/handlers/GomokuResetHandler.h"
#include "../include/handlers/GomokuEntryHandler.h"
#include "../include/handlers/GomokuAIMoveHandler.h"


#include "../include/GomokuServer.h"

using namespace http;

GomokuServer::GomokuServer(int port,
                       const std::string& name,
                       muduo::net::TcpServer::Option option)
                       :httpServer_(port,name,false,option)
{
    initialize();
}

void GomokuServer::initialize()
{
    std::cout << "GomokuServer initialize start" << std::endl;
    initializeSession();
    initializeMiddleware();
    initializeRouter();
}

void GomokuServer::setThreadNum(int numThreads){
    httpServer_.setThreadNum(numThreads);
}

void GomokuServer::start()
{
    httpServer_.start();
}

void GomokuServer::initializeRouter() {
    auto serveStaticFile = [this](const std::string& path,
                                  const std::string& contentType,
                                  const http::HttpRequest& req,
                                  http::HttpResponse* resp) {
        FileUtil fileOperater(path);
        if (!fileOperater.isValid()) {
            packageResp(req.getVersion(), http::HttpResponse::NotFound404,
                "Not Found", true, "text/plain", 9, "Not Found", resp);
            return;
        }

        std::vector<char> buffer(fileOperater.size());
        fileOperater.readFile(buffer);
        std::string body(buffer.data(), buffer.size());
        packageResp(req.getVersion(), http::HttpResponse::Ok200,
            "OK", false, contentType, static_cast<int>(body.size()), body, resp);
    };

    httpServer_.Get("/gomoku", std::make_shared<GomokuEntryHandler>(this));
    httpServer_.Get("/gomoku/gomoku.css", [serveStaticFile](const http::HttpRequest& req, http::HttpResponse* resp) {
        serveStaticFile("../AIApps/GomokuServer/resource/gomoku.css", "text/css", req, resp);
    });
    httpServer_.Get("/gomoku/gomoku.js", [serveStaticFile](const http::HttpRequest& req, http::HttpResponse* resp) {
        serveStaticFile("../AIApps/GomokuServer/resource/gomoku.js", "application/javascript", req, resp);
    });
    httpServer_.Get("/gomoku/state", std::make_shared<GomokuStateHandler>(this));
    httpServer_.Post("/gomoku/move", std::make_shared<GomokuMoveHandler>(this));
    httpServer_.Post("/gomoku/reset", std::make_shared<GomokuResetHandler>(this));
    httpServer_.Post("/gomoku/ai-move", std::make_shared<GomokuAIMoveHandler>(this));
    std::cout << "GomokuServer initialize router end" << std::endl;   
}


void GomokuServer::initializeSession()
{
    auto sessionStorage = std::make_unique<http::session::MemorySessionStorage>();
    auto sessionManager = std::make_unique<http::session::SessionManager>(std::move(sessionStorage));
    setSessionManager(std::move(sessionManager));
}

void GomokuServer::initializeMiddleware()
{
    auto corsMiddleware = std::make_shared<http::middleware::CorsMiddleware>();
    
    httpServer_.addMiddleware(corsMiddleware);
}

void GomokuServer::packageResp(const std::string& version,
    http::HttpResponse::HttpStatusCode statusCode,
    const std::string& statusMsg,
    bool close,
    const std::string& contentType,
    int contentLen,
    const std::string& body,
    http::HttpResponse* resp)
{
    if(resp == nullptr)
    {
        LOG_ERROR << "Response pointer is null";
        return;
    }
    try
    {
        resp->setVersion(version);
        resp->setStatusCode(statusCode);
        resp->setStatusMessage(statusMsg);
        resp->setCloseConnection(close);
        resp->setContentType(contentType);
        resp->setContentLength(contentLen);
        resp->setBody(body);

        LOG_INFO << "Response packaged successfully";
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Error in packageResp: " << e.what();

        resp->setStatusCode(http::HttpResponse::InternalServerError500);
        resp->setStatusMessage("Internal Server Error");
        resp->setCloseConnection(true);
    }
}

