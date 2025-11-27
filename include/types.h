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
    RULE_STANDARD, // Standard Gomoku with forbidden moves for Black
    RULE_NO_FORBIDDEN // Free style
} RuleType;

typedef struct {
    int row;
    int col;
} Position;

// Forward declaration
struct HistoryNode;

typedef struct {
    CellState board[BOARD_SIZE][BOARD_SIZE];
    Player currentPlayer;
    Position lastMove;
    int moveCount;
    GameMode mode;
    RuleType ruleType;
    struct HistoryNode* historyHead;
} GameState;

typedef struct HistoryNode {
    CellState board[BOARD_SIZE][BOARD_SIZE];
    Player currentPlayer;
    Position lastMove;
    int moveCount;
    struct HistoryNode* next;
} HistoryNode;

#endif
