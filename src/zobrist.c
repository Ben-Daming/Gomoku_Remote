#include "../include/zobrist.h"
#include <stdlib.h>
#include <time.h>

uint64_t zobrist_table[BOARD_SIZE][BOARD_SIZE][2];
uint64_t zobrist_player;

// 简单的64位随机数生成器
static uint64_t rand64() {
    uint64_t r1 = (uint64_t)rand();
    uint64_t r2 = (uint64_t)rand();
    uint64_t r3 = (uint64_t)rand();
    uint64_t r4 = (uint64_t)rand();
    return r1 | (r2 << 16) | (r3 << 32) | (r4 << 48);
}

void initZobrist() {
    srand(123456789); // 固定种子便于debug

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            zobrist_table[i][j][0] = rand64(); // 黑子
            zobrist_table[i][j][1] = rand64(); // 白子
        }
    }
    zobrist_player = rand64(); // 当前玩家
}


// 遍历棋盘以计算初始哈希值
// 注意：此方法较慢，仅在根节点或初始化时使用
uint64_t calculateZobristHash(const BitBoardState* board, Player currentPlayer) {
    uint64_t hash = 0;
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        // 检查黑子
        Line b_row = board->black.rows[i];
        for (int j = 0; j < BOARD_SIZE; j++) {
            if ((b_row >> j) & 1) {
                hash ^= zobrist_table[i][j][0];
            }
        }
        
        // 检查白子
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
