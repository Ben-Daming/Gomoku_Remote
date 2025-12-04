#include "../include/ai.h"
#include "../include/bitboard.h"
#include "../include/evaluate.h"
#include <string.h>
#include <stdlib.h>
#include<stdio.h>

#define INF 100000000
#define WIN_THRESHOLD 90000
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define RESOLVE_SCORE(score) ((score) >> _SHIFT)
#define RESOLVE_3(score) ((score) & ((BASE_4 - 1)))
#define RESOLVE_4(score) (((score) & ((1 << (_SHIFT)) - BASE_4)) >> (_SHIFT / 2))

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
    memset(eval->count_live3, 0, sizeof(eval->count_live3));
    memset(eval->count_4, 0, sizeof(eval->count_4));


    // 1. Cols & Rows
    for (int i = 0; i < BOARD_SIZE; i++) {
        // Cols
        Line b = board->black.cols[i];
        Line w = board->white.cols[i];
        unsigned long long scores = evaluateLines2(b, w, BOARD_SIZE, w, b, BOARD_SIZE);
        int b_score = RESOLVE_SCORE((int)scores);
        int w_score = RESOLVE_SCORE((int)(scores >> 32));
        int score = b_score - w_score;
        eval->line_net_scores[DIR_COL][i] = score;
        eval->count_live3[DIR_COL][i] = RESOLVE_3((int)scores);
        eval->count_4[DIR_COL][i] = RESOLVE_4((int)scores);
        eval->total_score += score;

        // Rows
        b = board->black.rows[i];
        w = board->white.rows[i];
        scores = evaluateLines2(b, w, BOARD_SIZE, w, b, BOARD_SIZE);
        b_score = RESOLVE_SCORE((int)scores);
        w_score = RESOLVE_SCORE((int)(scores >> 32));
        score = b_score - w_score;
        eval->line_net_scores[DIR_ROW][i] = score;
        eval->count_live3[DIR_ROW][i] = RESOLVE_3((int)scores);
        eval->count_4[DIR_ROW][i] = RESOLVE_4((int)scores);
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
        int b_score = RESOLVE_SCORE((int)scores);
        int w_score = RESOLVE_SCORE((int)(scores >> 32));
        int score = b_score - w_score;
        eval->line_net_scores[DIR_DIAG1][i] = score;
        eval->count_live3[DIR_DIAG1][i] = RESOLVE_3((int)scores);
        eval->count_4[DIR_DIAG1][i] = RESOLVE_4((int)scores);
        eval->total_score += score;

        // Diag2
        b = board->black.diag2[i];
        w = board->white.diag2[i];
        scores = evaluateLines2(b, w, len, w, b, len);
        b_score = RESOLVE_SCORE((int)scores);
        w_score = RESOLVE_SCORE((int)(scores >> 32));
        score = b_score - w_score;
        eval->line_net_scores[DIR_DIAG2][i] = score;
        eval->count_live3[DIR_DIAG2][i] = RESOLVE_3((int)scores);
        eval->count_4[DIR_DIAG2][i] = RESOLVE_4((int)scores);
        eval->total_score += score;
    }
    // // 1. Cols & Rows
    // for (int i = 0; i < BOARD_SIZE; i++) {
    //     // Cols
    //     Line b_col = board->black.cols[i];
    //     Line w_col = board->white.cols[i];
    //     int b_score = evaluateLine(b_col, w_col, BOARD_SIZE);
    //     int w_score = evaluateLine(w_col, b_col, BOARD_SIZE);
        
    //     eval->line_net_scores[DIR_COL][i] = RESOLVE_SCORE(b_score) - RESOLVE_SCORE(w_score);
    //     eval->count_live3[DIR_COL][i] = RESOLVE_3(b_score);
    //     eval->count_4[DIR_COL][i] = RESOLVE_4(b_score);
    //     eval->total_score += eval->line_net_scores[DIR_COL][i];
        
    //     // Rows
    //     Line b_row = board->black.rows[i];
    //     Line w_row = board->white.rows[i];
    //     b_score = evaluateLine(b_row, w_row, BOARD_SIZE);
    //     w_score = evaluateLine(w_row, b_row, BOARD_SIZE);
        
    //     eval->line_net_scores[DIR_ROW][i] = RESOLVE_SCORE(b_score) - RESOLVE_SCORE(w_score);
    //     eval->count_live3[DIR_ROW][i] = RESOLVE_3(b_score);
    //     eval->count_4[DIR_ROW][i] = RESOLVE_4(b_score);
    //     eval->total_score += eval->line_net_scores[DIR_ROW][i];
    // }

    // // 2. Diagonals
    // for (int i = 0; i < (BOARD_SIZE * 2 - 1); i++) {
    //     int len = getLineLength(DIR_DIAG1, i);
    //     if (len < 5) continue;

    //     // Diag1
    //     Line b_d1 = board->black.diag1[i];
    //     Line w_d1 = board->white.diag1[i];
    //     int b_score = evaluateLine(b_d1, w_d1, len);
    //     int w_score = evaluateLine(w_d1, b_d1, len);
        
    //     eval->line_net_scores[DIR_DIAG1][i] = RESOLVE_SCORE(b_score) - RESOLVE_SCORE(w_score);
    //     eval->count_live3[DIR_DIAG1][i] = RESOLVE_3(b_score);
    //     eval->count_4[DIR_DIAG1][i] = RESOLVE_4(b_score);
    //     eval->total_score += eval->line_net_scores[DIR_DIAG1][i];

    //     // Diag2
    //     Line b_d2 = board->black.diag2[i];
    //     Line w_d2 = board->white.diag2[i];
    //     b_score = evaluateLine(b_d2, w_d2, len);
    //     w_score = evaluateLine(w_d2, b_d2, len);
        
    //     eval->line_net_scores[DIR_DIAG2][i] = RESOLVE_SCORE(b_score) - RESOLVE_SCORE(w_score);
    //     eval->count_live3[DIR_DIAG2][i] = RESOLVE_3(b_score);
    //     eval->count_4[DIR_DIAG2][i] = RESOLVE_4(b_score);
    //     eval->total_score += eval->line_net_scores[DIR_DIAG2][i];
    // }
    //print all init data on (6, 10)
    // for(int i = 0; i < 4; i++){
    //     // Print all direction of eval
    //     char tmp[100];
    //     switch (i)
    //     {
    //     case 0:
    //         sprintf(tmp, "Column");
    //         break;
    //     case 1:
    //         sprintf(tmp, "Row");
    //         break;
    //     case 2:
    //         sprintf(tmp, "Diagonal \\");
    //         break;
    //     case 3:
    //         sprintf(tmp, "Diagonal /");
    //         break;
    //     default:
    //         break;
    //     }
    //     printf("Direction %s: net_score=%d, live3=%d, four=%d\n", tmp, 
    //         (int)eval->line_net_scores[i][(i==0)?10:((i==1)?6:((i==2)?(6-10+BOARD_SIZE-1):(6+10)))],
    //         (int)eval->count_live3[i][(i==0)?10:((i==1)?6:((i==2)?(6-10+BOARD_SIZE-1):(6+10)))],
    //         (int)eval->count_4[i][(i==0)?10:((i==1)?6:((i==2)?(6-10+BOARD_SIZE-1):(6+10)))]);
    // }
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
        undo->old_count_live3[i] = eval->count_live3[i][indices[i]];
        undo->old_count_4[i] = eval->count_4[i][indices[i]];
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
    int b_score = RESOLVE_SCORE(b_scores.low & 0xFFFFFFFF);
    int w_score = RESOLVE_SCORE(w_scores.low & 0xFFFFFFFF);
    int col_live3 = RESOLVE_3(b_scores.low & 0xFFFFFFFF);
    int col_4 = RESOLVE_4(b_scores.low & 0xFFFFFFFF);

    eval->line_net_scores[DIR_COL][col] = b_score - w_score;
    eval->count_live3[DIR_COL][col] = col_live3;
    eval->count_4[DIR_COL][col] = col_4;
    eval->total_score += eval->line_net_scores[DIR_COL][col];

    // Row
    b_score = RESOLVE_SCORE(b_scores.low >> 32);
    w_score = RESOLVE_SCORE(w_scores.low >> 32);
    int row_live3 = RESOLVE_3(b_scores.low >> 32);
    int row_4 = RESOLVE_4(b_scores.low >> 32);
    
    eval->line_net_scores[DIR_ROW][row] = b_score - w_score;
    eval->count_live3[DIR_ROW][row] = row_live3;
    eval->count_4[DIR_ROW][row] = row_4;
    eval->total_score += eval->line_net_scores[DIR_ROW][row];

    // Diag1
    if (lens[2] >= 5) {
        b_score = RESOLVE_SCORE(b_scores.high & 0xFFFFFFFF);
        w_score = RESOLVE_SCORE(w_scores.high & 0xFFFFFFFF);
        int diag1_live3 = RESOLVE_3(b_scores.high & 0xFFFFFFFF);
        int diag1_4 = RESOLVE_4(b_scores.high & 0xFFFFFFFF);
        
        eval->line_net_scores[DIR_DIAG1][indices[2]] = b_score - w_score;
        eval->count_live3[DIR_DIAG1][indices[2]] = diag1_live3;
        eval->count_4[DIR_DIAG1][indices[2]] = diag1_4;
        eval->total_score += eval->line_net_scores[DIR_DIAG1][indices[2]];
    }

    // Diag2
    if (lens[3] >= 5) {
        b_score = RESOLVE_SCORE(b_scores.high >> 32);
        w_score = RESOLVE_SCORE(w_scores.high >> 32);
        int diag2_live3 = RESOLVE_3(b_scores.high >> 32);
        int diag2_4 = RESOLVE_4(b_scores.high >> 32);
        
        eval->line_net_scores[DIR_DIAG2][indices[3]] = b_score - w_score;
        eval->count_live3[DIR_DIAG2][indices[3]] = diag2_live3;
        eval->count_4[DIR_DIAG2][indices[3]] = diag2_4;
        eval->total_score += eval->line_net_scores[DIR_DIAG2][indices[3]];
    }

    // 禁手判断 (仅对黑棋)
    if (player == PLAYER_BLACK) {
        int new_live3_count = 0;
        int new_4_count = 0;
        
        // 计算新产生的活三和四的总数
        for (int i = 0; i < 4; i++) {
            int idx = indices[i];
            // 跳过长度不足5的对角线
            if ((i == 2 || i == 3) && lens[i] < 5) continue;
            
            // 新产生的数量 = 当前数量 - 旧数量
            int diff_3 = eval->count_live3[i][idx] - undo->old_count_live3[i];
            int diff_4 = eval->count_4[i][idx] - undo->old_count_4[i];
            
            if (diff_3 > 0) new_live3_count += diff_3;
            if (diff_4 > 0) new_4_count += diff_4;
            
            // Debug info for potential forbidden moves
            // if (diff_3 > 0 || diff_4 > 0) {
            //    printf("Move (%d, %d) Dir %d: +Live3=%d, +Four=%d\n", row, col, i, diff_3, diff_4);
            // }
        }
        
        // 判断是否触发禁手：双活三(33)或双四(44)
        if (new_live3_count >= 2 || new_4_count >= 2) {
            // printf("Forbidden move detected at (%d, %d): Live3=%d, Four=%d\n", row, col, new_live3_count, new_4_count);
            eval->total_score = -INF;
        }
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
        eval->count_live3[i][indices[i]] = undo->old_count_live3[i];
        eval->count_4[i][indices[i]] = undo->old_count_4[i];
    }
}

// Forward declarations
static void aiMakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, UndoInfo* undo);
static void aiUnmakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, const UndoInfo* undo);

// Helper: Sort moves based on heuristics using insertion sort
// Order: Killer Moves > (MyScore + OpponentScore) (descending order)
static inline int sortMoves(SearchContext* ctx, Position* moves, Position* sorted_moves, Position best_move, int count, int depth, Player player) {
    int scores[BEAM_WIDTH + 1]; // Cache scores for sorted_moves to avoid re-evaluation
    int sorted_count = 0;
   
    for (int i = 0; i < count; i++) {
        // 1. Calculate Score for current move
        int score;
        if((best_move.row != INVALID_POS.row)  && (moves[i].row == best_move.row) && (moves[i].col == best_move.col)){
            score = INF + 1;
        }
        else if ((moves[i].row == ctx->killer_moves[depth][0].row && moves[i].col == ctx->killer_moves[depth][0].col) ||
            (moves[i].row == ctx->killer_moves[depth][1].row && moves[i].col == ctx->killer_moves[depth][1].col)) {
            score = INF;
        } else {
            UndoInfo undo;
            
            // A. Evaluate My Move
            aiMakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);
            score = (player == PLAYER_BLACK) ? ctx->eval.total_score : -ctx->eval.total_score;
            aiUnmakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);

        }

        // 2. Insert into the  list
        if (sorted_count < BEAM_WIDTH || score > scores[BEAM_WIDTH - 1]) {
            int j = sorted_count - 1;

            while (j >= 0 && scores[j] < score) {
                sorted_moves[j + 1] = sorted_moves[j];
                scores[j + 1] = scores[j];
                j--;
            }
            
            sorted_moves[j + 1] = moves[i];
            scores[j + 1] = score;

            if (sorted_count < BEAM_WIDTH) {
                sorted_count++;
            }
        }
    }
    return sorted_count;//返回min(BEAM_WIDTH,count)
}

// Alpha-Beta Search (Naive with Heuristics)
static int alphaBeta(SearchContext* ctx, int depth, int max_depth, int alpha, int beta, Player player) {
    // 1. Static Evaluation
    int current_score = (player == PLAYER_BLACK) ? ctx->eval.total_score : -ctx->eval.total_score;

    if (current_score > WIN_THRESHOLD) return current_score; // Win
    if (current_score < -WIN_THRESHOLD) return current_score; // Loss

    if (depth >= max_depth) {
        return current_score;
    }

    // 2. Generate Moves
    Position moves[225];
    int count = generateMoves(&ctx->board, moves);
    if (count == 0) return 0; // Draw

    // 3. Sort Moves
    Position sorted_moves[BEAM_WIDTH + 1];
    int limit = sortMoves(ctx, moves, sorted_moves, INVALID_POS, count, depth, player);
    

    int best_score = -INF;

    for (int i = 0; i < limit; i++) {
        UndoInfo undo;
        aiMakeMove(&ctx->board, &ctx->eval, sorted_moves[i].row, sorted_moves[i].col, player, &undo);
        ctx->nodes_searched++;

        int score = -alphaBeta(ctx, depth + 1, max_depth, -beta, -alpha, (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);

        aiUnmakeMove(&ctx->board, &ctx->eval, sorted_moves[i].row, sorted_moves[i].col, player, &undo);

        if (score > best_score) {
            best_score = score;
            
            if (score > alpha) {
                alpha = score;
            }
        }
        
        if (alpha >= beta) {
            // Update Killer Moves
            if (sorted_moves[i].row != ctx->killer_moves[depth][0].row || sorted_moves[i].col != ctx->killer_moves[depth][0].col) {
                ctx->killer_moves[depth][1] = ctx->killer_moves[depth][0];
                ctx->killer_moves[depth][0] = sorted_moves[i];
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

    // printf("AI thinking...\n");

    // 1. Initialize Search Context
    SearchContext ctx;
    memset(&ctx, 0, sizeof(SearchContext));
    
    // Clone BitBoardState
    ctx.board = game->bitBoard;
    
    // Initialize EvalState
    initEvalState(&ctx.board, &ctx.eval);

    Player me = game->currentPlayer;
    
    // 3. Iterative Deepening Search
    Position moves[225];
    int count = generateMoves(&ctx.board, moves);
    if (count == 0) return (Position){7, 7}; // Should not happen

    Position best_move = moves[0];
    int best_score = -INF;
    UndoInfo undo;
    
    for (int depth = 2; depth <= SEARCH_DEPTH; depth += 2) {
        // Sort root moves
        Position sorted_moves[BEAM_WIDTH + 1] = {0};
        int limit;

        // Move Ordering: Put best move from previous iteration first
        if (depth > 2) {
            limit = sortMoves(&ctx, moves, sorted_moves, best_move, count, 0, me);
            // int i;
            // for (i = 0; i < limit; i++) {
            //     if (sorted_moves[i].row == best_move.row && sorted_moves[i].col == best_move.col) {
            //         // Swap with first
            //         Position temp = sorted_moves[0];
            //         sorted_moves[0] = sorted_moves[i];
            //         sorted_moves[i] = temp;
            //         // printf("depth: %d, bestmove: (%d, %d), \
            //         //         replaced : (%d, %d)\n", depth,
            //         //         best_move.row, best_move.col,
            //         //         temp.row, temp.col);
            //         break;
            //     }
            // }

            /*上一层的最佳走法没排上
            那就顶掉最后一个元素，然后把上层的best_score放到第一位
            */
            // if(limit == i){
            //     for(int j = limit - 1; j > 0; j--){
            //         sorted_moves[j] = sorted_moves[j-1];
            //     }
            //     sorted_moves[0] = best_move; 
            // }
        }
        else {
            limit = sortMoves(&ctx, moves, sorted_moves, INVALID_POS, count, 0, me);
        }
        int current_best_score = -INF;
        Position current_best_move = sorted_moves[0];
        int alpha = -INF;
        int beta = INF;
        
        // Beam Search at Root
        for (int i = 0; i < limit; i++) {
            aiMakeMove(&ctx.board, &ctx.eval, sorted_moves[i].row, sorted_moves[i].col, me, &undo);
            
            // Check if root move is forbidden (for Black)
            if (me == PLAYER_BLACK && ctx.eval.total_score == -INF) {
                 // printf("Root move (%d, %d) is FORBIDDEN. Skipping.\n", sorted_moves[i].row, sorted_moves[i].col);
                 aiUnmakeMove(&ctx.board, &ctx.eval, sorted_moves[i].row, sorted_moves[i].col, me, &undo);
                 continue;
            }
            // Check for immediate win at root
            int current_val = (me == PLAYER_BLACK) ? ctx.eval.total_score : -ctx.eval.total_score;
            if(current_val >= WIN_THRESHOLD){
                aiUnmakeMove(&ctx.board, &ctx.eval, sorted_moves[i].row, sorted_moves[i].col, me, &undo);
                return sorted_moves[i];
            }

            ctx.nodes_searched++;

            int score = -alphaBeta(&ctx, 1, depth, -beta, -alpha, (me == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);

            aiUnmakeMove(&ctx.board, &ctx.eval, sorted_moves[i].row, sorted_moves[i].col, me, &undo);

            if (score > current_best_score) {
                current_best_score = score;
                current_best_move = sorted_moves[i];
            }
            
            if (score > alpha) {
                alpha = score;
            }
        }
        
        best_move = current_best_move;
        best_score = current_best_score;
        printf("Depth %d: Best Move (%d, %d), Score %d\nMove List:", depth, best_move.row, best_move.col, best_score);
        for(int i = 0; i < limit; i++){
            printf("(%d, %d) ", sorted_moves[i].row, sorted_moves[i].col);
        }
        printf("\n");
        // Early exit if winning move found
        if (best_score > WIN_THRESHOLD) break;
    }
    //trace best move
    // printf("currently the score is: %lld\n", ctx.eval.total_score);
    // for(int i = 0; i < 4; i++){
    //     // Print all direction of eval
    //     char tmp[100];
    //     switch (i)
    //     {
    //     case 0:
    //         sprintf(tmp, "Column");
    //         break;
    //     case 1:
    //         sprintf(tmp, "Row");
    //         break;
    //     case 2:
    //         sprintf(tmp, "Diagonal \\");
    //         break;
    //     case 3:
    //         sprintf(tmp, "Diagonal /");
    //         break;
    //     default:
    //         break;
    //     }
    //     printf("Direction %s: net_score=%d, live3=%d, four=%d\n", tmp, 
    //         (int)ctx.eval.line_net_scores[i][(i==0)?best_move.col:((i==1)?best_move.row:((i==2)?(best_move.row-best_move.col+BOARD_SIZE-1):(best_move.row+best_move.col)))],
    //         (int)ctx.eval.count_live3[i][(i==0)?best_move.col:((i==1)?best_move.row:((i==2)?(best_move.row-best_move.col+BOARD_SIZE-1):(best_move.row+best_move.col)))],
    //         (int)ctx.eval.count_4[i][(i==0)?best_move.col:((i==1)?best_move.row:((i==2)?(best_move.row-best_move.col+BOARD_SIZE-1):(best_move.row+best_move.col)))]);
    // }
    // aiMakeMove(&ctx.board, &ctx.eval, best_move.row, best_move.col, me, &undo);
    // printf("SCORE of AI moving (%d, %d): %lld\n", best_move.row, best_move.col, ctx.eval.total_score);
    // for(int i = 0; i < 4; i++){
    //     // Print all direction of eval
    //     char tmp[100];
    //     switch (i)
    //     {
    //     case 0:
    //         sprintf(tmp, "Column");
    //         break;
    //     case 1:
    //         sprintf(tmp, "Row");
    //         break;
    //     case 2:
    //         sprintf(tmp, "Diagonal \\");
    //         break;
    //     case 3:
    //         sprintf(tmp, "Diagonal /");
    //         break;
    //     default:
    //         break;
    //     }
    //     printf("Direction %s: net_score=%d, live3=%d, four=%d\n", tmp, 
    //         (int)ctx.eval.line_net_scores[i][(i==0)?best_move.col:((i==1)?best_move.row:((i==2)?(best_move.row-best_move.col+BOARD_SIZE-1):(best_move.row+best_move.col)))],
    //         (int)ctx.eval.count_live3[i][(i==0)?best_move.col:((i==1)?best_move.row:((i==2)?(best_move.row-best_move.col+BOARD_SIZE-1):(best_move.row+best_move.col)))],
    //         (int)ctx.eval.count_4[i][(i==0)?best_move.col:((i==1)?best_move.row:((i==2)?(best_move.row-best_move.col+BOARD_SIZE-1):(best_move.row+best_move.col)))]);
    // }
    // aiUnmakeMove(&ctx.board, &ctx.eval, best_move.row, best_move.col, me, &undo);
    printf("AI selects move (%d, %d) with score %d after searching %lld nodes.\n", best_move.row, best_move.col, best_score, ctx.nodes_searched);
    return best_move;
}
