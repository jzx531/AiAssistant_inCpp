#include "../include/aigame/GomokuGame.h"

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
    // Check rows
    for (int i = 0; i < boardSize_; i++)
    {
        int count = 0;
        for (int j = 0; j < boardSize_; j++)
        {
            if (board_[i][j] == currentPlayer_)
            {
                count++;
            }
            else
            {
                count = 0;
            }
            if (count == 5)
            {
                winner = currentPlayer_;
                return true;
            }
        }
    }

    // Check columns
    for (int i = 0; i < boardSize_; i++)
    {
        int count = 0;
        for (int j = 0; j < boardSize_; j++)
        {
            if (board_[j][i] == currentPlayer_)
            {
                count++;
            }
            else
            {
                count = 0;
            }
            if (count == 5)
            {
                winner = currentPlayer_;
                return true;
            }
        }
    }

    // Check diagonals
    int count = 0;
    for (int i = 0; i < boardSize_; i++)
    {
        if (board_[i][i] == currentPlayer_)
        {
            count++;
        }
        else
        {
            count = 0;
        }
        if (count == 5)
        {
            winner = currentPlayer_;
            return true;
        }
    }

    //check other diagonal
    count = 0;
    for (int i = 0; i < boardSize_; i++)
    {
        if (board_[i][boardSize_ - 1 - i] == currentPlayer_)
        {
            count++;
        }
        else
        {
            count = 0;
        }
        if (count == 5)
        {
            winner = currentPlayer_;
            return true;
        }
    }
    return false;
}

void GomokuGame::setCurrentPlayer(int player)
{
    currentPlayer_ = player;
}

std::string GomokuGame::serialize()
{
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < boardSize_; i++)
    {
        ss << "[";
        for (int j = 0; j < boardSize_; j++)
        {
            ss << board_[i][j];
            if (j < boardSize_ - 1)
            {
                ss << ",";
            }
        }
        ss << "]";
        if (i < boardSize_ - 1)
        {
            ss << ",";
        }
    }
    ss << "]";

    return ss.str();
}