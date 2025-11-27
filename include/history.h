#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"

void pushState(GameState *game);
int undoMove(GameState *game); // Returns 1 if success, 0 if empty
void clearHistory(GameState *game);

#endif
