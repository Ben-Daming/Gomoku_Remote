#include "../include/ai.h"
#include "../include/bitboard.h"
#include "../include/evaluate.h"
#include <string.h>
#include <stdlib.h>

#define INF 100000000
#define WIN_THRESHOLD 90000
#define ABS(x) ((x) < 0 ? -(x) : (x))

#define DIR_COL 0
#define DIR_ROW 1
#define DIR_DIAG1 2
#define DIR_DIAG2 3

// Helper: Get length of a line
static inline int getLineLength(int dir, int idx) {
    if (dir == DIR_COL || dir == DIR_ROW) return BOARD_SIZE;
    return BOARD_SIZE - ABS(idx - (BOARD_SIZE - 1));
}

// Helper: Get line status from BitBoard
static inline Line getLineStatus(const BitBoardState* board, int dir, int idx, Player player) {
    const PlayerBitBoard* pBoard = (player == PLAYER_BLACK) ? &board->black : &board->white;
    switch(dir) {
        case DIR_COL: return pBoard->cols[idx];
        case DIR_ROW: return pBoard->rows[idx];
        case DIR_DIAG1: return pBoard->diag1[idx];
        case DIR_DIAG2: return pBoard->diag2[idx];
    }
    return 0;
}

// Initialize EvalState from scratch
static void initEvalState(const BitBoardState* board, EvalState* eval) {
    eval->total_score = 0;
    memset(eval->line_net_scores, 0, sizeof(eval->line_net_scores));

    // 1. Cols & Rows
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Cols
        Line b = board->black.cols[i];
        Line w = board->white.cols[i];
        unsigned long long scores = evaluateLines2(b, w, BOARD_SIZE, w, b, BOARD_SIZE);
        int score = (int)scores - (int)(scores >> 32);
        eval->line_net_scores[DIR_COL][i] = score;
        eval->total_score += score;

        // Rows
        b = board->black.rows[i];
        w = board->white.rows[i];
        scores = evaluateLines2(b, w, BOARD_SIZE, w, b, BOARD_SIZE);
        score = (int)scores - (int)(scores >> 32);
        eval->line_net_scores[DIR_ROW][i] = score;
        eval->total_score += score;
    }

    // 2. Diagonals
    for (int i = 0; i < (BOARD_SIZE * 2 - 1); i++) {
        int len = getLineLength(DIR_DIAG1, i);
        if (len < 5) continue;

        // Diag1
        Line b = board->black.diag1[i];
        Line w = board->white.diag1[i];
        unsigned long long scores = evaluateLines2(b, w, len, w, b, len);
        int score = (int)scores - (int)(scores >> 32);
        eval->line_net_scores[DIR_DIAG1][i] = score;
        eval->total_score += score;

        // Diag2
        b = board->black.diag2[i];
        w = board->white.diag2[i];
        scores = evaluateLines2(b, w, len, w, b, len);
        score = (int)scores - (int)(scores >> 32);
        eval->line_net_scores[DIR_DIAG2][i] = score;
        eval->total_score += score;
    }
}

static void aiMakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, UndoInfo* undo) {
    // Calculate indices
    int indices[4];
    indices[0] = col;
    indices[1] = row;
    indices[2] = row - col + (BOARD_SIZE - 1);
    indices[3] = row + col;

    // 1. Backup old scores & remove from total
    for (int i = 0; i < 4; i++) {
        int idx = indices[i];
        undo->old_line_net_scores[i] = eval->line_net_scores[i][idx];
        eval->total_score -= undo->old_line_net_scores[i];
    }

    // 2. Update BitBoard (Pass backup buffer for move_mask)
    updateBitBoard(board, row, col, player, undo->move_mask_backup);

    // 3. Calculate new scores
    for (int i = 0; i < 4; i++) {
        int idx = indices[i];
        int len = getLineLength(i, idx);
        
        if (len < 5) {
            eval->line_net_scores[i][idx] = 0;
            continue;
        }

        Line b = getLineStatus(board, i, idx, PLAYER_BLACK);
        Line w = getLineStatus(board, i, idx, PLAYER_WHITE);

        unsigned long long scores = evaluateLines2(b, w, len, w, b, len);
        int net_score = (int)scores - (int)(scores >> 32);
        
        eval->line_net_scores[i][idx] = net_score;
        eval->total_score += net_score;
    }
}

static void aiUnmakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, const UndoInfo* undo) {
    // 1. Restore BitBoard
    undoBitBoard(board, row, col, player, undo->move_mask_backup);

    // Calculate indices
    int indices[4];
    indices[0] = col;
    indices[1] = row;
    indices[2] = row - col + (BOARD_SIZE - 1);
    indices[3] = row + col;

    // 2. Restore scores
    for (int i = 0; i < 4; i++) {
        int idx = indices[i];
        eval->total_score -= eval->line_net_scores[i][idx]; // Remove current
        eval->line_net_scores[i][idx] = undo->old_line_net_scores[i]; // Restore old
        eval->total_score += undo->old_line_net_scores[i]; // Add old
    }
}

// 排序辅助结构体
typedef struct {
    Position move;
    int score;
} ScoredMove;

// Forward declarations
static void aiMakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, UndoInfo* undo);
static void aiUnmakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, const UndoInfo* undo);

// 降序比较函数
static int compareScoredMoves(const void* a, const void* b) {
    const ScoredMove* sa = (const ScoredMove*)a;
    const ScoredMove* sb = (const ScoredMove*)b;
    return sb->score - sa->score; // 降序
}

// Helper: Sort moves based on heuristics
// Order: Killer Moves > Static Eval
static void sortMoves(SearchContext* ctx, Position* moves, int count, int depth, Player player) {
    ScoredMove scored_moves[225];
    
    for (int i = 0; i < count; i++) {
        int score = 0;
        
        // 1. Killer Moves
        if ((moves[i].row == ctx->killer_moves[depth][0].row && moves[i].col == ctx->killer_moves[depth][0].col) ||
            (moves[i].row == ctx->killer_moves[depth][1].row && moves[i].col == ctx->killer_moves[depth][1].col)) {
            score = 90000000;
        }
        // 2. Static Evaluation
        else {
            UndoInfo undo;
            aiMakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);
            
            // Score from the perspective of the player making the move
            score = (player == PLAYER_BLACK) ? ctx->eval.total_score : -ctx->eval.total_score;
            
            aiUnmakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);
        }
        
        scored_moves[i].move = moves[i];
        scored_moves[i].score = score;
    }

    // Quick Sort
    qsort(scored_moves, count, sizeof(ScoredMove), compareScoredMoves);

    // Copy back
    for (int i = 0; i < count; i++) {
        moves[i] = scored_moves[i].move;
    }
}

// Alpha-Beta Search (Naive with Heuristics)
static int alphaBeta(SearchContext* ctx, int depth, int alpha, int beta, Player player) {
    // 1. Static Evaluation
    int current_score = (player == PLAYER_BLACK) ? ctx->eval.total_score : -ctx->eval.total_score;

    if (current_score > WIN_THRESHOLD) return current_score; // Win
    if (current_score < -WIN_THRESHOLD) return current_score; // Loss

    if (depth == 0) {
        return current_score;
    }

    // 2. Generate Moves
    Position moves[225];
    int count = generateMoves(&ctx->board, moves);
    if (count == 0) return 0; // Draw

    // 3. Sort Moves
    sortMoves(ctx, moves, count, depth, player);

    int best_score = -INF;
    
    // Beam Search: Limit the number of moves searched
    int limit = (count > BEAM_WIDTH) ? BEAM_WIDTH : count;

    for (int i = 0; i < limit; i++) {
        UndoInfo undo;
        aiMakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);
        ctx->nodes_searched++;

        int score = -alphaBeta(ctx, depth - 1, -beta, -alpha, (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);

        aiUnmakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);

        if (score > best_score) {
            best_score = score;
            
            if (score > alpha) {
                alpha = score;
            }
        }
        
        if (alpha >= beta) {
            // Update Killer Moves
            if (moves[i].row != ctx->killer_moves[depth][0].row || moves[i].col != ctx->killer_moves[depth][0].col) {
                ctx->killer_moves[depth][1] = ctx->killer_moves[depth][0];
                ctx->killer_moves[depth][0] = moves[i];
            }
            break;
        }
    }

    return best_score;
}

Position getAIMove(const GameState *game) {
    if(game->moveCount == 0) {
        // If first move, play in center
        return (Position){BOARD_SIZE / 2, BOARD_SIZE / 2};
    }

    // 1. Initialize Search Context
    SearchContext ctx;
    memset(&ctx, 0, sizeof(SearchContext));
    
    // Clone BitBoardState
    ctx.board = game->bitBoard;
    
    // Initialize EvalState
    initEvalState(&ctx.board, &ctx.eval);

    Player me = game->currentPlayer;
    
    // 3. Root Search (Fixed Depth)
    int depth = SEARCH_DEPTH;
    
    Position moves[225];
    int count = generateMoves(&ctx.board, moves);
    if (count == 0) return (Position){7, 7}; // Should not happen

    // Sort root moves too
    sortMoves(&ctx, moves, count, depth, me);

    int best_score = -INF;
    Position best_move = moves[0];
    int alpha = -INF;
    int beta = INF;
    
    // Beam Search at Root
    int limit = (count > BEAM_WIDTH) ? BEAM_WIDTH : count;

    for (int i = 0; i < limit; i++) {
        UndoInfo undo;
        aiMakeMove(&ctx.board, &ctx.eval, moves[i].row, moves[i].col, me, &undo);
        ctx.nodes_searched++;

        int score = -alphaBeta(&ctx, depth - 1, -beta, -alpha, (me == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);

        aiUnmakeMove(&ctx.board, &ctx.eval, moves[i].row, moves[i].col, me, &undo);

        if (score > best_score) {
            best_score = score;
            best_move = moves[i];
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }

    return best_move;
}
