#ifndef TT_H
#define TT_H

#include <stdint.h>
#include "types.h"

// TT Flags
#define TT_FLAG_NONE 0
#define TT_FLAG_EXACT 1
#define TT_FLAG_LOWERBOUND 2
#define TT_FLAG_UPPERBOUND 3

// TT Entry的结构
typedef struct {
    uint64_t key;               

    uint32_t rem_depth : 7;      
    uint32_t abs_depth : 7;      
    uint32_t flag      : 2;      
    uint32_t age       : 8;      
    uint32_t best_move : 8;      
    int32_t value;              
} TTEntry;

// 用size_mb MB初始化置换表
void tt_init(int size_mb);

// 释放置换表内存
void tt_free();

// 清空置换表
void tt_clear();

// 查询置换表（TT）
// 如果找到并且可用（基于深度/边界），返回1，否则返回0。
// out_val: 从TT中获取的分数
// out_move: 从TT中获取的最佳走法（只要存在总是返回，无论深度如何）
// alpha, beta: 当前搜索窗口上下界
// rem_depth: 当前剩余深度
// tt_probe 现在接受alpha/beta的指针，可以根据TT中的边界收紧窗口。
// 如果TT条目提供了更紧的边界，会更新*alpha和/或*beta，但不会强制剪枝。
// 如果条目可直接返回分数（剪枝或精确），则返回1。
int tt_probe(uint64_t key, int rem_depth, int* alpha, int* beta, int* out_val, Position* out_move);

// 保存到置换表（TT）
void tt_save(uint64_t key, int rem_depth, int value, int flag, Position best_move);

// 预取TT条目
void tt_prefetch(uint64_t key);

#endif 
