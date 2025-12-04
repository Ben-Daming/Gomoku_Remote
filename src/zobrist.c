#include "../include/zobrist.h"
#include <stdlib.h>
#include <time.h>

uint64_t zobrist_table[BOARD_SIZE][BOARD_SIZE][2];
uint64_t zobrist_player;

// Simple 64-bit random number generator
static uint64_t rand64() {
    uint64_t r1 = (uint64_t)rand();
    uint64_t r2 = (uint64_t)rand();
    uint64_t r3 = (uint64_t)rand();
    uint64_t r4 = (uint64_t)rand();
    return r1 | (r2 << 16) | (r3 << 32) | (r4 << 48);
}

void initZobrist() {
    srand(123456789); // Fixed seed for reproducibility

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            zobrist_table[i][j][0] = rand64(); // Black
            zobrist_table[i][j][1] = rand64(); // White
        }
    }
    zobrist_player = rand64();
}

uint64_t calculateZobristHash(const BitBoardState* board, Player currentPlayer) {
    uint64_t hash = 0;
    
    // Iterate over board to calculate initial hash
    // Note: This is slow, only used at root or initialization
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Check Black
        Line b_row = board->black.rows[i];
        for (int j = 0; j < BOARD_SIZE; j++) {
            if ((b_row >> j) & 1) {
                hash ^= zobrist_table[i][j][0];
            }
        }
        
        // Check White
        Line w_row = board->white.rows[i];
        for (int j = 0; j < BOARD_SIZE; j++) {
            if ((w_row >> j) & 1) {
                hash ^= zobrist_table[i][j][1];
            }
        }
    }

    if (currentPlayer == PLAYER_WHITE) {
        hash ^= zobrist_player;
    }

    return hash;
}
