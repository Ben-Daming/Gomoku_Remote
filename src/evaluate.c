#include "../include/evaluate.h"
#include <stdlib.h>

// Helper macro for population count (number of set bits)
#define POPCOUNT(x) __builtin_popcount(x)
#define ABS(x) ((x) < 0 ? -(x) : (x))

// --- Core Evaluation Kernel (Bitwise SWAR) ---
// Based on AI Migration Plan Step 2.1
int evaluateLine(Line me, Line enemy, int length) {
    int score = 0;
    // Empty spots are those not occupied by me or enemy
    // Mask with length to ensure we only care about the valid bits
    Line mask = (1 << length) - 1;
    Line empty = ~(me | enemy) & mask;

    // --- 1. Five in a Row (11111) ---
    Line m5 = me & (me >> 1) & (me >> 2) & (me >> 3) & (me >> 4);
    if (m5) return SCORE_FIVE;

    // --- 2. Four in a Row (1111) ---
    Line m4 = me & (me >> 1) & (me >> 2) & (me >> 3);
    
    // Live Four (011110): Left Empty & Four & Right Empty
    Line live4 = (empty << 1) & m4 & (empty >> 4);
    if (live4) return SCORE_LIVE_4;

    // Rush Four (011112 or 211110)
    // Either Left Empty OR Right Empty (excluding Live Four)
    Line rush4 = ((empty << 1) & m4) | (m4 & (empty >> 4));
    // Note: rush4 includes live4 cases if we didn't return above.
    // Since we returned, rush4 here strictly means "blocked on one side".

    // Jump Rush Four (10111, 11011, 11101)
    Line jump4_1 = me & (empty >> 1) & (me >> 2) & (me >> 3) & (me >> 4); // 11101
    Line jump4_2 = me & (me >> 1) & (empty >> 2) & (me >> 3) & (me >> 4); // 11011
    Line jump4_3 = me & (me >> 1) & (me >> 2) & (empty >> 3) & (me >> 4); // 10111

    // if (rush4) score += POPCOUNT(rush4) * SCORE_RUSH_4;
    // if (jump4_1) score += POPCOUNT(jump4_1) * SCORE_RUSH_4;
    // if (jump4_2) score += POPCOUNT(jump4_2) * SCORE_RUSH_4;
    // if (jump4_3) score += POPCOUNT(jump4_3) * SCORE_RUSH_4;
    score += POPCOUNT(rush4 | jump4_1 | jump4_2 | jump4_3) * 8000;
    // --- 3. Three in a Row (111) ---
    Line m3 = me & (me >> 1) & (me >> 2);
    // Exclude Threes that are part of Fours
    Line m3_clean = m3 & ~(m4 | (m4 << 1));

    // Live Three (01110): Left Empty & Right Empty
    Line live3 = (empty << 1) & m3_clean & (empty >> 3);
    // Rush Three: Left Empty XOR Right Empty
    Line rush3 = ((empty << 1) & m3_clean) ^ (m3_clean & (empty >> 3));

    score += POPCOUNT(live3) * SCORE_LIVE_3;
    score += POPCOUNT(rush3) * SCORE_RUSH_3;
    
    // --- 4. Jump Three (1011, 1101) ---
    Line jump3_a = me & (empty >> 1) & (me >> 2) & (me >> 3); // 1101
    Line jump3_b = me & (me >> 1) & (empty >> 2) & (me >> 3); // 1011
    // Exclude Jump Threes that are part of Jump Fours
    jump3_a &= ~(jump4_1 | (jump4_2 << 1));
    jump3_b &= ~(jump4_2 | (jump4_3 << 1));

    // Live Jump Three (010110, 011010): Needs both ends empty
    Line live_jump3_a = (empty << 1) & jump3_a & (empty >> 3);
    Line live_jump3_b = (empty << 1) & jump3_b & (empty >> 3);

    score += POPCOUNT(live_jump3_a | live_jump3_b) * SCORE_JUMP_LIVE_3;

    // --- 5. Two in a Row (11) ---
    Line m2 = me & (me >> 1);
    // Exclude Twos that are part of Threes
    // m3 covers i..i+2. m2 covers i..i+1.

    // Live Two (0110): Left Empty & Right Empty
    Line live2 = (empty << 1) & m2 & (empty >> 2);

    // Rush Two (0112, 2110)
    Line rush2_left = (empty << 1) & m2 & (enemy >> 2);
    Line rush2_right = (enemy << 1) & m2 & (empty >> 2);
    Line rush2 = rush2_left | rush2_right;

    // Strong Live Two (001100)
    Line strong_live2 = (empty << 2) & live2 & (empty >> 3);
    score += POPCOUNT(strong_live2) * SCORE_STRONG_LIVE_2_ADDITION;
    score += POPCOUNT(live2) * SCORE_LIVE_2;
    score += POPCOUNT(rush2) * SCORE_RUSH_2;

    return score;
}

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
