#ifndef AI_H
#define AI_H

#include "types.h"
#include "bitboard.h"
#include <stdint.h>

#define MAX_LINES 30 // Max diagonals is 29
#define INVALID_POS ((Position){-1, -1})

typedef struct {
    // Cache net scores for 4 directions (Black Score - White Score)
    // [0]: Col, [1]: Row, [2]: Diag1, [3]: Diag2
    long long line_net_scores[4][MAX_LINES];

    // 缓存上一步可能触发禁手的总数目
    long long count_live3[4][MAX_LINES];
    long long count_4[4][MAX_LINES];

    long long total_score; // Global score = Sum(line_net_scores)
} EvalState;

typedef struct {
    Line move_mask_backup[15]; // Restore neighborhood mask
    long long old_line_net_scores[4]; // Restore old scores for the 4 affected lines
    long long old_total_score;
    long long old_count_live3[4]; // Restore old live3 counts for the 4 affected lines
    long long old_count_4[4]; // Restore old 4 counts for the 4 affected lines
} UndoInfo;

// --- Search Parameters ---
#define SEARCH_DEPTH 12
#define BEAM_WIDTH 10
#define MAX_DEPTH (SEARCH_DEPTH + 1)

typedef struct {
    Position killer_moves[MAX_DEPTH][2];
    unsigned long long nodes_searched;
    
    // Thread-local board & eval state
    BitBoardState board;
    EvalState eval;
} SearchContext;

Position getAIMove(const GameState *game);

#endif
