#ifndef RULES_H
#define RULES_H

#include "types.h"

// Validation result codes
#define VALID_MOVE 0
#define ERR_OUT_OF_BOUNDS 1
#define ERR_OCCUPIED 2
#define ERR_FORBIDDEN_33 3
#define ERR_FORBIDDEN_44 4
#define ERR_FORBIDDEN_OVERLINE 5

int checkValidMove(const GameState *game, int row, int col);
int checkWin(const GameState *game); // Returns winner (PLAYER_BLACK/WHITE) or 0
int isForbidden(const GameState *game, int row, int col);

#endif
