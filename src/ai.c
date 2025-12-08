#include "../include/ai.h"
#include "../include/bitboard.h"
#include "../include/evaluate.h"
#include "../include/tt.h"
#include "../include/zobrist.h"
#include "../include/ascii_art.h"
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

// Helper: 向TT中存分
static inline int scoreToTT(int score, int depth) {
    if (score > WIN_THRESHOLD) return score + depth;
    if (score < -WIN_THRESHOLD) return score - depth;
    return score;
}

// Helper: 从TT中解析真实分
static inline int scoreFromTT(int score, int depth) {
    if (score > WIN_THRESHOLD) return score - depth;
    if (score < -WIN_THRESHOLD) return score + depth;
    return score;
}

// Helper: 拿到line长度
static inline int getLineLength(int dir, int idx) {
    if (dir == DIR_COL || dir == DIR_ROW) return BOARD_SIZE;
    return BOARD_SIZE - ABS(idx - (BOARD_SIZE - 1));
}

//弃用
// // Helper: 获取BitBoard中的线状态
// static inline Line getLineStatus(const BitBoardState* board, int dir, int idx, Player player) {
//     const PlayerBitBoard* pBoard = (player == PLAYER_BLACK) ? &board->black : &board->white;
//     switch(dir) {
//         case DIR_COL: return pBoard->cols[idx];
//         case DIR_ROW: return pBoard->rows[idx];
//         case DIR_DIAG1: return pBoard->diag1[idx];
//         case DIR_DIAG2: return pBoard->diag2[idx];
//     }
//     return 0;
// }

// Helper: 初始化88条缓存和eval状态机
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
}

static void aiMakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, UndoInfo* undo) {
    // 计算各个方向的索引
    int indices[4];
    indices[0] = col;
    indices[1] = row;
    indices[2] = row - col + (BOARD_SIZE - 1);
    indices[3] = row + col;

    // 备份旧数据到 undo
    undo->old_total_score = eval->total_score;
    for(int i=0; i<4; i++) {
        undo->old_line_net_scores[i] = eval->line_net_scores[i][indices[i]];
        undo->old_count_live3[i] = eval->count_live3[i][indices[i]];
        undo->old_count_4[i] = eval->count_4[i][indices[i]];
        eval->total_score -= undo->old_line_net_scores[i];
    }

    // 更新棋盘
    updateBitBoard(board, row, col, player, undo->move_mask_backup);

    // 使用 evaluateLines4 计算新的分数
    Lines4 b_lines, w_lines, masks;
    int lens[4];
    for(int i=0; i<4; i++) lens[i] = getLineLength(i, indices[i]);

    // 打包各方向的棋型: [Diag2 | Diag1 | Row | Col]
    // 低位: [Row (32-63) | Col (0-31)]
    // 高位: [Diag2 (32-63) | Diag1 (0-31)]
    
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

    // 解包并更新
    // col
    int b_score = RESOLVE_SCORE(b_scores.low & 0xFFFFFFFF);
    int w_score = RESOLVE_SCORE(w_scores.low & 0xFFFFFFFF);
    int col_live3 = RESOLVE_3(b_scores.low & 0xFFFFFFFF);
    int col_4 = RESOLVE_4(b_scores.low & 0xFFFFFFFF);

    eval->line_net_scores[DIR_COL][col] = b_score - w_score;
    eval->count_live3[DIR_COL][col] = col_live3;
    eval->count_4[DIR_COL][col] = col_4;
    eval->total_score += eval->line_net_scores[DIR_COL][col];

    // row
    b_score = RESOLVE_SCORE(b_scores.low >> 32);
    w_score = RESOLVE_SCORE(w_scores.low >> 32);
    int row_live3 = RESOLVE_3(b_scores.low >> 32);
    int row_4 = RESOLVE_4(b_scores.low >> 32);
    
    eval->line_net_scores[DIR_ROW][row] = b_score - w_score;
    eval->count_live3[DIR_ROW][row] = row_live3;
    eval->count_4[DIR_ROW][row] = row_4;
    eval->total_score += eval->line_net_scores[DIR_ROW][row];

    // diag1
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

    // diag2
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

    // 禁手判断（仅对黑棋）
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
            
            // 调试可能的禁手
            // if (diff_3 > 0 || diff_4 > 0) {
            //    printf("Move (%d, %d) Dir %d: +Live3=%d, +Four=%d\n", row, col, i, diff_3, diff_4);
            // }
        }
        
        if (new_live3_count >= 2 || new_4_count >= 2) {
            // printf("检测到禁手 (%d, %d): Live3=%d, Four=%d\n", row, col, new_live3_count, new_4_count);
            eval->total_score = -INF;
        }
    }

}

static void aiUnmakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, const UndoInfo* undo) {
    undoBitBoard(board, row, col, player, undo->move_mask_backup);

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

    // 前置声明
static void aiMakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, UndoInfo* undo);
static void aiUnmakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, const UndoInfo* undo);

// Helper: 维护一个sort列表
// Order: Hash Move > Killer Moves > MyScore
static inline int sortMoves(SearchContext* ctx, Position* moves, Position* sorted_moves, Position tt_move, int count, int depth, Player player) {
    int scores[BEAM_WIDTH + 1]; // 缓存分数，避免重复计算
    int sorted_count = 0;
   
    for (int i = 0; i < count; i++) {
        // 1. 计算当前走法的分数
        int score;
        if((tt_move.row != INVALID_POS.row)  && (moves[i].row == tt_move.row) && (moves[i].col == tt_move.col)){
            score = INF + 1; // 哈希表走法优先级最高
        }
        else if ((moves[i].row == ctx->killer_moves[depth][0].row && moves[i].col == ctx->killer_moves[depth][0].col) ||
            (moves[i].row == ctx->killer_moves[depth][1].row && moves[i].col == ctx->killer_moves[depth][1].col)) {
            score = INF; // 杀手走法优先级次高
        } else {
            UndoInfo undo;
            
            // 评估我的走法
            aiMakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);
            score = (player == PLAYER_BLACK) ? ctx->eval.total_score : -ctx->eval.total_score;
            aiUnmakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);

        }

        // 2. 插入排序列表
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
    return sorted_count;// 返回 min(BEAM_WIDTH, count)
}

// 搜索函数，返回best_score（我）或者worst_score（对方）
static int alphaBeta(SearchContext* ctx, int depth, int max_depth, int alpha, int beta, Player player) {
    // 置换表查询
    int rem_depth = max_depth - depth;
    int tt_val;
    Position tt_move = INVALID_POS;
    
    int _loc_alpha = alpha;
    int _loc_beta = beta;
    if (tt_probe(ctx->board.hash, rem_depth, &_loc_alpha, &_loc_beta, &tt_val, &tt_move)) {
        return scoreFromTT(tt_val, depth);
    }
    alpha = _loc_alpha;
    beta = _loc_beta;

    // 静态评估
    int current_score = (player == PLAYER_BLACK) ? ctx->eval.total_score : -ctx->eval.total_score;

    if (current_score > WIN_THRESHOLD) return current_score - depth; // 胜利
    if (current_score < -WIN_THRESHOLD) return current_score + depth; // 失败

    if (depth >= max_depth) {
        return current_score;
    }

    // 生成走法
    Position moves[225];
    int count = generateMoves(&ctx->board, moves);
    if (count == 0) return 0; // 平局

    // 排序走法
    Position sorted_moves[BEAM_WIDTH + 1];
    int limit = sortMoves(ctx, moves, sorted_moves, tt_move, count, depth, player);
    

    int best_score = -INF;
    int original_alpha = alpha;
    Position best_move = INVALID_POS;

    for (int i = 0; i < limit; i++) {
        UndoInfo undo;
        aiMakeMove(&ctx->board, &ctx->eval, sorted_moves[i].row, sorted_moves[i].col, player, &undo);
        ctx->nodes_searched++;

        int score;

        if (i == 0) {
            // 第一个子节点（主变线）用全窗口搜索
            score = -alphaBeta(ctx, depth + 1, max_depth, -beta, -alpha, (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);
        } else {
            // 非主变线节点用零窗口搜索
            score = -alphaBeta(ctx, depth + 1, max_depth, -alpha - 1, -alpha, (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);
            // 如果比预期的高，就改成全窗口搜索
            if (score > alpha && score < beta) {
                score = -alphaBeta(ctx, depth + 1, max_depth, -beta, -alpha, (player == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK);
            }
        }

        aiUnmakeMove(&ctx->board, &ctx->eval, sorted_moves[i].row, sorted_moves[i].col, player, &undo);

        if (score > best_score) {
            best_score = score;
            best_move = sorted_moves[i];
            if (score > alpha) {
                alpha = score;
            }
        }

        if (alpha >= beta) {
            // 更新杀手走法
            if (sorted_moves[i].row != ctx->killer_moves[depth][0].row || sorted_moves[i].col != ctx->killer_moves[depth][0].col) {
                ctx->killer_moves[depth][1] = ctx->killer_moves[depth][0];
                ctx->killer_moves[depth][0] = sorted_moves[i];
            }
            break;
        }
    }

    // 保存到置换表
    int flag = TT_FLAG_EXACT;
    if (best_score <= original_alpha) {
        flag = TT_FLAG_UPPERBOUND;
    } else if (best_score >= beta) {
        flag = TT_FLAG_LOWERBOUND;
    }
    
    tt_save(ctx->board.hash, rem_depth, scoreToTT(best_score, depth), flag, best_move);

    return best_score;
}

Position getAIMove(const GameState *game) {
    if(game->moveCount == 0) {
        // 如果是第一步，落子在棋盘中心
        return (Position){BOARD_SIZE / 2, BOARD_SIZE / 2};
    }

    //初始化eval
    SearchContext ctx;
    memset(&ctx, 0, sizeof(SearchContext));
    ctx.board = game->bitBoard;
    initEvalState(&ctx.board, &ctx.eval);
    Player me = game->currentPlayer;
    if (ctx.board.hash == 0) {
        ctx.board.hash = calculateZobristHash(&ctx.board, me);
    }

    // 初始化置换表
    static int tt_initialized = 0;
    if (!tt_initialized) {
        initZobrist();
        tt_init(64); // 64MB
        tt_initialized = 1;
    }
    

    //迭代加深搜索
    Position moves[225];
    int count = generateMoves(&ctx.board, moves);
    // if (count == 0) return (Position){7, 7}; // 理论上不会发生

    Position best_move = moves[0];
    int best_score = -INF;
    UndoInfo undo;
    
    for (int depth = 2; depth <= SEARCH_DEPTH; depth += 2) {
        // 对根节点走法排序
        Position sorted_moves[BEAM_WIDTH + 1] = {0};
        int limit;

        // 查询置换表中的根节点走法
        int tt_val;
        Position tt_root_move = INVALID_POS;
        {
            int _ra = -INF, _rb = INF;
            tt_probe(ctx.board.hash, depth, &_ra, &_rb, &tt_val, &tt_root_move);
        }

        // 走法排序：将置换表中的最佳走法放在首位
        limit = sortMoves(&ctx, moves, sorted_moves, tt_root_move, count, 0, me);

        int current_best_score = -INF;
        Position current_best_move = sorted_moves[0];
        int alpha = -INF;
        int beta = INF;
        
        for (int i = 0; i < limit; i++) {
            aiMakeMove(&ctx.board, &ctx.eval, sorted_moves[i].row, sorted_moves[i].col, me, &undo);
            
            // 检查走法是否为禁手（仅对黑棋）
            if (me == PLAYER_BLACK && ctx.eval.total_score == -INF) {
                 // printf("根节点走法 (%d, %d) 是禁手，跳过。\n", sorted_moves[i].row, sorted_moves[i].col);
                 aiUnmakeMove(&ctx.board, &ctx.eval, sorted_moves[i].row, sorted_moves[i].col, me, &undo);
                 continue;
            }
            // 检查根节点是否直接获胜
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
        
        // 如果更深层搜索结果极低（被迫输），则不更新 best_move / best_score。
        // 这样可以避免ai在对方棋力不如自己的时候开摆
        if (current_best_score > -WIN_THRESHOLD) {
            best_score = current_best_score;
            best_move = current_best_move;  
        } 
        printf("Depth %d: Best Move (%d, %d), Score %d\nMove List:", depth, best_move.row, best_move.col, best_score);
        for(int i = 0; i < limit; i++){
            printf("(%d, %d) ", sorted_moves[i].row, sorted_moves[i].col);
        }
        printf("\n");
        //有胜手了就提前退出第0层搜索
        if (best_score > WIN_THRESHOLD) break;
    }
    //跟踪best move
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
    // Set ascii face flag according to final best_score
    if (best_score > WIN_THRESHOLD ) {
        setAsciiFaceFlag(1); // 找到胜手来
    } else if (best_score < -WIN_THRESHOLD / 3) {
        setAsciiFaceFlag(-1); // 可能要输
    } else {
        setAsciiFaceFlag(0); // common
    }
    printf("AI selects move (%d, %d) with score %d after searching %lld nodes.\n", best_move.row, best_move.col, best_score, ctx.nodes_searched);
    return best_move;
}
