#ifndef TYPES_H
#define TYPES_H

#define BOARD_SIZE 15

typedef enum {
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
} CellState;

typedef enum {
    PLAYER_BLACK = 1,
    PLAYER_WHITE = 2
} Player;

typedef enum {
    MODE_PVP,
    MODE_PVE
} GameMode;

typedef enum {
    RULE_STANDARD, // Standard Renju
    RULE_NO_FORBIDDEN // Free style
} RuleType;

typedef struct {
    int row;
    int col;
} Position;

typedef unsigned short Line;

typedef struct {
    Line black[BOARD_SIZE];      // 1 = 黑子, 0 = 非黑子
    Line white[BOARD_SIZE];      // 1 = 白子, 0 = 非白子
    Line occupy[BOARD_SIZE];     // 0 = 已被占据(黑或白), 1 = 空
    Line move_mask[BOARD_SIZE];  // 1 = 有效落子点(邻域), 0 = 无效
} BitBoardState;

struct HistoryNode;

typedef struct {
    CellState board[BOARD_SIZE][BOARD_SIZE];
    BitBoardState bitBoard; 
    Player currentPlayer;
    Position lastMove;
    int moveCount;
    GameMode mode;
    RuleType ruleType;
    struct HistoryNode* historyHead;
} GameState;

typedef struct HistoryNode {
    CellState board[BOARD_SIZE][BOARD_SIZE];
    BitBoardState bitBoard; 
    Player currentPlayer;
    Position lastMove;
    int moveCount;
    struct HistoryNode* next;
} HistoryNode;

#endif
