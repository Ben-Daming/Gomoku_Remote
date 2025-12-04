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


// Evaluate a single line (15 bits)
// me: Bitmask of current player's stones
// enemy: Bitmask of opponent's stones
// length: Effective length of the line (for diagonals)
//int evaluateLine(Line me, Line enemy, int length);

// Evaluate two lines in parallel (Batching)
// Packs two lines into a 64-bit integer to compute scores simultaneously
// Returns a packed 64-bit integer: [Score2 (32 bits) | Score1 (32 bits)]
unsigned long long evaluateLines2(Line me1, Line enemy1, int len1, Line me2, Line enemy2, int len2);

// Structure to hold 4 packed lines (128 bits total)
// low:  [Line 2 (32 bits) | Line 1 (32 bits)]
// high: [Line 4 (32 bits) | Line 3 (32 bits)]
typedef struct {
    unsigned long long low;
    unsigned long long high;
} Lines4;

// Structure to hold scores for both players
typedef struct {
    Lines4 me;
    Lines4 enemy;
} DualLines;

// Evaluate four lines in parallel for both players
// Returns the packed scores of all 4 lines for both players
DualLines evaluateLines4(Lines4 me, Lines4 enemy, Lines4 mask);

// Evaluate the entire board for a specific player
// Returns the sum of scores for all lines (Vertical, Horizontal, Diagonals)
int evaluateBoard(const BitBoardState *bitBoard, Player player);

// Calculate the total score (Black Score - White Score)
// Positive means Black advantage, Negative means White advantage
int evaluate(const BitBoardState *bitBoard);

#endif
