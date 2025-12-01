#ifndef AI_H
#define AI_H

#include "types.h"
#include "bitboard.h"
#include <stdint.h>

#define MAX_LINES 30 // Max diagonals is 29

// --- Transposition Table Definitions ---

#define TT_FLAG_EXACT 0
#define TT_FLAG_LOWERBOUND 1 // Alpha (Fail Low)
#define TT_FLAG_UPPERBOUND 2 // Beta (Fail High)

typedef struct {
    uint64_t key;       // Zobrist Hash
    int depth;          // Search depth
    int value;          // Score
    int flag;           // TT_FLAG_*
    Position bestMove;  // Best move for this position
} TTEntry;

// TT API
void initTT(int size_mb);
void freeTT();
void clearTT();
// Returns 1 if found and usable (score updated), 0 otherwise. 
// Always updates bestMove if found (even if depth is insufficient).
int probeTT(uint64_t key, int depth, int alpha, int beta, int* score, Position* bestMove);
void saveTT(uint64_t key, int depth, int value, int flag, Position bestMove);

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

// --- Search Context ---
#define MAX_DEPTH 20
#define SEARCH_DEPTH 10
#define BEAM_WIDTH 11

typedef struct {
    int thread_id;
    
    // Thread-local heuristics
    Position killer_moves[MAX_DEPTH][2];
    int history_table[BOARD_SIZE][BOARD_SIZE];
    
    // Thread-local board & eval state
    BitBoardState board;
    EvalState eval;
    
    // Statistics
    long nodes_searched;
} SearchContext;

Position getAIMove(const GameState *game);

#endif
