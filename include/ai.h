#ifndef AI_H
#define AI_H

#include "types.h"
#include "bitboard.h"
#include <stdint.h>

// --- 搜索参数 ---
#define SEARCH_DEPTH 12
#define BEAM_WIDTH 10
#define MAX_DEPTH (SEARCH_DEPTH + 1)


#define MAX_LINES 30 // 最大对角线数为29
#define INVALID_POS ((Position){-1, -1})


typedef struct {
    // 缓存4个方向的净分（黑分-白分）
    // [0]: 列, [1]: 行, [2]: 主对角线, [3]: 副对角线
    long long line_net_scores[4][MAX_LINES];

    // 缓存上一步可能触发禁手的总数目
    long long count_live3[4][MAX_LINES];
    long long count_4[4][MAX_LINES];

    long long total_score; // 全局分数 = 所有方向净分之和
} EvalState;


typedef struct {
    Line move_mask_backup[15]; // 备份邻域掩码
    long long old_line_net_scores[4]; // 备份受影响的4条线的旧分数
    long long old_total_score;
    long long old_count_live3[4]; // 备份受影响的4条线的旧活三数
    long long old_count_4[4]; // 备份受影响的4条线的旧四数
} UndoInfo;




typedef struct {
    Position killer_moves[MAX_DEPTH][2]; // 杀手着法
    unsigned long long nodes_searched;   // 已搜索节点数

    // 线程本地棋盘与评估状态
    BitBoardState board;
    EvalState eval;
} SearchContext;

Position getAIMove(const GameState *game); // 获取AI落子

#endif
