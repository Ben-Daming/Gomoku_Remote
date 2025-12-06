#ifndef TT_H
#define TT_H

#include <stdint.h>
#include "types.h"

// TT Entry Flags
#define TT_FLAG_NONE 0
#define TT_FLAG_EXACT 1
#define TT_FLAG_LOWERBOUND 2
#define TT_FLAG_UPPERBOUND 3

// Transposition Table Entry (16 bytes)
typedef struct {
    uint64_t key;               // 8 bytes: Hash Key

    // 4 bytes (Packed Control Word)
    uint32_t rem_depth : 7;      // Remaining depth (0~127)
    uint32_t abs_depth : 7;      // Absolute depth (0~127) - Optional/Reserved
    uint32_t flag      : 2;      // Node type (EXACT/LOWER/UPPER)
    uint32_t age       : 8;      // Generation/Age (0~255)
    uint32_t best_move : 8;      // Best move packed (row:4, col:4) -> Wait, 4 bits is not enough for 15.
                                 // Need 8 bits for row/col combined? 
                                 // 15*15 = 225 < 256. So 8 bits is enough for index (row*15+col) or (row<<4 | col).
                                 // Let's use 8 bits for packed position: (row << 4) | col. Max row/col is 14. 14<<4 | 14 = 224+14 = 238 < 255.

    int32_t value;              // 4 bytes: Evaluation Score

} TTEntry;

// Initialize TT with size in MB
void tt_init(int size_mb);

// Free TT memory
void tt_free();

// Clear TT (reset entries)
void tt_clear();

// Probe TT
// Returns 1 if entry found and usable (based on depth/bounds), 0 otherwise.
// out_val: The score from TT
// out_move: The best move from TT (always returned if present, regardless of depth)
// alpha, beta: Current search bounds
// rem_depth: Current remaining depth
// tt_probe now accepts pointers to alpha/beta so it can tighten the window
// based on TT bounds. It updates *alpha and/or *beta if entry provides
// a tighter bound but does not force a cutoff. Returns 1 if the entry
// is usable to return a score immediately (cutoff or exact).
int tt_probe(uint64_t key, int rem_depth, int* alpha, int* beta, int* out_val, Position* out_move);

// Save to TT
void tt_save(uint64_t key, int rem_depth, int value, int flag, Position best_move);

// Prefetch TT entry
void tt_prefetch(uint64_t key);

#endif // TT_H
