#ifndef EVALUATE_H
#define EVALUATE_H

#include "types.h"
#include "bitboard.h"

// Score Constants (Based on AI Migration Plan Step 2)
#define SCORE_FIVE           100000
#define SCORE_LIVE_4         50000
#define SCORE_RUSH_4         8000
#define SCORE_LIVE_3         6000
#define SCORE_JUMP_LIVE_3    4000
#define SCORE_RUSH_3         700
#define SCORE_STRONG_LIVE_2_ADDITION  100
#define SCORE_LIVE_2         100
#define SCORE_RUSH_2         20

// Evaluate a single line (15 bits)
// me: Bitmask of current player's stones
// enemy: Bitmask of opponent's stones
// length: Effective length of the line (for diagonals)
int evaluateLine(Line me, Line enemy, int length);

// Evaluate the entire board for a specific player
// Returns the sum of scores for all lines (Vertical, Horizontal, Diagonals)
int evaluateBoard(const BitBoardState *bitBoard, Player player);

// Calculate the total score (Black Score - White Score)
// Positive means Black advantage, Negative means White advantage
int evaluate(const BitBoardState *bitBoard);

#endif
