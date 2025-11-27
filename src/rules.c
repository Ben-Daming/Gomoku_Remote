#include "../include/rules.h"
#include <stdio.h>

// 方向：水平，垂直，对角线 \, 对角线 /
static int dr[] = {0, 1, 1, 1};
static int dc[] = {1, 0, 1, -1};

// 辅助函数：判断位置是否在棋盘里
static int isValidPos(int r, int c) {
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

// 辅助函数：统计某一方向上连续的棋子数
static int countLine(const GameState *game, int r, int c, int dirIdx, Player p) {
    int count = 1;
    // 正向统计
    for (int i = 1; i < 6; i++) {
        int nr = r + dr[dirIdx] * i;
        int nc = c + dc[dirIdx] * i;
        if (isValidPos(nr, nc) && game->board[nr][nc] == (CellState)p) count++;
        else break;
    }
    // 反向统计
    for (int i = 1; i < 6; i++) {
        int nr = r - dr[dirIdx] * i;
        int nc = c - dc[dirIdx] * i;
        if (isValidPos(nr, nc) && game->board[nr][nc] == (CellState)p) count++;
        else break;
    }
    return count;
}

// 检查胜利条件，返回胜利者 (PLAYER_BLACK/WHITE) 或 0
int checkWin(const GameState *game) {
    if (game->moveCount == 0) return 0;
    
    int r = game->lastMove.row;
    int c = game->lastMove.col;
    CellState cell = game->board[r][c];
    if (cell == EMPTY) return 0;
    Player p = (cell == BLACK) ? PLAYER_BLACK : PLAYER_WHITE;

    for (int i = 0; i < 4; i++) {
        int count = countLine(game, r, c, i, p);
        if (p == PLAYER_WHITE) {
            if (count >= 5) return p;
        } else {
            // 黑方
            if (game->ruleType == RULE_STANDARD) {
                if (count == 5) return p;
                // 超过五连（长连）在标准规则下黑方不胜，为禁手。
                // 但如果走到这里，说明已经下子。
                // 如果 checkValidMove 正常工作，这里不应发生。
            } else {
                if (count >= 5) return p;
            }
        }
    }
    return 0;
}



// TODO: 实现完整的三三和四四禁手检测。

// 提前存好3的基数
static const int base[15] = {1, 3, 9, 27, 81, 243, 729, 2187, 6561, 19683, 59049, 177147, 531441, 1594323, 4782969};

// 递归，提前声明
int isForbidden(const GameState *game, int row, int col);

// 判断是否为连起来的有效四
static int isHuoFour(const GameState *game, int i, int j, int dx, int dy) {
    int num;
    int now_i, now_j;
    int l_edge = 1;
    int r_edge = 1;

    // 正向算连珠数目
    for (now_i = i, now_j = j, num = 0; 
         isValidPos(now_i, now_j) && game->board[now_i][now_j] == BLACK; 
         now_i += dx, now_j += dy, num++);    

    if (!isValidPos(now_i, now_j) || game->board[now_i][now_j] == WHITE) {
        l_edge = 0; 
    }

    // 反向算连珠数目
    for (now_i = i - dx, now_j = j - dy; 
         isValidPos(now_i, now_j) && game->board[now_i][now_j] == BLACK; 
         now_i -= dx, now_j -= dy, num++);    

    if (!isValidPos(now_i, now_j) || game->board[now_i][now_j] == WHITE) {
        r_edge = 0; 
    }

    if (num != 4 || (!l_edge && !r_edge)) 
        return 0;

    if ((r_edge) || (l_edge)) 
        return 1;
    else 
        return 0;
}

// 判断是否为跳跃型的三种冲四
static int isChongFour(const GameState *game, int i, int j, int dx, int dy) {
    int value, key_i, key_j, now_i, now_j, search_i, search_j;
    int num = 0;
    int index1, index2;
    index2 = 0;

    for (now_i = i, now_j = j; index2 < 5 && isValidPos(now_i, now_j); 
         now_i -= dx, now_j -= dy, index2++) { 
        value = 0;
        search_i = now_i;
        search_j = now_j;
        for (index1 = 0; index1 < 5 && isValidPos(search_i, search_j); 
             index1++, search_i += dx, search_j += dy) {
            if (game->board[search_i][search_j] == WHITE) 
                break;
            else if (game->board[search_i][search_j] == BLACK)
                value += 1 * base[index1];
            else // 空位
                value += 0;
        } 
        if (index1 == 5) {
            if (value == PATTERN_CHONG_FOUR_1) {
                key_i = now_i + 2*dx;
                key_j = now_j + 2*dy;
                num++;   
            } else if (value == PATTERN_CHONG_FOUR_2) {
                key_i = now_i + 3*dx;
                key_j = now_j + 3*dy;
                num++;
            } else if (value == PATTERN_CHONG_FOUR_3) {
                key_i = now_i + 1*dx;
                key_j = now_j + 1*dy;
                num++;
            }
        }
    }
    return num;
}

// 判断是否为活三
static int isHuoThree(const GameState *game, int i, int j, int dx, int dy) {
    int value, key_i, key_j, now_i, now_j, end_i, end_j, search_i, search_j;
    int index1;     
    int index2 = 0; 

    for (now_i = i - dx, now_j = j - dy; index2 < 4 && isValidPos(now_i, now_j); 
         now_i -= dx, now_j -= dy, index2++) {
        value = 0;
        search_i = now_i - dx;
        search_j = now_j - dy;
        // 检查左侧是否被黑子堵住（否则会变成长连）
        if (isValidPos(search_i, search_j) && game->board[search_i][search_j] == BLACK) 
            continue; 

        search_i = now_i;
        search_j = now_j;
        for (index1 = 0; index1 < 6 && isValidPos(search_i, search_j); 
             index1++, search_i += dx, search_j += dy) {
            if (game->board[search_i][search_j] == WHITE) 
                break;
            else if (game->board[search_i][search_j] == BLACK)
                value += 1 * base[index1];
            else
                value += 0;
        } 

        end_i = now_i + index1*dx;
        end_j = now_j + index1*dy;

        if(index1 < 6) continue;
        // 检查右侧是否被黑子堵住
        else if(!isValidPos(end_i, end_j) || game->board[end_i][end_j] != BLACK) {
            switch (value) {
                case PATTERN_HUO_THREE_1:
                    key_i = now_i + dx*1;  
                    key_j = now_j + dy*1;  
                    if (!isForbidden(game, key_i, key_j)) 
                        return 1;  
                    break;
                case PATTERN_HUO_THREE_2:
                    key_i = now_i + dx*4;  
                    key_j = now_j + dy*4;  
                    if (!isForbidden(game, key_i, key_j)) 
                        return 1;
                    break;
                case PATTERN_HUO_THREE_3:
                    key_i = now_i + dx*2;  
                    key_j = now_j + dy*2;  
                    if (!isForbidden(game, key_i, key_j)) 
                        return 1;
                    break;
                case PATTERN_HUO_THREE_4:
                    key_i = now_i + dx*3;  
                    key_j = now_j + dy*3;  
                    if (!isForbidden(game, key_i, key_j)) 
                        return 1; 
                    break;
            }
        } 
    }
    return 0;
}

// 判断是否为长连
static int isOverline(const GameState *game, int r, int c) {
    for (int i = 0; i < 4; i++) {
        if (countLine(game, r, c, i, PLAYER_BLACK) > 5) return 1;
    }
    return 0;
}

// 禁手检测（仅标准规则下黑方需要）
int isForbidden(const GameState *game, int row, int col) {
    if (game->ruleType != RULE_STANDARD) return 0;
    // 只有黑方有禁手
    // isHuoThree 递归调用时是检测潜在落子是否为禁手。
    // currentPlayer 检查在模拟时可能有陷阱。
    // 但 isForbidden 只会用于黑方落子。

    GameState *mutableGame = (GameState *)game;
    CellState original = mutableGame->board[row][col];
    mutableGame->board[row][col] = BLACK;

    int forbidden = 0;

    // 1. 长连
    if (isOverline(mutableGame, row, col)) {
        forbidden = ERR_FORBIDDEN_OVERLINE;
    } else {
        int num_four = 0;
        int num_three = 0;

        // 检查四个方向
        for (int i = 0; i < 4; i++) {
            if (isHuoFour(mutableGame, row, col, dr[i], dc[i])) num_four++;
            num_four += isChongFour(mutableGame, row, col, dr[i], dc[i]);
            num_three += isHuoThree(mutableGame, row, col, dr[i], dc[i]);
        }

        if (num_four >= 2) forbidden = ERR_FORBIDDEN_44;
        else if (num_three >= 2) forbidden = ERR_FORBIDDEN_33;
    }
    
    mutableGame->board[row][col] = original; // 恢复原状态
    return forbidden;
}

// 检查落子是否合法
int checkValidMove(const GameState *game, int row, int col) {
    if (!isValidPos(row, col)) return ERR_OUT_OF_BOUNDS;
    if (game->board[row][col] != EMPTY) return ERR_OCCUPIED;

    if (game->currentPlayer == PLAYER_BLACK && game->ruleType == RULE_STANDARD) {
        int forbidden = isForbidden(game, row, col);
        if (forbidden) return forbidden;
    }

    return VALID_MOVE;
}
