#include "../include/aigame/GomokuGame.h"

namespace {

bool hasFiveInDirection(const std::vector<std::vector<int>>& board, int boardSize, int player, int row, int col, int rowStep, int colStep)
{
    for (int offset = 0; offset < 5; ++offset)
    {
        const int currentRow = row + offset * rowStep;
        const int currentCol = col + offset * colStep;
        if (currentRow < 0 || currentRow >= boardSize || currentCol < 0 || currentCol >= boardSize)
        {
            return false;
        }
        if (board[currentRow][currentCol] != player)
        {
            return false;
        }
    }
    return true;
}

}

void GomokuGame::initBoard()
{
    board_ = std::vector<std::vector<int>>(boardSize_, std::vector<int>(boardSize_, 0));
}

void GomokuGame::resetGame()
{
    initBoard();
    // Reset other game state variables here
    currentPlayer_ = 1; // Assuming player 1 starts
}

bool GomokuGame::placeStone(int row, int col, int player)
{
    if(row < 0 || row >= boardSize_ || col < 0 || col >= boardSize_)
    {
        return false; // Invalid move
    }
    if (board_[row][col] != 0)
    {
        return false; // Stone already placed
    }
    board_[row][col] = player;
    return true; // Stone placed successfully
}

bool GomokuGame::checkWin(int & winner)
{
    for (int row = 0; row < boardSize_; ++row)
    {
        for (int col = 0; col < boardSize_; ++col)
        {
            if (board_[row][col] != currentPlayer_)
            {
                continue;
            }

            if (hasFiveInDirection(board_, boardSize_, currentPlayer_, row, col, 0, 1)
                || hasFiveInDirection(board_, boardSize_, currentPlayer_, row, col, 1, 0)
                || hasFiveInDirection(board_, boardSize_, currentPlayer_, row, col, 1, 1)
                || hasFiveInDirection(board_, boardSize_, currentPlayer_, row, col, 1, -1))
            {
                winner = currentPlayer_;
                return true;
            }
        }
    }

    return false;
}

void GomokuGame::setCurrentPlayer(int player)
{
    currentPlayer_ = player;
}

int GomokuGame::getCurrentPlayer() const
{
    return currentPlayer_;
}

void GomokuGame::switchPlayer()
{
    currentPlayer_ = currentPlayer_ == 1 ? 2 : 1;
}

bool GomokuGame::isBoardFull() const
{
    for (const auto& row : board_)
    {
        for (int cell : row)
        {
            if (cell == 0)
            {
                return false;
            }
        }
    }
    return true;
}

json GomokuGame::serialize() const
{
    return board_;
}

std::vector<std::vector<int>> GomokuGame::getBoard() const
{
    return board_;
}
