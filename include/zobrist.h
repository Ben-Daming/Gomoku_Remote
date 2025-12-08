#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>
#include "types.h"

// Zobrist
// [row][col][player] (player: 0=Black, 1=White)
extern uint64_t zobrist_table[BOARD_SIZE][BOARD_SIZE][2];
extern uint64_t zobrist_player; // 当前玩家哈希

void initZobrist();

uint64_t calculateZobristHash(const BitBoardState* board, Player currentPlayer);

#endif 
