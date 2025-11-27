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

// 二进制位运算用的棋型常量
// 0: 空，1: 黑子
// 棋型通常为5或6长度。
// index 0 为最低位。

// 活三 (长度6)
// _ ● ● ● _ _ -> 011100 (LSB at left 0) -> 2^1 + 2^2 + 2^3 = 14 (0x0E)
// _ _ ● ● ● _ -> 001110 -> 2^2 + 2^3 + 2^4 = 28 (0x1C)
// _ ● _ ● ● _ -> 010110 -> 2^1 + 2^3 + 2^4 = 26 (0x1A)
// _ ● ● _ ● _ -> 011010 -> 2^1 + 2^2 + 2^4 = 22 (0x16)
#define PATTERN_HUO_THREE_1  0x0E // 011100 (Key at index 4)
#define PATTERN_HUO_THREE_2  0x1C // 001110 (Key at index 1)
#define PATTERN_HUO_THREE_3  0x1A // 010110 (Key at index 2)
#define PATTERN_HUO_THREE_4  0x16 // 011010 (Key at index 3)

// 冲四 (长度5)
// ● ● _ ● ● -> 11011 -> 1+2+8+16 = 27 (0x1B)
// ● _ ● ● ● -> 10111 -> 1+4+8+16 = 29 (0x1D)
// ● ● ● _ ● -> 11101 -> 1+2+4+16 = 23 (0x17)
#define PATTERN_CHONG_FOUR_1 0x1B // 11011 (Key at index 2)
#define PATTERN_CHONG_FOUR_2 0x1D // 10111 (Key at index 1)
#define PATTERN_CHONG_FOUR_3 0x17 // 11101 (Key at index 3)

int checkValidMove(const GameState *game, int row, int col);
int checkWin(const GameState *game); // 返回胜者（PLAYER_BLACK/WHITE），无胜者返回0
int isForbidden(const GameState *game, int row, int col);

#endif
