#include "../include/handlers/GomokuMoveHandler.h"

void GomokuMoveHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    auto contentType = req.getHeader("Content-Type");
    if(contentType.empty() || contentType != "application/json" || req.getBody().empty())
    {
        LOG_INFO << "content" << req.getBody();
        resp->setStatusLine(req.getVersion(), http::HttpResponse::BadRequest400, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(0);
        resp->setBody("");
        return;
    }
    try{
        json request = json::parse(req.getBody());
        int x = request["x"];
        int y = request["y"];
        auto session = server_->getSessionManager()->getSession(req, resp);

        if(session.empty()){
            throw std::runtime_error("Invalid session");  // Handle invalid session
        }
        auto sessionId = session->getId();

        std::lock_guard<std::mutex> lock(server_->boardMapMutex);
        server_->boardMap[sessionId].setCurrentPlayer(1);
        if(!server_->boardMap[sessionId].placeStone(x, y))
        {
            throw std::runtime_error("Invalid move stone");  // Handle invalid move
        }
        int winner = 0;
        bool isWin = server_->boardMap[sessionId].checkWin(winner);
        json successResp;
        successResp["success"] = "true";
        successResp["board"] = server_->boardMap[sessionId].serialize();
        
        successResp["nextTurn"] = 2;
        successResp["gameOver"] = !isWin;
        successResp["winner"] = winner;
        
        std::string successBody = successResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::Ok200, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
    }
    catch(const std::exception& e){
        json failureResp;
        failureResp["success"] = "false";
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::BadRequest400, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}