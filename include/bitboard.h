#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"

// Initialize Zobrist Table (Call once at startup)
void initZobrist();

// Initialize the BitBoard system
void initBitBoard(BitBoardState *bitBoard);

// Get the current move mask (neighborhood of occupied cells)
void getMoveMask(const BitBoardState *bitBoard, Line* buffer);

// Update the BitBoard after a move
// backup_mask: [OUT] Buffer to store the previous move_mask (size: BOARD_SIZE * sizeof(Line))
void updateBitBoard(BitBoardState *bitBoard, int row, int col, Player player, Line* backup_mask);

// Undo a move on the BitBoard
// backup_mask: [IN] The move_mask stored during update
void undoBitBoard(BitBoardState *bitBoard, int row, int col, Player player, const Line* backup_mask);

// Generate all valid moves based on move_mask
// moves: [OUT] Buffer to store generated moves (max 225)
// Returns: Number of moves generated
int generateMoves(const BitBoardState *bitBoard, Position *moves);

#endif // BITBOARD_H
