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

// Structure to hold bitboards for different directions for a single player
typedef struct {
    Line cols[BOARD_SIZE];          // Vertical (Index: col, Bit: row)
    Line rows[BOARD_SIZE];          // Horizontal (Index: row, Bit: col)
    Line diag1[BOARD_SIZE * 2];     // Main Diagonal (Index: row - col + 14, Bit: col)
    Line diag2[BOARD_SIZE * 2];     // Anti Diagonal (Index: row + col, Bit: row)
} PlayerBitBoard;

typedef struct {
    PlayerBitBoard black;
    PlayerBitBoard white;
    Line occupy[BOARD_SIZE];     // 0 = 已被占据(黑或白), 1 = 空
    Line move_mask[BOARD_SIZE];  // 1 = 有效落子点(邻域), 0 = 无效
    unsigned long long hash;     // Zobrist Hash
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
