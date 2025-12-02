#include "../include/evaluate.h"
#include <stdlib.h>

// Helper macro for population count (number of set bits)
#define POPCOUNT(x) __builtin_popcount(x)
#define POPCOUNT64(x) __builtin_popcountll(x)
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

// Parallel Evaluation of 4 Lines (128-bit SWAR via struct)
Lines4 evaluateLines4(Lines4 me, Lines4 enemy, Lines4 mask) {
    Lines4 score = {0, 0};
    
    Lines4 valid;
    valid.low = ~(me.low | enemy.low) & mask.low;
    valid.high = ~(me.high | enemy.high) & mask.high;

    // --- Parallel Logic (Identical to evaluateLine but 2x 64-bit) ---
    
    //连2，连3，连4，连5
    Lines4 m2, m3, m4, m5;
    
    m2.low = me.low & (me.low >> 1);
    m2.high = me.high & (me.high >> 1);
    
    m3.low = m2.low & (m2.low >> 1);
    m3.high = m2.high & (m2.high >> 1);
    
    m4.low = m3.low & (m3.low >> 1);
    m4.high = m3.high & (m3.high >> 1);
    
    m5.low = m4.low & (m4.low >> 1);
    m5.high = m4.high & (m4.high >> 1);

    // Check for Win immediately (Any m5 non-zero)
    if (m5.low | m5.high) {
        if (m5.low & 0xFFFFFFFFULL) score.low |= (unsigned long long)SCORE_FIVE;
        if (m5.low & 0xFFFFFFFF00000000ULL) score.low |= ((unsigned long long)SCORE_FIVE << 32);
        if (m5.high & 0xFFFFFFFFULL) score.high |= (unsigned long long)SCORE_FIVE;
        if (m5.high & 0xFFFFFFFF00000000ULL) score.high |= ((unsigned long long)SCORE_FIVE << 32);
        return score;
    }

    // m3 &= ~(m4 | (m4 << 1));//排除连四一部分
    m2.low &= ~(m3.low | (m3.low << 1));
    m2.high &= ~(m3.high | (m3.high << 1));

    // 先判断连二
    // 活二 (0110)
    Lines4 live2;
    live2.low = (valid.low << 1) & m2.low & (valid.low >> 2);
    live2.high = (valid.high << 1) & m2.high & (valid.high >> 2);
    
    // 冲二 (0112, 2110)
    Lines4 rush2;
    rush2.low = m2.low & ((valid.low << 1) ^ (valid.low >> 2));
    rush2.high = m2.high & ((valid.high << 1) ^ (valid.high >> 2));
    
    // 强活二 (001100) 
    Lines4 strong_live2;
    strong_live2.low = (valid.low << 2) & live2.low & (valid.low >> 3);
    strong_live2.high = (valid.high << 2) & live2.high & (valid.high >> 3);

    score.low += (unsigned long long)POPCOUNT64(strong_live2.low & 0xFFFFFFFF) * (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2);
    score.low += ((unsigned long long)POPCOUNT64(strong_live2.low >> 32) * (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2)) << 32;
    score.high += (unsigned long long)POPCOUNT64(strong_live2.high & 0xFFFFFFFF) * (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2);
    score.high += ((unsigned long long)POPCOUNT64(strong_live2.high >> 32) * (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2)) << 32;

    score.low += (unsigned long long)POPCOUNT64(live2.low & 0xFFFFFFFF) * SCORE_LIVE_2;
    score.low += ((unsigned long long)POPCOUNT64(live2.low >> 32) * SCORE_LIVE_2) << 32;
    score.high += (unsigned long long)POPCOUNT64(live2.high & 0xFFFFFFFF) * SCORE_LIVE_2;
    score.high += ((unsigned long long)POPCOUNT64(live2.high >> 32) * SCORE_LIVE_2) << 32;

    score.low += (unsigned long long)POPCOUNT64(rush2.low & 0xFFFFFFFF) * SCORE_RUSH_2;
    score.low += ((unsigned long long)POPCOUNT64(rush2.low >> 32) * SCORE_RUSH_2) << 32;
    score.high += (unsigned long long)POPCOUNT64(rush2.high & 0xFFFFFFFF) * SCORE_RUSH_2;
    score.high += ((unsigned long long)POPCOUNT64(rush2.high >> 32) * SCORE_RUSH_2) << 32;

    // 活三 (01110)
    Lines4 live3_raw;
    live3_raw.low = (valid.low << 1) & m3.low & (valid.low >> 3);
    live3_raw.high = (valid.high << 1) & m3.high & (valid.high >> 3);
    
    // 冲连三 (21110, 01112)
    Lines4 rush3_raw;
    rush3_raw.low = ((valid.low << 1) ^ (valid.low >> 3)) & m3.low;
    rush3_raw.high = ((valid.high << 1) ^ (valid.high >> 3)) & m3.high;
    
    // 跳三(1101, 1011)
    Lines4 jump3_a, jump3_b;
    jump3_a.low = me.low & (m2.low >> 2) & (valid.low >> 1);
    jump3_a.high = me.high & (m2.high >> 2) & (valid.high >> 1);
    
    jump3_b.low = (me.low >> 3) & m2.low & (valid.low >> 2);
    jump3_b.high = (me.high >> 3) & m2.high & (valid.high >> 2);
    
    Lines4 mask_0xxxx0, mask_axxxxb;
    mask_0xxxx0.low = (valid.low >> 4) & (valid.low << 1);
    mask_0xxxx0.high = (valid.high >> 4) & (valid.high << 1);
    
    mask_axxxxb.low = (valid.low >> 4) ^ (valid.low << 1);
    mask_axxxxb.high = (valid.high >> 4) ^ (valid.high << 1);

    // 活跳三(011010, 010110)
    Lines4 live_jump3;
    live_jump3.low = (jump3_a.low | jump3_b.low) & mask_0xxxx0.low;
    live_jump3.high = (jump3_a.high | jump3_b.high) & mask_0xxxx0.high;
    
    score.low += (unsigned long long)POPCOUNT64(live_jump3.low & 0xFFFFFFFF) * (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2);
    score.low += ((unsigned long long)POPCOUNT64(live_jump3.low >> 32) * (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2)) << 32;
    score.high += (unsigned long long)POPCOUNT64(live_jump3.high & 0xFFFFFFFF) * (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2);
    score.high += ((unsigned long long)POPCOUNT64(live_jump3.high >> 32) * (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2)) << 32;

    // 活四 (011110)
    Lines4 live4;
    live4.low = m4.low & mask_0xxxx0.low;
    live4.high = m4.high & mask_0xxxx0.high;
    
    score.low += (unsigned long long)POPCOUNT64(live4.low & 0xFFFFFFFF) * (SCORE_LIVE_4 - 2 * SCORE_RUSH_3);
    score.low += ((unsigned long long)POPCOUNT64(live4.low >> 32) * (SCORE_LIVE_4 - 2 * SCORE_RUSH_3)) << 32;
    score.high += (unsigned long long)POPCOUNT64(live4.high & 0xFFFFFFFF) * (SCORE_LIVE_4 - 2 * SCORE_RUSH_3);
    score.high += ((unsigned long long)POPCOUNT64(live4.high >> 32) * (SCORE_LIVE_4 - 2 * SCORE_RUSH_3)) << 32;

    // 冲四 (211110, 011112)
    Lines4 rush4;
    rush4.low = m4.low & mask_axxxxb.low;
    rush4.high = m4.high & mask_axxxxb.high;
    
    score.low += (unsigned long long)POPCOUNT64(rush4.low & 0xFFFFFFFF) * (SCORE_RUSH_4 - SCORE_RUSH_3);
    score.low += ((unsigned long long)POPCOUNT64(rush4.low >> 32) * (SCORE_RUSH_4 - SCORE_RUSH_3)) << 32;
    score.high += (unsigned long long)POPCOUNT64(rush4.high & 0xFFFFFFFF) * (SCORE_RUSH_4 - SCORE_RUSH_3);
    score.high += ((unsigned long long)POPCOUNT64(rush4.high >> 32) * (SCORE_RUSH_4 - SCORE_RUSH_3)) << 32;

    // 跳四 (11101, 11011, 10111)
    Lines4 jump4_1, jump4_2, jump4_3;
    jump4_1.low = me.low & (valid.low >> 1) & (m3.low >> 2);
    jump4_1.high = me.high & (valid.high >> 1) & (m3.high >> 2);
    
    jump4_2.low = m2.low & (valid.low >> 2) & (m2.low >> 3);
    jump4_2.high = m2.high & (valid.high >> 2) & (m2.high >> 3);
    
    jump4_3.low = m3.low & (valid.low >> 3) & (me.low >> 4);
    jump4_3.high = m3.high & (valid.high >> 3) & (me.high >> 4);


    //消除假三
    Lines4 live3, rush3;
    live3.low = live3_raw.low & ~(jump4_1.low << 2) & ~jump4_3.low;
    live3.high = live3_raw.high & ~(jump4_1.high << 2) & ~jump4_3.high;
    
    rush3.low = rush3_raw.low & ~(jump4_1.low << 2) & ~jump4_3.low;
    rush3.high = rush3_raw.high & ~(jump4_1.high << 2) & ~jump4_3.high;

    score.low += (unsigned long long)POPCOUNT64(live3.low & 0xFFFFFFFF) * SCORE_LIVE_3;
    score.low += ((unsigned long long)POPCOUNT64(live3.low >> 32) * SCORE_LIVE_3) << 32;
    score.high += (unsigned long long)POPCOUNT64(live3.high & 0xFFFFFFFF) * SCORE_LIVE_3;
    score.high += ((unsigned long long)POPCOUNT64(live3.high >> 32) * SCORE_LIVE_3) << 32;

    score.low += (unsigned long long)POPCOUNT64(rush3.low & 0xFFFFFFFF) * SCORE_RUSH_3;
    score.low += ((unsigned long long)POPCOUNT64(rush3.low >> 32) * SCORE_RUSH_3) << 32;
    score.high += (unsigned long long)POPCOUNT64(rush3.high & 0xFFFFFFFF) * SCORE_RUSH_3;
    score.high += ((unsigned long long)POPCOUNT64(rush3.high >> 32) * SCORE_RUSH_3) << 32;

    // Score Jump 4 (Rush 4 equivalent)
    unsigned long long jump4_low = jump4_1.low | jump4_2.low | jump4_3.low;
    unsigned long long jump4_high = jump4_1.high | jump4_2.high | jump4_3.high;
    
    score.low += (unsigned long long)POPCOUNT64(jump4_low & 0xFFFFFFFF) * SCORE_RUSH_4;
    score.low += ((unsigned long long)POPCOUNT64(jump4_low >> 32) * SCORE_RUSH_4) << 32;
    score.high += (unsigned long long)POPCOUNT64(jump4_high & 0xFFFFFFFF) * SCORE_RUSH_4;
    score.high += ((unsigned long long)POPCOUNT64(jump4_high >> 32) * SCORE_RUSH_4) << 32;

    return score;
}

// --- Board Scanning Helpers ---

// Parallel Evaluation of 2 Lines (Batching)
// Stride = 32 bits (15 data + 17 guard) to fit 2 lines in 64 bits
// Line 1 at bit 0, Line 2 at bit 32
unsigned long long evaluateLines2(Line me1, Line enemy1, int len1, Line me2, Line enemy2, int len2) {
    unsigned long long score = 0;
    
    // Pack inputs into 64-bit integers
    // Line 1: Bits 0-14
    // Line 2: Bits 32-46
    unsigned long long me = (unsigned long long)me1 | ((unsigned long long)me2 << 32);
    unsigned long long enemy = (unsigned long long)enemy1 | ((unsigned long long)enemy2 << 32);
    
    // Create masks
    unsigned long long mask1 = (1ULL << len1) - 1;
    unsigned long long mask2 = (1ULL << len2) - 1;
    unsigned long long mask = mask1 | (mask2 << 32);
    
    unsigned long long valid = ~(me | enemy) & mask;

    // --- Parallel Logic (Identical to evaluateLine but 64-bit) ---
    
    //连2，连3，连4，连5
    unsigned long long m2 = me & (me >> 1); 
    unsigned long long m3 = m2 & (m2 >> 1);
    unsigned long long m4 = m3 & (m3 >> 1);
    unsigned long long m5 = m4 & (m4 >> 1);

    // Check for Win immediately (Any m5 non-zero)
    if (m5) {
        // If m5 has bits in lower 32, Line 1 wins. If upper 32, Line 2 wins.
        unsigned long long win_score = 0;
        if (m5 & 0xFFFFFFFFULL) win_score |= (unsigned long long)SCORE_FIVE;
        if (m5 & 0xFFFFFFFF00000000ULL) win_score |= ((unsigned long long)SCORE_FIVE << 32);
        return win_score;
    }

    // m3 &= ~(m4 | (m4 << 1));//排除连四一部分
    m2 &= ~(m3 | (m3 << 1));

    // 先判断连二
    // 活二 (0110)
    unsigned long long live2 = (valid << 1) & m2 & (valid >> 2);
    // 冲二 (0112, 2110)
    unsigned long long rush2 = m2 & ((valid << 1) ^ (valid >> 2));
    // 强活二 (001100) 
    unsigned long long strong_live2 = (valid << 2) & live2 & (valid >> 3);

    score += (unsigned long long)POPCOUNT(strong_live2 & 0xFFFFFFFF) * (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2);
    score += ((unsigned long long)POPCOUNT(strong_live2 >> 32) * (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2)) << 32;

    score += (unsigned long long)POPCOUNT(live2 & 0xFFFFFFFF) * SCORE_LIVE_2;
    score += ((unsigned long long)POPCOUNT(live2 >> 32) * SCORE_LIVE_2) << 32;

    score += (unsigned long long)POPCOUNT(rush2 & 0xFFFFFFFF) * SCORE_RUSH_2;
    score += ((unsigned long long)POPCOUNT(rush2 >> 32) * SCORE_RUSH_2) << 32;

    // 活三 (01110)
    unsigned long long live3_raw = (valid << 1) & m3 & (valid >> 3);
    // 冲连三 (21110, 01112)
    unsigned long long rush3_raw = ((valid << 1) ^ (valid >> 3)) & m3;
    
    // 跳三(1101, 1011)
    unsigned long long jump3_a = me & (m2 >> 2) & (valid >> 1);//1101
    unsigned long long jump3_b = (me >> 3) & m2 & (valid >> 2);//1011  
    
    unsigned long long mask_0xxxx0 = (valid >> 4) & (valid << 1);
    unsigned long long mask_axxxxb = (valid >> 4) ^ (valid << 1); //两边有一个0

    // 活跳三(011010, 010110)
    unsigned long long live_jump3 = (jump3_a | jump3_b) & mask_0xxxx0;
    score += (unsigned long long)POPCOUNT(live_jump3 & 0xFFFFFFFF) * (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2);
    score += ((unsigned long long)POPCOUNT(live_jump3 >> 32) * (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2)) << 32;

    // 活四 (011110)
    unsigned long long live4 = m4 & mask_0xxxx0;
    score += (unsigned long long)POPCOUNT(live4 & 0xFFFFFFFF) * (SCORE_LIVE_4 - 2 * SCORE_RUSH_3);
    score += ((unsigned long long)POPCOUNT(live4 >> 32) * (SCORE_LIVE_4 - 2 * SCORE_RUSH_3)) << 32;

    // 冲四 (211110, 011112)
    unsigned long long rush4 = m4 & mask_axxxxb;
    score += (unsigned long long)POPCOUNT(rush4 & 0xFFFFFFFF) * (SCORE_RUSH_4 - SCORE_RUSH_3);
    score += ((unsigned long long)POPCOUNT(rush4 >> 32) * (SCORE_RUSH_4 - SCORE_RUSH_3)) << 32;

    // 跳四 (11101, 11011, 10111)
    unsigned long long jump4_1 = me & (valid >> 1) & (m3 >> 2); // 11101
    unsigned long long jump4_2 = m2 & (valid >> 2) & (m2 >> 3); // 11011
    unsigned long long jump4_3 = m3 & (valid >> 3) & (me >> 4); // 10111


    //消除假三
    unsigned long long live3 = live3_raw & ~(jump4_1 << 2) & ~jump4_3;
    unsigned long long rush3 = rush3_raw & ~(jump4_1 << 2) & ~jump4_3;

    score += (unsigned long long)POPCOUNT(live3 & 0xFFFFFFFF) * SCORE_LIVE_3;
    score += ((unsigned long long)POPCOUNT(live3 >> 32) * SCORE_LIVE_3) << 32;

    score += (unsigned long long)POPCOUNT(rush3 & 0xFFFFFFFF) * SCORE_RUSH_3;
    score += ((unsigned long long)POPCOUNT(rush3 >> 32) * SCORE_RUSH_3) << 32;

    // Score Jump 4 (Rush 4 equivalent)
    unsigned long long jump4 = jump4_1 | jump4_2 | jump4_3;
    score += (unsigned long long)POPCOUNT(jump4 & 0xFFFFFFFF) * SCORE_RUSH_4;
    score += ((unsigned long long)POPCOUNT(jump4 >> 32) * SCORE_RUSH_4) << 32;

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
