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

// --- Transposition Table Implementation ---

static TTEntry* tt_table = NULL;
static int tt_size = 0; // Number of entries

void initTT(int size_mb) {
    if (tt_table) free(tt_table);
    
    // Calculate number of entries
    // Ensure size is power of 2 for fast indexing (optional but good practice)
    // For now, just simple division
    tt_size = (size_mb * 1024 * 1024) / sizeof(TTEntry);
    tt_table = (TTEntry*)calloc(tt_size, sizeof(TTEntry));
}

void freeTT() {
    if (tt_table) {
        free(tt_table);
        tt_table = NULL;
    }
}

void clearTT() {
    if (tt_table) {
        memset(tt_table, 0, tt_size * sizeof(TTEntry));
    }
}

int probeTT(uint64_t key, int depth, int alpha, int beta, int* score, Position* bestMove) {
    if (!tt_table) return 0;
    int index = key % tt_size;
    TTEntry* entry = &tt_table[index];

    if (entry->key == key) {
        // Always retrieve the best move for ordering, regardless of depth
        if (entry->bestMove.row != -1) {
            *bestMove = entry->bestMove;
        }

        // Only return score if depth is sufficient
        if (entry->depth >= depth) {
            if (entry->flag == TT_FLAG_EXACT) {
                *score = entry->value;
                return 1;
            }
            if (entry->flag == TT_FLAG_LOWERBOUND && entry->value >= beta) {
                *score = entry->value;
                return 1;
            }
            if (entry->flag == TT_FLAG_UPPERBOUND && entry->value <= alpha) {
                *score = entry->value;
                return 1;
            }
        }
    }
    return 0;
}

void saveTT(uint64_t key, int depth, int value, int flag, Position bestMove) {
    if (!tt_table) return;

    int index = key % tt_size;
    TTEntry* entry = &tt_table[index];

    // Replacement Strategy:
    // 1. Always replace if empty (key == 0)
    // 2. Replace if new depth is greater or equal
    // 3. (Optional) Replace if same depth (to keep fresh)
    if (entry->key == 0 || depth >= entry->depth) {
        entry->key = key;
        entry->depth = depth;
        entry->value = value;
        entry->flag = flag;
        entry->bestMove = bestMove;
    }
}

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

// Alpha-Beta Search with Transposition Table
static int alphaBeta(BitBoardState* board, EvalState* eval, int depth, int alpha, int beta, Player player) {
    // 0. Transposition Table Lookup
    int tt_score;
    Position tt_move = {-1, -1};
    if (probeTT(board->hash, depth, alpha, beta, &tt_score, &tt_move)) {
        return tt_score; // Cutoff based on TT
    }

    // 1. Static Evaluation
    int current_score = (player == PLAYER_BLACK) ? eval->total_score : -eval->total_score;

    if (current_score > WIN_THRESHOLD) return current_score; // Win
    if (current_score < -WIN_THRESHOLD) return current_score; // Loss

    if (depth == 0) {
        // Save leaf node to TT (Exact value)
        saveTT(board->hash, 0, current_score, TT_FLAG_EXACT, (Position){-1, -1});
        return current_score;
    }

    // 2. Generate Moves
    Position moves[225];
    int count = generateMoves(board, moves);
    if (count == 0) return 0; // Draw

    // 3. Move Ordering (Hash Move First)
    if (tt_move.row != -1) {
        // Find the hash move in the list and swap it to the front
        for (int i = 0; i < count; i++) {
            if (moves[i].row == tt_move.row && moves[i].col == tt_move.col) {
                Position temp = moves[0];
                moves[0] = moves[i];
                moves[i] = temp;
                break;
            }
        }
    }

    int best_score = -INF;
    Position best_move = {-1, -1};
    int tt_flag = TT_FLAG_UPPERBOUND; // Default to Upper Bound (Fail Low)

    for (int i = 0; i < count; i++) {
        UndoInfo undo;
        aiMakeMove(board, eval, moves[i].row, moves[i].col, player, &undo);

        int score = -alphaBeta(board, eval, depth - 1, -beta, -alpha, (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);

        aiUnmakeMove(board, eval, moves[i].row, moves[i].col, player, &undo);

        if (score > best_score) {
            best_score = score;
            best_move = moves[i];
            
            if (score > alpha) {
                alpha = score;
                tt_flag = TT_FLAG_EXACT; // Found a better move, so it's at least Exact (PV-Node)
            }
        }
        
        if (alpha >= beta) {
            tt_flag = TT_FLAG_LOWERBOUND; // Fail High (Beta Cutoff)
            break;
        }
    }

    // 4. Save to Transposition Table
    saveTT(board->hash, depth, best_score, tt_flag, best_move);

    return best_score;
}

// MTD(f) Driver
static int mtdf(BitBoardState* board, EvalState* eval, int first_guess, int depth, Player player) {
    int g = first_guess;
    int upper = INF;
    int lower = -INF;

    while (lower < upper) {
        int beta = (g == lower) ? g + 1 : g;
        
        // Null window search: alpha = beta - 1
        int score = alphaBeta(board, eval, depth, beta - 1, beta, player);
        
        if (score < beta) {
            upper = score;
        } else {
            lower = score;
        }
        g = score;
    }
    return g;
}

Position getAIMove(const GameState *game) {
    // 0. Initialize TT if not already (e.g., 64MB)
    if (!tt_table) {
        initTT(64); 
    }

    // 1. Clone BitBoardState to avoid modifying the actual game
    BitBoardState boardCopy = game->bitBoard;
    
    // 2. Initialize EvalState
    EvalState eval;
    initEvalState(&boardCopy, &eval);

    Player me = game->currentPlayer;
    int guess = 0;
    
    // 3. Iterative Deepening with MTD(f)
    // Start from depth 1 up to target depth (e.g., 6 or 8)
    // This helps populate TT with good moves for deeper searches
    for (int depth = 1; depth <= 6; depth++) {
        guess = mtdf(&boardCopy, &eval, guess, depth, me);
        
        // If we found a winning line, we can stop early
        if (guess > WIN_THRESHOLD || guess < -WIN_THRESHOLD) break;
    }

    // 4. Retrieve Best Move from TT
    Position bestMove = {-1, -1};
    int score;
    // We probe with a dummy window just to get the move. 
    // The depth check in probeTT is for score validity, but bestMove is always returned if key matches.
    probeTT(boardCopy.hash, 0, -INF, INF, &score, &bestMove);
    
    // Fallback if TT failed (should not happen for root unless collision/overwrite)
    if (bestMove.row == -1) {
        Position moves[225];
        int count = generateMoves(&boardCopy, moves);
        if (count > 0) bestMove = moves[0];
        else { bestMove.row = 7; bestMove.col = 7; }
    }

    return bestMove;
}
