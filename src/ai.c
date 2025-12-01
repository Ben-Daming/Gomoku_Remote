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
static int getLineLength(int dir, int idx) {
    if (dir == DIR_COL || dir == DIR_ROW) return BOARD_SIZE;
    return BOARD_SIZE - ABS(idx - (BOARD_SIZE - 1));
}

// Helper: Get line status from BitBoard
static Line getLineStatus(const BitBoardState* board, int dir, int idx, Player player) {
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
        int score = evaluateLine(b, w, BOARD_SIZE) - evaluateLine(w, b, BOARD_SIZE);
        eval->line_net_scores[DIR_COL][i] = score;
        eval->total_score += score;

        // Rows
        b = board->black.rows[i];
        w = board->white.rows[i];
        score = evaluateLine(b, w, BOARD_SIZE) - evaluateLine(w, b, BOARD_SIZE);
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
        int score = evaluateLine(b, w, len) - evaluateLine(w, b, len);
        eval->line_net_scores[DIR_DIAG1][i] = score;
        eval->total_score += score;

        // Diag2
        b = board->black.diag2[i];
        w = board->white.diag2[i];
        score = evaluateLine(b, w, len) - evaluateLine(w, b, len);
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

        int net_score = evaluateLine(b, w, len) - evaluateLine(w, b, len);
        
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

// Alpha-Beta Search (Naive)
static int alphaBeta(BitBoardState* board, EvalState* eval, int depth, int alpha, int beta, Player player) {
    // 1. Static Evaluation
    int current_score = (player == PLAYER_BLACK) ? eval->total_score : -eval->total_score;

    if (current_score > WIN_THRESHOLD) return current_score; // Win
    if (current_score < -WIN_THRESHOLD) return current_score; // Loss

    if (depth == 0) {
        return current_score;
    }

    // 2. Generate Moves
    Position moves[225];
    int count = generateMoves(board, moves);
    if (count == 0) return 0; // Draw

    int best_score = -INF;

    for (int i = 0; i < count; i++) {
        UndoInfo undo;
        aiMakeMove(board, eval, moves[i].row, moves[i].col, player, &undo);

        int score = -alphaBeta(board, eval, depth - 1, -beta, -alpha, (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);

        aiUnmakeMove(board, eval, moves[i].row, moves[i].col, player, &undo);

        if (score > best_score) {
            best_score = score;
            
            if (score > alpha) {
                alpha = score;
            }
        }
        
        if (alpha >= beta) {
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

    // 1. Clone BitBoardState to avoid modifying the actual game
    BitBoardState boardCopy = game->bitBoard;
    
    // 2. Initialize EvalState
    EvalState eval;
    initEvalState(&boardCopy, &eval);

    Player me = game->currentPlayer;
    
    // 3. Root Search (Fixed Depth)
    // We need to replicate the loop here to find the best move, 
    // since alphaBeta only returns score.
    int depth = 6; // Fixed depth for naive search
    
    Position moves[225];
    int count = generateMoves(&boardCopy, moves);
    if (count == 0) return (Position){7, 7}; // Should not happen

    int best_score = -INF;
    Position best_move = moves[0];
    int alpha = -INF;
    int beta = INF;

    for (int i = 0; i < count; i++) {
        UndoInfo undo;
        aiMakeMove(&boardCopy, &eval, moves[i].row, moves[i].col, me, &undo);

        int score = -alphaBeta(&boardCopy, &eval, depth - 1, -beta, -alpha, (me == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);

        aiUnmakeMove(&boardCopy, &eval, moves[i].row, moves[i].col, me, &undo);

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
