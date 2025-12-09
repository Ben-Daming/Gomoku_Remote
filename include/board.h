#ifndef BOARD_H
#define BOARD_H

#include "types.h"

void initGame(GameState *game, GameMode mode, RuleType rule);
void printBoard(const GameState *game);

//1 if ok, else error
int makeMove(GameState *game, int row, int col); 
int isBoardFull(const GameState *game);

#endif
