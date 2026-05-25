#ifndef GOMOKUGAME_H
#define GOMOKUGAME_H

#include <vector>
#include <iostream>
#include <string>
#include <sstream>

class GomokuGame
{

public:
    // 黑子为1，白子为2
    GomokuGame(int boardSize = 32) : boardSize_(boardSize), currentPlayer_(1) { initBoard(); }
    ~GomokuGame() = default;

    // 初始化棋盘
    void initBoard();
    // 重置棋盘
    void resetGame();
    // 检查是否有人获胜
    bool checkWin(int &winner);
    // 落子
    bool placeStone(int row, int col, int player);
    // 设置当前玩家
    void setCurrentPlayer(int player) ;

    std::string serialize();

private:
    std::vector<std::vector<int>> board_;
    int boardSize_;
    int currentPlayer_;
};


#endif // GOMOKUGAME_H

