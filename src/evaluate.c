#include "../include/evaluate.h"
#include <stdlib.h>

// Helper macro 
#define POPCOUNT(x) __builtin_popcount(x)
#define POPCOUNT64(x) __builtin_popcountll(x)
#define ABS(x) ((x) < 0 ? -(x) : (x))

//弃用api
// // --- 评估函数 ---

// int evaluateLine(Line me, Line enemy, int length) {
//     int score = 0;
//     // Empty spots are those not occupied by me or enemy
//     // Mask with length to ensure we only care about the valid bits
//     Line mask = (1 << length) - 1;
//     Line valid = ~(me | enemy) & mask;

//     //连2，连3，连4，连5
//     Line m2 = me & (me >> 1); 
//     Line m3 = m2 & (m2 >> 1);
//     Line m4 = m3 & (m3 >> 1);
//     Line m5 = m4 & (m4 >> 1);
//     // m3 &= ~(m4 | (m4 << 1));//排除连四一部分
//     m2 &= ~(m3 | (m3 << 1));
//     if (m5) return SCORE_FIVE;
//     // 先判断连二
//     // 活二 (0110)
//     Line live2 = (valid << 1) & m2 & (valid >> 2);
//     // 冲二 (0112, 2110)
//     Line rush2 = m2 & ((valid << 1) ^ (valid >> 2));
//     // 强活二 (001100) 
//     Line strong_live2 = (valid << 2) & live2 & (valid >> 3);

//     score += POPCOUNT(strong_live2) * (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2);
//     score += POPCOUNT(live2) * SCORE_LIVE_2;
//     score += POPCOUNT(rush2) * SCORE_RUSH_2;

//     // 活三 (01110)
//     Line live3_raw = (valid << 1) & m3 & (valid >> 3);
//     // 冲连三 (21110, 01112)
//     Line rush3_raw = ((valid << 1) ^ (valid >> 3)) & m3;
    
//     // 跳三(1101, 1011)
//     Line jump3_a = me & (m2 >> 2) & (valid >> 1);//1101
//     Line jump3_b = (me >> 3) & m2 & (valid >> 2);//1011  
    
//     Line mask_0xxxx0 = (valid >> 4) & (valid << 1);
//     Line mask_axxxxb = (valid >> 4) ^ (valid << 1); //两边有一个0

//     // 活跳三(011010, 010110)
//     Line live_jump3 = (jump3_a | jump3_b) & mask_0xxxx0;
//     score += POPCOUNT(live_jump3) * (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2);

//     // 活四 (011110)
//     Line live4 = m4 & mask_0xxxx0;
//     score += POPCOUNT(live4) * (SCORE_LIVE_4 - 2 * SCORE_RUSH_3);

//     // 冲四 (211110, 011112)
//     Line rush4 = m4 & mask_axxxxb;
//     score += POPCOUNT(rush4) * (SCORE_RUSH_4 - SCORE_RUSH_3);

//     // 跳四 (11101, 11011, 10111)
//     Line jump4_1 = me & (valid >> 1) & (m3 >> 2); // 11101
//     Line jump4_2 = m2 & (valid >> 2) & (m2 >> 3); // 11011
//     Line jump4_3 = m3 & (valid >> 3) & (me >> 4); // 10111


//     //消除假三
//     Line live3 = live3_raw & ~(jump4_1 << 2) & ~jump4_3;
//     Line rush3 = rush3_raw & ~(jump4_1 << 2) & ~jump4_3;

//     score += POPCOUNT(live3) * SCORE_LIVE_3;
//     score += POPCOUNT(rush3) * SCORE_RUSH_3;

//     // Score Jump 4 (Rush 4 equivalent)
//     score += POPCOUNT(jump4_1 | jump4_2 | jump4_3) * SCORE_RUSH_4;

//     return score;
// }

// 并行评估4条线（通过数组打包进行向量化）
// 这种方法将4条线（我方低/高，敌方低/高）打包到数组中
// 允许编译器使用AVX2/SIMD进行位运算向量化。
// POPCOUNT随后作为标量指令顺序执行。
DualLines evaluateLines4(Lines4 me, Lines4 enemy, Lines4 mask) {
    DualLines scores = {{0, 0}, {0, 0}};

    // 1. 预计算有效位（我方与敌方的交互，不适合统一SIMD循环）
    unsigned long long valid_low = ~(me.low | enemy.low) & mask.low;
    unsigned long long valid_high = ~(me.high | enemy.high) & mask.high;

    // 2. 打包数据
    // 布局: [Me_Low, Me_High, Enemy_Low, Enemy_High]
    unsigned long long inputs[4] = {me.low, me.high, enemy.low, enemy.high};
    
    // 准备有效掩码
    // 布局: [valid_low, valid_high, valid_low, valid_high]
    unsigned long long valids[4] = {valid_low, valid_high, valid_low, valid_high};

    //要用的暂存数组
    unsigned long long res_m5[4];
    unsigned long long res_live2[4], res_rush2[4], res_strong_live2[4];
    unsigned long long res_live_jump3[4];
    unsigned long long res_live3[4], res_rush3[4];
    unsigned long long res_live4[4], res_rush4[4];
    unsigned long long res_jump4[4];

    // ==========================================================
    // 3. SIMD循环
    // 编译器应展开此循环并使用AVX2（4x64位），否则效率会低下！
    // ==========================================================
    #pragma omp simd
    for (int i = 0; i < 4; i++) {
        unsigned long long my_line = inputs[i];
        unsigned long long valid = valids[i];

        // 共享掩码（本地计算以便向量化）
        unsigned long long mask_0xxxx0 = (valid >> 4) & (valid << 1);
        unsigned long long mask_axxxxb = (valid >> 4) ^ (valid << 1);

        // ---基础连子 ---
        unsigned long long m2 = my_line & (my_line >> 1);
        unsigned long long m3 = m2 & (m2 >> 1);
        unsigned long long m4 = m3 & (m3 >> 1);
        unsigned long long m5 = m4 & (m4 >> 1);
        
        res_m5[i] = m5;

        // --- 精炼连二 ---
        m2 &= ~(m3 | (m3 << 1));

        // --- 活二与冲二 ---
        unsigned long long live2 = (valid << 1) & m2 & (valid >> 2);
        unsigned long long rush2 = m2 & ((valid << 1) ^ (valid >> 2));
        
        res_live2[i] = live2;
        res_rush2[i] = rush2;

        // --- 强活二 ---
        unsigned long long strong_live2 = (valid << 2) & live2 & (valid >> 3);
        res_strong_live2[i] = strong_live2;

        // --- 跳三 ---
        unsigned long long jump3_a = my_line & (m2 >> 2) & (valid >> 1);
        unsigned long long jump3_b = (my_line >> 3) & m2 & (valid >> 2);
        unsigned long long live_jump3 = (jump3_a | jump3_b) & mask_0xxxx0;
        
        res_live_jump3[i] = live_jump3;

        // --- 活四与冲四 ---
        unsigned long long live4 = m4 & mask_0xxxx0;
        unsigned long long rush4 = m4 & mask_axxxxb;
        
        res_live4[i] = live4;
        res_rush4[i] = rush4;

        // --- 跳四 ---
        unsigned long long jump4_1 = my_line & (valid >> 1) & (m3 >> 2);
        unsigned long long jump4_2 = m2 & (valid >> 2) & (m2 >> 3);
        unsigned long long jump4_3 = m3 & (valid >> 3) & (my_line >> 4);
        unsigned long long jump4 = jump4_1 | jump4_2 | jump4_3;
        
        res_jump4[i] = jump4;

        // --- 最终活三与冲三 ---
        unsigned long long live3_raw = (valid << 1) & m3 & (valid >> 3);
        unsigned long long rush3_raw = ((valid << 1) ^ (valid >> 3)) & m3;
        
        unsigned long long filter = ~(jump4_1 << 2) & ~jump4_3;
        
        unsigned long long live3 = live3_raw & filter;
        unsigned long long rush3 = rush3_raw & filter;
        
        res_live3[i] = live3;
        res_rush3[i] = rush3;
    }

    // ==========================================================
    // 标量计分，没有256位的POPCOUNT指令
    // ==========================================================

    // 检查五连
    // 索引: 0=我方低, 1=我方高, 2=敌方低, 3=敌方高
    if (res_m5[0] | res_m5[1]) {
        if (res_m5[0] & 0xFFFFFFFFULL) scores.me.low |= (unsigned long long)SCORE_FIVE;
        if (res_m5[0] & 0xFFFFFFFF00000000ULL) scores.me.low |= ((unsigned long long)SCORE_FIVE << 32);
        if (res_m5[1] & 0xFFFFFFFFULL) scores.me.high |= (unsigned long long)SCORE_FIVE;
        if (res_m5[1] & 0xFFFFFFFF00000000ULL) scores.me.high |= ((unsigned long long)SCORE_FIVE << 32);
        return scores;
    }
    if (res_m5[2] | res_m5[3]) {
        if (res_m5[2] & 0xFFFFFFFFULL) scores.enemy.low |= (unsigned long long)SCORE_FIVE;
        if (res_m5[2] & 0xFFFFFFFF00000000ULL) scores.enemy.low |= ((unsigned long long)SCORE_FIVE << 32);
        if (res_m5[3] & 0xFFFFFFFFULL) scores.enemy.high |= (unsigned long long)SCORE_FIVE;
        if (res_m5[3] & 0xFFFFFFFF00000000ULL) scores.enemy.high |= ((unsigned long long)SCORE_FIVE << 32);
        return scores;
    }

    // 累加分数
    unsigned long long* targets[4] = {&scores.me.low, &scores.me.high, &scores.enemy.low, &scores.enemy.high};
    
    //8*9 = 72次POPCOUNT64
    for (int i = 0; i < 4; i++) {
        unsigned long long score_acc = 0;
        unsigned long long high_acc = 0;

        #define ACC(feats, score_val) \
            score_acc += (unsigned long long)POPCOUNT64(feats[i] & 0xFFFFFFFF) * score_val; \
            high_acc  += (unsigned long long)POPCOUNT64(feats[i] >> 32) * score_val;

        ACC(res_live2, SCORE_LIVE_2);
        ACC(res_rush2, SCORE_RUSH_2);
        ACC(res_strong_live2, (SCORE_STRONG_LIVE_2 - SCORE_LIVE_2));
        ACC(res_live_jump3, (SCORE_JUMP_LIVE_3 - SCORE_LIVE_2));
        ACC(res_live3, SCORE_LIVE_3);
        ACC(res_rush3, SCORE_RUSH_3);
        ACC(res_live4, (SCORE_LIVE_4 - 2 * SCORE_RUSH_3));
        ACC(res_rush4, (SCORE_RUSH_4 - SCORE_RUSH_3));
        ACC(res_jump4, SCORE_RUSH_4);
        
        #undef ACC

        *targets[i] = score_acc | (high_acc << 32);
    }

    return scores;
}

// --- ai 初始化 Helpers ---

// 一次并行算两lines
// 第1条线位于位0，第2条线位于位32
unsigned long long evaluateLines2(Line me1, Line enemy1, int len1, Line me2, Line enemy2, int len2) {
    unsigned long long score = 0;
    
    // 打包输入到64位整数
    // 第1条线: 位0-14
    // 第2条线: 位32-46
    unsigned long long me = (unsigned long long)me1 | ((unsigned long long)me2 << 32);
    unsigned long long enemy = (unsigned long long)enemy1 | ((unsigned long long)enemy2 << 32);
    
    // 掩码
    unsigned long long mask1 = (1ULL << len1) - 1;
    unsigned long long mask2 = (1ULL << len2) - 1;
    unsigned long long mask = mask1 | (mask2 << 32);
    
    unsigned long long valid = ~(me | enemy) & mask;

    
    //连2，连3，连4，连5
    unsigned long long m2 = me & (me >> 1); 
    unsigned long long m3 = m2 & (m2 >> 1);
    unsigned long long m4 = m3 & (m3 >> 1);
    unsigned long long m5 = m4 & (m4 >> 1);

    if (m5) {
        // 如果m5在低32位有位，表示第1条线获胜；如果在高32位，表示第2条线获胜。
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

    // 计分跳四
    unsigned long long jump4 = jump4_1 | jump4_2 | jump4_3;
    score += (unsigned long long)POPCOUNT(jump4 & 0xFFFFFFFF) * SCORE_RUSH_4;
    score += ((unsigned long long)POPCOUNT(jump4 >> 32) * SCORE_RUSH_4) << 32;

    return score;
}

// 弃用api
// // 总分初始化api
// int evaluateBoard(const BitBoardState *bitBoard, Player player) {
//     int total_score = 0;
//     const PlayerBitBoard *my_board = (player == PLAYER_BLACK) ? &bitBoard->black : &bitBoard->white;
//     const PlayerBitBoard *opp_board = (player == PLAYER_BLACK) ? &bitBoard->white : &bitBoard->black;
//     Line me, enemy;

//     // 1. Vertical (Columns)
//     for (int col = 0; col < BOARD_SIZE; col++) {
//         me = my_board->cols[col];
//         enemy = opp_board->cols[col];
//         total_score += evaluateLine(me, enemy, BOARD_SIZE);
//     }

//     // 2. Horizontal (Rows)
//     for (int row = 0; row < BOARD_SIZE; row++) {
//         me = my_board->rows[row];
//         enemy = opp_board->rows[row];
//         total_score += evaluateLine(me, enemy, BOARD_SIZE);
//     }

//     // 3. Main Diagonals (\)
//     // Index: row - col + 14. Range: 0 to 28.
//     // Length: 15 - |14 - index|
//     // Only scan diagonals with length >= 5
//     for (int i = 0; i < BOARD_SIZE * 2 - 1; i++) {
//         int len = BOARD_SIZE - ABS(BOARD_SIZE - 1 - i);
//         if (len < 5) continue;
        
//         me = my_board->diag1[i];
//         enemy = opp_board->diag1[i];
//         total_score += evaluateLine(me, enemy, len);
//     }

//     // 4. Anti Diagonals (/)
//     // Index: row + col. Range: 0 to 28.
//     // Length: 15 - |14 - index|
//     for (int i = 0; i < BOARD_SIZE * 2 - 1; i++) {
//         int len = BOARD_SIZE - ABS(BOARD_SIZE - 1 - i);
//         if (len < 5) continue;

//         me = my_board->diag2[i];
//         enemy = opp_board->diag2[i];
//         total_score += evaluateLine(me, enemy, len);
//     }

//     return total_score;
// }

// int evaluate(const BitBoardState *bitBoard) {
//     int blackScore = evaluateBoard(bitBoard, PLAYER_BLACK);
//     int whiteScore = evaluateBoard(bitBoard, PLAYER_WHITE);
//     return blackScore - whiteScore;
// }
