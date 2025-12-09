#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"

// 初始化位棋盘
void initBitBoard(BitBoardState *bitBoard);

//弃用
// // 获取当前落子掩码
// void getMoveMask(const BitBoardState *bitBoard, Line* buffer);

// 落子后更新 BitBoard
// backup_mask: [OUT] 用于存储之前 move_mask 的缓冲区（大小：BOARD_SIZE * sizeof(Line)）
void updateBitBoard(BitBoardState *bitBoard, int row, int col, Player player, Line* backup_mask);

// 撤销 BitBoard 上的一步落子
// backup_mask: [IN] 在更新时存储的 move_mask
void undoBitBoard(BitBoardState *bitBoard, int row, int col, Player player, const Line* backup_mask);

// 根据 move_mask 生成所有合法落子
// moves: [OUT] 用于存储合法的落子点（最多 225 个）
// 返回值：生成的落子数量
int generateMoves(const BitBoardState *bitBoard, Position *moves);

#endif
