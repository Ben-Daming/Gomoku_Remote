#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "bitboard.h"

// 禁手位移量
#define _SHIFT 6

// 活三,四掩码
#define _MASK_3 0b111
#define _MASK_4 0b111000

// 四的基数
#define BASE_4 (1 << (_SHIFT / 2))

// 评分宏
// 评分宏
#define SCORE_FIVE           ((10000000) <<( _SHIFT))
#define SCORE_LIVE_4         (((50000) << (_SHIFT)) + BASE_4)
#define SCORE_RUSH_4         (((8000) << (_SHIFT)) + BASE_4)
#define SCORE_LIVE_3         (((7000) << (_SHIFT)) + 1)
#define SCORE_JUMP_LIVE_3    (((6000) << (_SHIFT)) + 1)
#define SCORE_RUSH_3         (((1500) << (_SHIFT)))
#define SCORE_STRONG_LIVE_2  ((800) << (_SHIFT))
#define SCORE_LIVE_2         ((400) << (_SHIFT))
#define SCORE_RUSH_2         ((50) << (_SHIFT))

//弃用api
// 评估一条线（15位）
// me: 当前玩家棋子的位掩码
// enemy: 对手棋子的位掩码
// length: 该线的有效长度（用于对角线）
//int evaluateLine(Line me, Line enemy, int length);

// 并行评估两条线
// 将两条线打包到一个64位整数中以同时计算分数
// 返回一个打包的64位整数: [Score2（32位）| Score1（32位）]
unsigned long long evaluateLines2(Line me1, Line enemy1, int len1, Line me2, Line enemy2, int len2);

// 存储4条打包线的结构体（共128位）
// low:  [第2条线（32位）| 第1条线（32位）]
// high: [第4条线（32位）| 第3条线（32位）]
typedef struct {
    unsigned long long low;
    unsigned long long high;
} Lines4;

// 256位结构体
typedef struct {
    Lines4 me;
    Lines4 enemy;
} DualLines;

// 并行评估双方的四条线
// 返回双方所有4条线的打包分数
DualLines evaluateLines4(Lines4 me, Lines4 enemy, Lines4 mask);

// 评估整个棋盘上某一玩家的分数
// 返回所有线（纵向、横向、对角线）的分数总和
int evaluateBoard(const BitBoardState *bitBoard, Player player);

// 计算总分（黑方分数 - 白方分数）
// 正数表示黑方优势，负数表示白方优势
int evaluate(const BitBoardState *bitBoard);

#endif
