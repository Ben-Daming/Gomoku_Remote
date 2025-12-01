#include "../include/bitboard.h"
#include <string.h> // For memset
#include <stdlib.h> // For rand

// Helper macro for absolute value
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
// Helper macro for bit set (Standard Bit Order: Row 0 = Bit 0)
// Returns a mask with the 'row'-th bit set to 0, others to 1.
#define BIT_SET(row) (~(1 << (row)))

// Neighborhood Masks (5x5 area)
// Optimized for Standard Bit Order (Row 0 = Bit 0)
// Using Symmetric 5x5 Box for all distances to ensure no potential connections are missed.
// bit_move_set[row][0]: Mask for the same column
// bit_move_set[row][1]: Mask for adjacent columns (Dist 1)
// bit_move_set[row][2]: Mask for adjacent columns (Dist 2)
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
// --- Public API Implementation ---

void initBitBoard(BitBoardState *bitBoard) {
    // 1. Initialize Black/White/MoveMask to 0
    memset(&bitBoard->black, 0, sizeof(bitBoard->black));
    memset(&bitBoard->white, 0, sizeof(bitBoard->white));
    memset(bitBoard->move_mask, 0, sizeof(bitBoard->move_mask));

    // 2. Initialize Occupy to ~0 (All 1s = All Empty)
    for (int i = 0; i < BOARD_SIZE; i++) {
        bitBoard->occupy[i] = (Line)~0;
    }
}

void getMoveMask(const BitBoardState *bitBoard, Line* buffer) {
    memcpy(buffer, bitBoard->move_mask, sizeof(bitBoard->move_mask));
}

void updateBitBoard(BitBoardState *bitBoard, int row, int col, Player player, Line* backup_mask) {
    // 1. Fast Backup of move_mask
    memcpy(backup_mask, bitBoard->move_mask, sizeof(Line) * BOARD_SIZE);

    // 2. Update Color Layers (4 Directions)
    PlayerBitBoard *pBoard = (player == PLAYER_BLACK) ? &bitBoard->black : &bitBoard->white;
    
    // Vertical: Index col, Bit row
    pBoard->cols[col] |= (1 << row);
    
    // Horizontal: Index row, Bit col
    pBoard->rows[row] |= (1 << col);
    
    // Main Diagonal: Index row - col + 14, Bit col
    pBoard->diag1[row - col + 14] |= (1 << MIN(row, col));
    
    // Anti Diagonal: Index row + col, Bit row
    pBoard->diag2[row + col] |= (1 << MIN(row, 14 - col));

    // 3. Update Occupy Layer (1=Empty, 0=Occupied)
    bitBoard->occupy[col] &= BIT_SET(row);

    // 4. Update Neighborhood Layer (Move Mask)
    int start_col = (col - 2 < 0) ? 0 : col - 2;
    int end_col = (col + 2 >= BOARD_SIZE) ? BOARD_SIZE - 1 : col + 2;

    for (int c = start_col; c <= end_col; c++) {
        int dist = ABS(c - col);
        bitBoard->move_mask[c] |= bit_move_set[row][dist];
        
        // Only keep empty spots
        bitBoard->move_mask[c] &= bitBoard->occupy[c];
    }
}

void undoBitBoard(BitBoardState *bitBoard, int row, int col, Player player, const Line* backup_mask) {
    // 1. Restore Color Layers (4 Directions)
    PlayerBitBoard *pBoard = (player == PLAYER_BLACK) ? &bitBoard->black : &bitBoard->white;
    
    pBoard->cols[col] &= ~(1 << row);
    pBoard->rows[row] &= ~(1 << col);
    pBoard->diag1[row - col + 14] &= ~(1 << MIN(row, col));
    pBoard->diag2[row + col] &= ~(1 << MIN(row, 14 - col));

    // 2. Restore Occupy Layer (Set back to 1)
    bitBoard->occupy[col] |= ~BIT_SET(row);

    // 3. Restore Neighborhood Layer
    memcpy(bitBoard->move_mask, backup_mask, sizeof(Line) * BOARD_SIZE);
}

int generateMoves(const BitBoardState *bitBoard, Position *moves) {
    int count = 0;
    for (int col = 0; col < BOARD_SIZE; col++) {
        // Valid moves: In move_mask AND Empty (occupy bit is 1)
        Line mask = bitBoard->move_mask[col] & bitBoard->occupy[col];
        while (mask) {
            // __builtin_ctz returns the number of trailing zeros (index of the lowest set bit)
            int row = __builtin_ctz(mask);
            
            moves[count].row = row;
            moves[count].col = col;
            count++;
            
            // Clear the lowest set bit
            mask &= (mask - 1);
        }
    }
    return count;
}
