#include "../include/bitboard.h"
#include "../include/zobrist.h"
#include <string.h> 
#include <stdlib.h> 

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
// 返回一个掩码，其中第'row'位为0，其他位为1。
#define BIT_SET(row) (~(1 << (row)))

// 邻域掩码（5x5区域）
// bit_move_set[row][0]：同一列的掩码
// bit_move_set[row][1]：相邻列（距离1）的掩码
// bit_move_set[row][2]：相邻列（距离2）的掩码
const Line bit_move_set[BOARD_SIZE][3] = {
{0b0000000000000111, 
 0b0000000000000011, 
 0b0000000000000101},
{0b0000000000001111, 
 0b0000000000000111, 
 0b0000000000001010},
{0b0000000000011111,
 0b0000000000001110,
 0b0000000000010101},
{0b0000000000111110,
 0b0000000000011100,
 0b0000000000101010},
{0b0000000001111100,
 0b0000000000111000,
 0b0000000001010100},
{0b0000000011111000,
 0b0000000001110000,
 0b0000000010101000},
{0b0000000111110000,
 0b0000000011100000,
 0b0000000101010000},
{0b0000001111100000,
 0b0000000111000000,
 0b0000001010100000},
{0b0000011111000000,
 0b0000001110000000,
 0b0000010101000000},
{0b0000111110000000,
 0b0000011100000000,
 0b0000101010000000},
{0b0001111100000000,
 0b0000111000000000,
 0b0001010100000000},
{0b0011111000000000,
 0b0001110000000000,
 0b0010101000000000},
{0b0111110000000000,
 0b0011100000000000,
 0b0101010000000000},
{0b0111100000000000,
 0b0111000000000000,
 0b0010100000000000},
{0b0111000000000000,
 0b0110000000000000,
 0b0101000000000000}  
};

// --- API ---

void initBitBoard(BitBoardState *bitBoard) {
    // 初始化黑子/白子/可落子掩码为0
    memset(&bitBoard->black, 0, sizeof(bitBoard->black));
    memset(&bitBoard->white, 0, sizeof(bitBoard->white));
    memset(bitBoard->move_mask, 0, sizeof(bitBoard->move_mask));

    // 初始化占位层为~0（全1表示全空）
    for (int i = 0; i < BOARD_SIZE; i++) {
        bitBoard->occupy[i] = (Line)(~0) >> 1;
    }
    
    // 初始化哈希值
    bitBoard->hash = 0; 
}

//弃用
// void getMoveMask(const BitBoardState *bitBoard, Line* buffer) {
//     // 获取当前掩码
//     memcpy(buffer, bitBoard->move_mask, sizeof(bitBoard->move_mask));
// }

void updateBitBoard(BitBoardState *bitBoard, int row, int col, Player player, Line* backup_mask) {
    // 1. 快速备份可落子掩码
    memcpy(backup_mask, bitBoard->move_mask, sizeof(Line) * BOARD_SIZE);

    // 2. 更新颜色层（4个方向）
    PlayerBitBoard *pBoard = (player == PLAYER_BLACK) ? &bitBoard->black : &bitBoard->white;
    
    // 垂直方向：索引为col，位为row
    pBoard->cols[col] |= (1 << row);
    
    // 水平方向：索引为row，位为col
    pBoard->rows[row] |= (1 << col);
    
    // 主对角线：索引为row - col + 14，位为MIN(row, col)
    pBoard->diag1[row - col + 14] |= (1 << MIN(row, col));
    
    // 副对角线：索引为row + col，位为MIN(row, 14 - col)
    pBoard->diag2[row + col] |= (1 << MIN(row, 14 - col));

    // 3. 更新Zobrist哈希值
    bitBoard->hash ^= zobrist_table[row][col][player == PLAYER_BLACK ? 0 : 1];
    bitBoard->hash ^= zobrist_player; // 切换回合哈希

    // 3. 更新占位层（1=空，0=已占）
    bitBoard->occupy[col] &= BIT_SET(row);

    // 4. 更新邻域层（可落子掩码）
    int start_col = (col - 2 < 0) ? 0 : col - 2;
    int end_col = (col + 2 >= BOARD_SIZE) ? BOARD_SIZE - 1 : col + 2;

    for (int c = start_col; c <= end_col; c++) {
        int dist = ABS(c - col);
        bitBoard->move_mask[c] |= bit_move_set[row][dist];
        
        // 只保留空位
        bitBoard->move_mask[c] &= bitBoard->occupy[c];
    }
}

void undoBitBoard(BitBoardState *bitBoard, int row, int col, Player player, const Line* backup_mask) {
    // 1. 恢复颜色层（4个方向）
    PlayerBitBoard *pBoard = (player == PLAYER_BLACK) ? &bitBoard->black : &bitBoard->white;
    
    pBoard->cols[col] &= ~(1 << row);
    pBoard->rows[row] &= ~(1 << col);
    pBoard->diag1[row - col + 14] &= ~(1 << MIN(row, col));
    pBoard->diag2[row + col] &= ~(1 << MIN(row, 14 - col));

    // 2. 恢复哈希值
    bitBoard->hash ^= zobrist_table[row][col][player == PLAYER_BLACK ? 0 : 1];
    bitBoard->hash ^= zobrist_player; // 回退回合哈希

    // 3. 恢复占位层（重新设为1）
    bitBoard->occupy[col] |= ~BIT_SET(row);

    // 4. 恢复邻域层
    memcpy(bitBoard->move_mask, backup_mask, sizeof(Line) * BOARD_SIZE);
}

int generateMoves(const BitBoardState *bitBoard, Position *moves) {
    int count = 0;
    for (int col = 0; col < BOARD_SIZE; col++) {
        // 合法落子：在可落子掩码中且为空（占位位为1）
        Line mask = bitBoard->move_mask[col] & bitBoard->occupy[col];
        while (mask) {
            // __builtin_ctz返回最低位1的索引（即最低位的行号）
            int row = __builtin_ctz(mask);
            
            moves[count].row = row;
            moves[count].col = col;
            count++;
            
            // 清除最低位的1
            mask &= (mask - 1);
        }
    }
    return count;
}
