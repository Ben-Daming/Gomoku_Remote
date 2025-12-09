#include "../include/tt.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static TTEntry* tt_table = NULL;
static uint64_t tt_size = 0; // 表项数量
static uint64_t tt_mask = 0; // 用于快速索引的掩码（size - 1）

// helper 打包/解包走子
static inline uint8_t packMove(Position p) {
    if (p.row == -1 || p.col == -1) return 0xFF; // -1 表示无效走子
    return (uint8_t)((p.row << 4) | p.col);
}

static inline Position unpackMove(uint8_t packed) {
    if (packed == 0xFF) return (Position){-1, -1}; // 0xFF 表示无效走子
    return (Position){(packed >> 4) & 0xF, packed & 0xF};
}

void tt_init(int size_mb) {
    if (tt_table) free(tt_table);
    
    // 计算表项数量
    // 保证大小为2的幂，便于快速索引
    uint64_t bytes = (uint64_t)size_mb * 1024 * 1024;
    uint64_t count = bytes / sizeof(TTEntry);
    
    // 找到不大于count的最近的2的幂
    tt_size = 1;
    while (tt_size * 2 <= count) {
        tt_size *= 2;
    }
    tt_mask = tt_size - 1;
    
    tt_table = (TTEntry*)calloc(tt_size, sizeof(TTEntry));
    printf("TT Initialized: %d MB, %lu entries\n", size_mb, tt_size);
}

void tt_free() {
    if (tt_table) {
        free(tt_table);
        tt_table = NULL;
    }
}

void tt_clear() {
    if (tt_table) {
        memset(tt_table, 0, tt_size * sizeof(TTEntry));
    }
}

int tt_probe(uint64_t key, int rem_depth, int* alpha, int* beta, int* out_val, Position* out_move) {
    if (!tt_table) return 0;

    uint64_t index = key & tt_mask;
    TTEntry* entry = &tt_table[index];

    if (entry->key == key) {
        // 取出最佳走子用于排序
        *out_move = unpackMove((uint8_t)entry->best_move);

        // 检查该表项是否可用于剪枝
        if (entry->rem_depth >= rem_depth) {
            int score = entry->value;
            // 如有必要可调整将死分数（此处省略）

            if (entry->flag == TT_FLAG_EXACT) {
                *out_val = score;
                return 1;
            }

            if (entry->flag == TT_FLAG_LOWERBOUND) {
                // 真正分数 >= entry->value
                if (score >= *beta) {
                    *out_val = score; // 剪枝
                    return 1;
                }
                if (score > *alpha) {
                    *alpha = score; // 收紧alpha
                }
            }

            if (entry->flag == TT_FLAG_UPPERBOUND) {
                // 真正分数 <= entry->value
                if (score <= *alpha) {
                    *out_val = score; // 剪枝
                    return 1;
                }
                if (score < *beta) {
                    *beta = score; // 收紧beta
                }
            }

            // 收紧后，检查是否立即剪枝
            if (*alpha >= *beta) {
                *out_val = *alpha; 
                return 1;
            }
        }
    } else {
        *out_move = (Position){-1, -1};
    }
    return 0;
}

void tt_save(uint64_t key, int rem_depth, int value, int flag, Position best_move) {
    if (!tt_table) return;

    uint64_t index = key & tt_mask;
    TTEntry* entry = &tt_table[index];

    // 替换策略：
    // 1. 如果为空（key==0）则总是替换
    // 2. 如果新深度更大或相等则替换
    
    if (entry->key == 0 || rem_depth >= entry->rem_depth) {
        
        entry->rem_depth = rem_depth;
        entry->value = value;
        entry->flag = flag;
        entry->best_move = packMove(best_move);
        
        entry->key = key;
    }
}

void tt_prefetch(uint64_t key) {
    if (!tt_table) return;
    uint64_t index = key & tt_mask;
    __builtin_prefetch(&tt_table[index]); // 预取缓存行
}
