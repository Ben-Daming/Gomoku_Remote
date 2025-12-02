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
        Line b_col = board->black.cols[i];
        Line w_col = board->white.cols[i];
        int b_score = evaluateLine(b_col, w_col, BOARD_SIZE);
        int w_score = evaluateLine(w_col, b_col, BOARD_SIZE);
        eval->line_net_scores[DIR_COL][i] = b_score - w_score;
        eval->total_score += eval->line_net_scores[DIR_COL][i];
        
        // Rows
        Line b_row = board->black.rows[i];
        Line w_row = board->white.rows[i];
        b_score = evaluateLine(b_row, w_row, BOARD_SIZE);
        w_score = evaluateLine(w_row, b_row, BOARD_SIZE);
        eval->line_net_scores[DIR_ROW][i] = b_score - w_score;
        eval->total_score += eval->line_net_scores[DIR_ROW][i];
    }

    // 2. Diagonals
    for (int i = 0; i < (BOARD_SIZE * 2 - 1); i++) {
        int len = getLineLength(DIR_DIAG1, i);
        if (len < 5) continue;

        // Diag1
        Line b_d1 = board->black.diag1[i];
        Line w_d1 = board->white.diag1[i];
        int b_score = evaluateLine(b_d1, w_d1, len);
        int w_score = evaluateLine(w_d1, b_d1, len);
        eval->line_net_scores[DIR_DIAG1][i] = b_score - w_score;
        eval->total_score += eval->line_net_scores[DIR_DIAG1][i];

        // Diag2
        Line b_d2 = board->black.diag2[i];
        Line w_d2 = board->white.diag2[i];
        b_score = evaluateLine(b_d2, w_d2, len);
        w_score = evaluateLine(w_d2, b_d2, len);
        eval->line_net_scores[DIR_DIAG2][i] = b_score - w_score;
        eval->total_score += eval->line_net_scores[DIR_DIAG2][i];
    }
}

static void aiMakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, UndoInfo* undo) {
    // Calculate indices
    int indices[4];
    indices[0] = col;
    indices[1] = row;
    indices[2] = row - col + (BOARD_SIZE - 1);
    indices[3] = row + col;

    // 1. Backup old state
    undo->old_total_score = eval->total_score;
    for(int i=0; i<4; i++) {
        undo->old_line_net_scores[i] = eval->line_net_scores[i][indices[i]];
        // Subtract old scores from total
        eval->total_score -= undo->old_line_net_scores[i];
    }

    // 2. Update BitBoard
    updateBitBoard(board, row, col, player, undo->move_mask_backup);

    // 3. Calculate NEW scores using evaluateLines4
    Lines4 b_lines, w_lines, masks;
    int lens[4];
    for(int i=0; i<4; i++) lens[i] = getLineLength(i, indices[i]);

    // Pack Lines: [Diag2 | Diag1 | Row | Col]
    // Low: [Row (32-63) | Col (0-31)]
    // High: [Diag2 (32-63) | Diag1 (0-31)]
    
    // Col
    b_lines.low = (unsigned long long)board->black.cols[col];
    w_lines.low = (unsigned long long)board->white.cols[col];
    masks.low = (1ULL << lens[0]) - 1;

    // Row
    b_lines.low |= ((unsigned long long)board->black.rows[row] << 32);
    w_lines.low |= ((unsigned long long)board->white.rows[row] << 32);
    masks.low |= (((1ULL << lens[1]) - 1) << 32);

    // Diag1
    if (lens[2] >= 5) {
        b_lines.high = (unsigned long long)board->black.diag1[indices[2]];
        w_lines.high = (unsigned long long)board->white.diag1[indices[2]];
        masks.high = (1ULL << lens[2]) - 1;
    } else {
        b_lines.high = 0; w_lines.high = 0; masks.high = 0;
    }

    // Diag2
    if (lens[3] >= 5) {
        b_lines.high |= ((unsigned long long)board->black.diag2[indices[3]] << 32);
        w_lines.high |= ((unsigned long long)board->white.diag2[indices[3]] << 32);
        masks.high |= (((1ULL << lens[3]) - 1) << 32);
    }

    DualLines scores = evaluateLines4(b_lines, w_lines, masks);
    Lines4 b_scores = scores.me;
    Lines4 w_scores = scores.enemy;

    // Unpack and update
    // Col
    int b_score = (int)(b_scores.low & 0xFFFFFFFF);
    int w_score = (int)(w_scores.low & 0xFFFFFFFF);
    eval->line_net_scores[DIR_COL][col] = b_score - w_score;
    eval->total_score += eval->line_net_scores[DIR_COL][col];

    // Row
    b_score = (int)(b_scores.low >> 32);
    w_score = (int)(w_scores.low >> 32);
    eval->line_net_scores[DIR_ROW][row] = b_score - w_score;
    eval->total_score += eval->line_net_scores[DIR_ROW][row];

    // Diag1
    if (lens[2] >= 5) {
        b_score = (int)(b_scores.high & 0xFFFFFFFF);
        w_score = (int)(w_scores.high & 0xFFFFFFFF);
        eval->line_net_scores[DIR_DIAG1][indices[2]] = b_score - w_score;
        eval->total_score += eval->line_net_scores[DIR_DIAG1][indices[2]];
    }

    // Diag2
    if (lens[3] >= 5) {
        b_score = (int)(b_scores.high >> 32);
        w_score = (int)(w_scores.high >> 32);
        eval->line_net_scores[DIR_DIAG2][indices[3]] = b_score - w_score;
        eval->total_score += eval->line_net_scores[DIR_DIAG2][indices[3]];
    }
}

static void aiUnmakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, const UndoInfo* undo) {
    // 1. Restore BitBoard
    undoBitBoard(board, row, col, player, undo->move_mask_backup);

    // 2. Restore scores
    eval->total_score = undo->old_total_score;
    
    int indices[4];
    indices[0] = col;
    indices[1] = row;
    indices[2] = row - col + (BOARD_SIZE - 1);
    indices[3] = row + col;

    for(int i=0; i<4; i++) {
        eval->line_net_scores[i][indices[i]] = undo->old_line_net_scores[i];
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
