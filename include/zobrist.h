#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <stdint.h>
#include "types.h"

// Zobrist Hashing Tables
// [row][col][player] (player: 0=Black, 1=White)
extern uint64_t zobrist_table[BOARD_SIZE][BOARD_SIZE][2];
extern uint64_t zobrist_player; // XOR this when switching player

// Initialize Zobrist tables with random values
void initZobrist();

// Calculate initial hash for a board
uint64_t calculateZobristHash(const BitBoardState* board, Player currentPlayer);

#endif // ZOBRIST_H
