#include "../include/ai.h"
#include "../include/bitboard.h"
#include "../include/evaluate.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

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

void clearHeuristics(SearchContext* ctx) {
    memset(ctx->killer_moves, 0, sizeof(ctx->killer_moves));
    memset(ctx->history_table, 0, sizeof(ctx->history_table));
    ctx->nodes_searched = 0;
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

// 排序辅助结构体
typedef struct {
    Position move;
    int score;
} ScoredMove;

// Forward declarations for static functions used in sortMoves
static void aiMakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, UndoInfo* undo);
static void aiUnmakeMove(BitBoardState* board, EvalState* eval, int row, int col, Player player, const UndoInfo* undo);

// 降序比较函数
static int compareScoredMoves(const void* a, const void* b) {
    const ScoredMove* sa = (const ScoredMove*)a;
    const ScoredMove* sb = (const ScoredMove*)b;
    return sb->score - sa->score; // 降序
}

// Helper: Sort moves based on heuristics
// Order: TT Move > Killer Moves > Static Eval
static void sortMoves(SearchContext* ctx, Position* moves, int count, Position ttMove, int depth, Player player) {
    ScoredMove scored_moves[225];
    
    for (int i = 0; i < count; i++) {
        int score = 0;
        
        // 1. TT Move (Highest Priority)
        if (moves[i].row == ttMove.row && moves[i].col == ttMove.col) {
            score = 100000000; // Increase to ensure it's always top
        }
        // 2. Killer Moves
        else if ((moves[i].row == ctx->killer_moves[depth][0].row && moves[i].col == ctx->killer_moves[depth][0].col) ||
                 (moves[i].row == ctx->killer_moves[depth][1].row && moves[i].col == ctx->killer_moves[depth][1].col)) {
            score = 90000000;
        }
        // 3. Static Evaluation (Beam Search Prep)
        else {
            UndoInfo undo;
            aiMakeMove(&ctx->board, &ctx->eval, moves[i].row, moves[i].col, player, &undo);
            
            // Score from the perspective of the player making the move
            // If player is BLACK, total_score is Black - White. We want max.
            // If player is WHITE, total_score is Black - White. We want min (so -total_score).
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

// 辅助函数：获取一条线的长度
static int getLineLength(int dir, int idx) {
    if (dir == DIR_COL || dir == DIR_ROW) return BOARD_SIZE;
    return BOARD_SIZE - ABS(idx - (BOARD_SIZE - 1));
}

// 辅助函数：从位棋盘获取某条线的状态
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

// 初始化评估状态
static void initEvalState(const BitBoardState* board, EvalState* eval) {
    eval->total_score = 0;
    memset(eval->line_net_scores, 0, sizeof(eval->line_net_scores));

    // 1. 列与行
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

    // 2. 对角线
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
    // 计算索引
    int indices[4];
    indices[0] = col;
    indices[1] = row;
    indices[2] = row - col + (BOARD_SIZE - 1);
    indices[3] = row + col;

    // 1. 备份旧分数并从总分中移除
    for (int i = 0; i < 4; i++) {
        int idx = indices[i];
        undo->old_line_net_scores[i] = eval->line_net_scores[i][idx];
        eval->total_score -= undo->old_line_net_scores[i];
    }

    // 2. 更新位棋盘（传递move_mask备份缓冲区）
    updateBitBoard(board, row, col, player, undo->move_mask_backup);

    // 3. 计算新分数
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
    // 1. 恢复位棋盘
    undoBitBoard(board, row, col, player, undo->move_mask_backup);

    // 计算索引
    int indices[4];
    indices[0] = col;
    indices[1] = row;
    indices[2] = row - col + (BOARD_SIZE - 1);
    indices[3] = row + col;

    // 2. 恢复分数
    for (int i = 0; i < 4; i++) {
        int idx = indices[i];
        eval->total_score -= eval->line_net_scores[i][idx]; // 移除当前分数
        eval->line_net_scores[i][idx] = undo->old_line_net_scores[i]; // 恢复旧分数
        eval->total_score += undo->old_line_net_scores[i]; // 加回旧分数
    }
}

// 带置换表的Alpha-Beta搜索
static int alphaBeta(SearchContext* ctx, int depth, int alpha, int beta, Player player) {
    // 0. 置换表查找
    int tt_score;
    Position tt_move = {-1, -1};
    if (probeTT(ctx->board.hash, depth, alpha, beta, &tt_score, &tt_move)) {
        return tt_score; // 基于置换表剪枝
    }

    // 1. 静态评估
    int current_score = (player == PLAYER_BLACK) ? ctx->eval.total_score : -ctx->eval.total_score;

    if (current_score > WIN_THRESHOLD) return current_score; // 胜利
    if (current_score < -WIN_THRESHOLD) return current_score; // 失败

    if (depth == 0) {
        // 保存叶子节点到置换表（精确值）
        saveTT(ctx->board.hash, 0, current_score, TT_FLAG_EXACT, (Position){-1, -1});
        return current_score;
    }

    // 2. 生成走法
    Position moves[225];
    int count = generateMoves(&ctx->board, moves);
    if (count == 0) return 0; // 平局

    // 3. 走法排序
    sortMoves(ctx, moves, count, tt_move, depth, player);

    int best_score = -INF;
    Position best_move = {-1, -1};
    int tt_flag = TT_FLAG_UPPERBOUND; // 默认上界（失败低）

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
            best_move = moves[i];
            
            if (score > alpha) {
                alpha = score;
                tt_flag = TT_FLAG_EXACT; // 找到更好的走法，至少为精确值（主变节点）
            }
        }
        
        if (alpha >= beta) {
            tt_flag = TT_FLAG_LOWERBOUND; // 失败高（β剪枝）
            
            // 剪枝时更新启发式
            // 1. 杀手走法（如果未存储则存储）
            if (moves[i].row != ctx->killer_moves[depth][0].row || moves[i].col != ctx->killer_moves[depth][0].col) {
                ctx->killer_moves[depth][1] = ctx->killer_moves[depth][0];
                ctx->killer_moves[depth][0] = moves[i];
            }
            
            // 2. 历史启发
            ctx->history_table[moves[i].row][moves[i].col] += depth * depth;
            
            break;
        }
    }

    // 4. 保存到置换表
    saveTT(ctx->board.hash, depth, best_score, tt_flag, best_move);

    return best_score;
}

// MTD(f) 驱动函数
static int mtdf(SearchContext* ctx, int first_guess, int depth, Player player) {
    int g = first_guess;
    int upper = INF;
    int lower = -INF;

    while (lower < upper) {
        int beta = (g == lower) ? g + 1 : g;
        
        // 空窗口搜索：alpha = beta - 1
        int score = alphaBeta(ctx, depth, beta - 1, beta, player);
        
        if (score < beta) {
            upper = score;
        } else {
            lower = score;
        }
        g = score;
    }
    return g;
}

// --- Lazy SMP Implementation ---

typedef struct {
    int id;
    BitBoardState board;
    Player player;
} ThreadData;

static void* searchWorker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    // 初始化线程局部上下文
    SearchContext ctx;
    ctx.thread_id = data->id;
    clearHeuristics(&ctx);
    ctx.board = data->board;
    initEvalState(&ctx.board, &ctx.eval);
    
    int guess = 0;
    
    // 迭代加深
    // 所有线程都运行相同的搜索逻辑
    // 通过共享置换表（无锁竞争）来共享信息
    for (int depth = 1; depth <= SEARCH_DEPTH; depth++) {
        guess = mtdf(&ctx, guess, depth, data->player);
        
        // 若已找到胜负线则提前终止
        if (guess > WIN_THRESHOLD || guess < -WIN_THRESHOLD) break;
    }
    return NULL;
}

Position getAIMove(const GameState *game) {
    if(game->moveCount == 0) {
        // 若是第一步，直接下在中间
        return (Position){BOARD_SIZE / 2, BOARD_SIZE / 2};
    }
    // 0. 若未初始化置换表则初始化（如64MB）
    if (!tt_table) {
        initTT(64); 
    }
    
    // 1. 启动多线程搜索 (Lazy SMP)
    #define NUM_THREADS 4
    pthread_t threads[NUM_THREADS];
    ThreadData t_data[NUM_THREADS];
    
    for(int i = 0; i < NUM_THREADS; i++) {
        t_data[i].id = i;
        t_data[i].board = game->bitBoard; // 复制棋盘状态
        t_data[i].player = game->currentPlayer;
        
        if (pthread_create(&threads[i], NULL, searchWorker, &t_data[i]) != 0) {
            // 如果创建失败，回退到单线程（或者直接报错，这里简单处理）
            // 实际生产中应有更好的错误处理
        }
    }
    
    // 2. 等待所有线程结束
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // 3. 从置换表获取最佳走法
    Position bestMove = {-1, -1};
    int score;
    // 使用当前局面的哈希查询置换表
    probeTT(game->bitBoard.hash, 0, -INF, INF, &score, &bestMove);
    
    return bestMove;
}
