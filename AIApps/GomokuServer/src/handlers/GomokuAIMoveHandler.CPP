#include "../include/handlers/GomokuAIMoveHandler.h"

void GomokuAIMoveHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try {
        auto session = server_->getSessionManager()->getSession(req, resp);
        if (!session) {
            throw std::runtime_error("Invalid session");
        }
        auto sessionId = session->getId();
        std::lock_guard<std::mutex> lock(server_->boardMapMutex);
        auto& game = server_->boardMap[sessionId];
        if (game.getCurrentPlayer() != 2) {
            throw std::runtime_error("Not white player's turn");
        }
        bool placed = false;
        int aiX = -1;
        int aiY = -1;
        for (int y = 0; y < 32 && !placed; ++y) {
            for (int x = 0; x < 32 && !placed; ++x) {
                if (game.placeStone(x, y, 2)) {
                    aiX = x;
                    aiY = y;
                    placed = true;
                }
            }
        }
        if (!placed) {
            throw std::runtime_error("AI has no valid move");
        }
        int winner = 0;
        bool isWin = game.checkWin(winner);
        bool isDraw = !isWin && game.isBoardFull();
        if (!isWin && !isDraw) {
            game.switchPlayer();
        }
        json successResp;
        successResp["success"] = true;
        successResp["board"] = game.serialize();
        successResp["aiMove"] = {
            {"x", aiX},
            {"y", aiY}
        };
        successResp["nextTurn"] = isWin || isDraw ? game.getCurrentPlayer() : 1;
        successResp["gameOver"] = isWin || isDraw;
        successResp["winner"] = winner;
        std::string successBody = successResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::Ok200, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
    }
    catch (const std::exception& e) {
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
