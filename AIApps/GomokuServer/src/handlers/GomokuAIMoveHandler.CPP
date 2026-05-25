#include "../include/handlers/GomokuAIMoveHandler.h"

void GomokuAIMoveHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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
        auto session = server_->getSessionManager()->getSession(req, resp);
        if(session.empty()){
            throw std::runtime_error("Invalid session");  // Handle invalid session
        }
        auto sessionId = session->getId();

        std::lock_guard<std::mutex> lock(server_->boardMapMutex);
        server_->boardMap[sessionId].setCurrentPlayer(2);
        
        //先不用ai，按顺序取一个空位
        int x,y;
        bool find = false;  //标记是否找到空位
        for(x = 0; x < 32; x ++)
        {
            for(y = 0; y < 32;y++)
            {   
                find =server_->boardMap[sessionId].placeStone(x, y);
                if(find)
                {   
                    break;
                }
            }
        }
        if(!find){
            throw std::runtime_error("ai move failed");  // Handle no empty space
        }

        int winner = 0;
        bool isWin = server_->boardMap[sessionId].checkWin(winner);
        json successResp;
        successResp["success"] = "true";
        successResp["board"] = server_->boardMap[sessionId].serialize();
        
        successResp["aiMove"] = {
            {"x", x},
            {"y", y}
        };
        successResp["nextTurn"] = 1;
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