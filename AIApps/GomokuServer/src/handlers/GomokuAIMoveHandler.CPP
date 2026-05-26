#include "../include/handlers/GomokuAIMoveHandler.h"
#include <iostream>

namespace {

std::pair<int, int> findFirstEmptyCell(const GomokuGame& game)
{
    const auto board = game.getBoard();
    for (int row = 0; row < static_cast<int>(board.size()); ++row) {
        for (int col = 0; col < static_cast<int>(board[row].size()); ++col) {
            if (board[row][col] == 0) {
                return {col, row};
            }
        }
    }

    return {-1, -1};
}

}

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

        // 使用ai 来进行策略选择
        std::unique_lock<std::mutex> ailock(server_->aiMapMutex);
        auto& ai = server_->aiMap[sessionId];
        if (!ai) {
            ai = std::make_shared<GomokuAI>();
        }

        int failCount = 0;
        while (!placed && failCount < 3) {
            try {
                const auto move = ai->makeMove(game.getBoard());
                aiX = move.first;
                aiY = move.second;
                if (game.placeStone(aiY, aiX, 2)) {
                    placed = true;
                    break;
                }
            } catch (const std::exception&) {
                break;
            }

            if (!placed) {
                failCount++;
            }
        }

        ailock.unlock(); // 解锁aiMapMutex

        // if (!placed) {
        //     const auto fallbackMove = findFirstEmptyCell(game);
        //     aiX = fallbackMove.first;
        //     aiY = fallbackMove.second;
        //     if (aiX >= 0 && aiY >= 0) {
        //         placed = game.placeStone(aiY, aiX, 2);
        //     }
        // }

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
        std::cout << "Error: " << e.what() << std::endl; // 输出错误信息到控制台
        std::string failureBody = failureResp.dump(4);
        resp->setStatusLine(req.getVersion(), http::HttpResponse::BadRequest400, "Bad Request");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}
