#ifndef AI_H
#define AI_H

#include "types.h"
#include "bitboard.h"
#include <stdint.h>

#define MAX_LINES 30 // Max diagonals is 29

typedef struct {
    // Cache net scores for 4 directions (Black Score - White Score)
    // [0]: Col, [1]: Row, [2]: Diag1, [3]: Diag2
    int line_net_scores[4][MAX_LINES]; 
    
    int total_score; // Global score = Sum(line_net_scores)
} EvalState;

typedef struct {
    Line move_mask_backup[15]; // Restore neighborhood mask
    int old_line_net_scores[4]; // Restore old scores for the 4 affected lines
} UndoInfo;

// --- Search Parameters ---
#define SEARCH_DEPTH 8
#define BEAM_WIDTH 15
#define MAX_DEPTH 20

typedef struct {
    Position killer_moves[MAX_DEPTH][2];
    long nodes_searched;
    
    // Thread-local board & eval state
    BitBoardState board;
    EvalState eval;
} SearchContext;

Position getAIMove(const GameState *game);

#endif
