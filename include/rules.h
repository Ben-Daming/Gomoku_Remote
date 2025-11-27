#ifndef RULES_H
#define RULES_H

#include "types.h"

// 落子合法性检查，返回的不同错误码
#define VALID_MOVE 0
#define ERR_OUT_OF_BOUNDS 1
#define ERR_OCCUPIED 2
#define ERR_FORBIDDEN_33 3
#define ERR_FORBIDDEN_44 4
#define ERR_FORBIDDEN_OVERLINE 5

// 三进制哈希用的棋型常量
// 0: 空，1: 黑子，2: 白子（通常哈希不使用，但基数为3，方便搞其他规则扩展）
// 棋型通常为5或6长度。
// 空->0，黑子->1，白子断开棋型，遇到白子循环终止。
// 基数：1, 3, 9, 27, 81, 243...

// 活三：011100 (117), 001110 (39), 011010 (111), 010110 (93)
#define PATTERN_HUO_THREE_1  117  // 011100
#define PATTERN_HUO_THREE_2  39   // 001110
#define PATTERN_HUO_THREE_3  111  // 011010
#define PATTERN_HUO_THREE_4  93   // 010110

// 冲四：11011 (112), 10111 (94), 11101 (118)
#define PATTERN_CHONG_FOUR_1 112 // 11011
#define PATTERN_CHONG_FOUR_2 94  // 10111
#define PATTERN_CHONG_FOUR_3 118 // 11101

int checkValidMove(const GameState *game, int row, int col);
int checkWin(const GameState *game); // 返回胜者（PLAYER_BLACK/WHITE），无胜者返回0
int isForbidden(const GameState *game, int row, int col);

#endif
