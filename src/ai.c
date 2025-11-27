#include <stdlib.h>
#include "../include/ai.h"
#include "../include/rules.h"

Position getAIMove(const GameState *game) {
    Position pos;
    pos.row = -1;
    pos.col = -1;

    // Simple strategy: Find the first valid move (Placeholder for Minimax)
    // Better: Pick a random valid move to avoid deterministic stupidity
    
    // Try center first if empty
    if (checkValidMove(game, 7, 7) == VALID_MOVE) {
        pos.row = 7;
        pos.col = 7;
        return pos;
    }

    // Scan for any valid move
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (checkValidMove(game, i, j) == VALID_MOVE) {
                pos.row = i;
                pos.col = j;
                return pos; // Return first valid found
            }
        }
    }

    return pos;
}
