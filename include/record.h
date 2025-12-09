#ifndef RECORD_H
#define RECORD_H
#include "types.h"
#include <time.h>

//记录棋谱
int record(GameState* game);

//导入固定格式的棋谱
int load(GameState* game, char* filename);

#endif