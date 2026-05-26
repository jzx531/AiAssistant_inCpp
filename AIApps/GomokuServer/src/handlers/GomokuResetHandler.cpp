#include "../include/handlers/GomokuResetHandler.h"

void GomokuResetHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try{
        
        auto session = server_->getSessionManager()->getSession(req, resp);

        if(!session){
            throw std::runtime_error("Invalid session");  // Handle invalid session
        }
        auto sessionId = session->getId();

        std::lock_guard<std::mutex> lock(server_->boardMapMutex);
        auto& game = server_->boardMap[sessionId];
        game.resetGame();
        int winner = 0;
        json successResp;
        successResp["success"] = true;
        successResp["board"] = game.serialize();
        
        successResp["nextTurn"] = 1;
        successResp["gameOver"] = false;
        successResp["winner"] = winner;
        
        std::string successBody = successResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::Ok200, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
    }
    catch(const std::exception& e){
        json failureResp;
        failureResp["success"] = false;
        failureResp["message"] = e.what();
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::BadRequest400, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}

