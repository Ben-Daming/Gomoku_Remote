#ifndef BOARD_H
#define BOARD_H

#include "types.h"

void initGame(GameState *game, GameMode mode, RuleType rule);
void printBoard(const GameState *game);
int makeMove(GameState *game, int row, int col); // Returns 1 if success, 0 if fail
int isBoardFull(const GameState *game);

#endif
