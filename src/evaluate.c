#include "../include/evaluate.h"
#include <stdlib.h>

// Helper macro for population count (number of set bits)
#define POPCOUNT(x) __builtin_popcount(x)
#define ABS(x) ((x) < 0 ? -(x) : (x))

// --- Core Evaluation Kernel (Bitwise SWAR) ---

int evaluateLine(Line me, Line enemy, int length) {
    int score = 0;
    // Empty spots are those not occupied by me or enemy
    // Mask with length to ensure we only care about the valid bits
    Line mask = (1 << length) - 1;
    Line valid = ~(me | enemy) & mask;

    //连2，连3，连4，连5
    Line m2 = me & (me >> 1); 
    Line m3 = m2 & (m2 >> 1);
    Line m4 = m3 & (m3 >> 1);
    Line m5 = m4 & (m4 >> 1);
    // m3 &= ~(m4 | (m4 << 1));//排除连四一部分
    m2 &= ~(m3 | (m3 << 1));
    if (m5) return SCORE_FIVE;
    // 先判断连二
    // 活二 (0110)
    Line live2 = (valid << 1) & m2 & (valid >> 2);
    // 冲二 (0112, 2110)
    Line rush2 = m2 & ((valid << 1) ^ (valid >> 2));
    // 强活二 (001100) 
    Line strong_live2 = (valid << 2) & live2 & (valid >> 3);

    score += POPCOUNT(strong_live2) * (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2);
    score += POPCOUNT(live2) * SCORE_LIVE_2;
    score += POPCOUNT(rush2) * SCORE_RUSH_2;

    // 活三 (01110)
    Line live3_raw = (valid << 1) & m3 & (valid >> 3);
    // 冲连三 (21110, 01112)
    Line rush3_raw = ((valid << 1) ^ (valid >> 3)) & m3;
    
    // 跳三(1101, 1011)
    Line jump3_a = me & (m2 >> 2) & (valid >> 1);//1101
    Line jump3_b = (me >> 3) & m2 & (valid >> 2);//1011  
    
    Line mask_0xxxx0 = (valid >> 4) & (valid << 1);
    Line mask_axxxxb = (valid >> 4) ^ (valid << 1); //两边有一个0

    // 活跳三(011010, 010110)
    Line live_jump3 = (jump3_a | jump3_b) & mask_0xxxx0;
    score += POPCOUNT(live_jump3) * (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2);

    // 活四 (011110)
    Line live4 = m4 & mask_0xxxx0;
    score += POPCOUNT(live4) * (SCORE_LIVE_4 - 2 * SCORE_RUSH_3);

    // 冲四 (211110, 011112)
    Line rush4 = m4 & mask_axxxxb;
    score += POPCOUNT(rush4) * (SCORE_RUSH_4 - SCORE_RUSH_3);

    // 跳四 (11101, 11011, 10111)
    Line jump4_1 = me & (valid >> 1) & (m3 >> 2); // 11101
    Line jump4_2 = m2 & (valid >> 2) & (m2 >> 3); // 11011
    Line jump4_3 = m3 & (valid >> 3) & (me >> 4); // 10111


    //消除假三
    Line live3 = live3_raw & ~(jump4_1 << 2) & ~jump4_3;
    Line rush3 = rush3_raw & ~(jump4_1 << 2) & ~jump4_3;

    score += POPCOUNT(live3) * SCORE_LIVE_3;
    score += POPCOUNT(rush3) * SCORE_RUSH_3;

    // Score Jump 4 (Rush 4 equivalent)
    score += POPCOUNT(jump4_1 | jump4_2 | jump4_3) * SCORE_RUSH_4;

    return score;
}

// --- Board Scanning Helpers ---
// Removed: getRow, getMainDiag, getAntiDiag are no longer needed
// as BitBoardState now maintains pre-calculated arrays for all directions.

// --- Full Board Evaluation ---


// int checkForbidden(int score) {
//     if (IS_FORBIDDEN_44(score)) return FORBIDDEN_44;
//     if (IS_FORBIDDEN_33(score)) return FORBIDDEN_33;
//     return FORBIDDEN_NONE;
// }


// --- Board Scanning Helpers ---
// Removed: getRow, getMainDiag, getAntiDiag are no longer needed
// as BitBoardState now maintains pre-calculated arrays for all directions.

// --- Full Board Evaluation ---

int evaluateBoard(const BitBoardState *bitBoard, Player player) {
    int total_score = 0;
    const PlayerBitBoard *my_board = (player == PLAYER_BLACK) ? &bitBoard->black : &bitBoard->white;
    const PlayerBitBoard *opp_board = (player == PLAYER_BLACK) ? &bitBoard->white : &bitBoard->black;
    Line me, enemy;

    // 1. Vertical (Columns)
    for (int col = 0; col < BOARD_SIZE; col++) {
        me = my_board->cols[col];
        enemy = opp_board->cols[col];
        total_score += evaluateLine(me, enemy, BOARD_SIZE);
    }

    // 2. Horizontal (Rows)
    for (int row = 0; row < BOARD_SIZE; row++) {
        me = my_board->rows[row];
        enemy = opp_board->rows[row];
        total_score += evaluateLine(me, enemy, BOARD_SIZE);
    }

    // 3. Main Diagonals (\)
    // Index: row - col + 14. Range: 0 to 28.
    // Length: 15 - |14 - index|
    // Only scan diagonals with length >= 5
    for (int i = 0; i < BOARD_SIZE * 2 - 1; i++) {
        int len = BOARD_SIZE - ABS(BOARD_SIZE - 1 - i);
        if (len < 5) continue;
        
        me = my_board->diag1[i];
        enemy = opp_board->diag1[i];
        total_score += evaluateLine(me, enemy, len);
    }

    // 4. Anti Diagonals (/)
    // Index: row + col. Range: 0 to 28.
    // Length: 15 - |14 - index|
    for (int i = 0; i < BOARD_SIZE * 2 - 1; i++) {
        int len = BOARD_SIZE - ABS(BOARD_SIZE - 1 - i);
        if (len < 5) continue;

        me = my_board->diag2[i];
        enemy = opp_board->diag2[i];
        total_score += evaluateLine(me, enemy, len);
    }

    return total_score;
}

int evaluate(const BitBoardState *bitBoard) {
    int blackScore = evaluateBoard(bitBoard, PLAYER_BLACK);
    int whiteScore = evaluateBoard(bitBoard, PLAYER_WHITE);
    return blackScore - whiteScore;
}
