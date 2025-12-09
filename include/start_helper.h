#ifndef START_HELPER_H
#define START_HELPER_H

#include "types.h"

// 调用开局定式辅助函数
// 返回1表示已下子(开局模块处理完成), 返回0表示不匹配任何硬编码开局
int start_helper(GameState *game);

#endif
