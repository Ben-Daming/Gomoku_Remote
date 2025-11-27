#include "../include/rules.h"
#include <stdio.h>

// 方向：水平，垂直，对角线 \, 对角线 /
static int dr[] = {0, 1, 1, 1};
static int dc[] = {1, 0, 1, -1};

// 辅助函数：判断位置是否在棋盘里
static int isValidPos(int r, int c) {
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

// 辅助函数：统计某一方向上连续的棋子数
static int countLine(const GameState *game, int r, int c, int dirIdx, Player p) {
    int count = 1;
    // Forward
    for (int i = 1; i < 6; i++) {
        int nr = r + dr[dirIdx] * i;
        int nc = c + dc[dirIdx] * i;
        if (isValidPos(nr, nc) && game->board[nr][nc] == (CellState)p) count++;
        else break;
    }
    // Backward
    for (int i = 1; i < 6; i++) {
        int nr = r - dr[dirIdx] * i;
        int nc = c - dc[dirIdx] * i;
        if (isValidPos(nr, nc) && game->board[nr][nc] == (CellState)p) count++;
        else break;
    }
    return count;
}

// 检查胜利条件，返回胜利者 (PLAYER_BLACK/WHITE) 或 0
int checkWin(const GameState *game) {
    if (game->moveCount == 0) return 0;
    
    int r = game->lastMove.row;
    int c = game->lastMove.col;
    CellState cell = game->board[r][c];
    if (cell == EMPTY) return 0;
    Player p = (cell == BLACK) ? PLAYER_BLACK : PLAYER_WHITE;

    for (int i = 0; i < 4; i++) {
        int count = countLine(game, r, c, i, p);
        if (p == PLAYER_WHITE) {
            if (count >= 5) return p;
        } else {
            // Black
            if (game->ruleType == RULE_STANDARD) {
                if (count == 5) return p;
                // Overline (>5) is not a win for Black in Standard, it's forbidden.
                // But if we are here, the move was already made.
                // If checkValidMove works, this shouldn't happen.
            } else {
                if (count >= 5) return p;
            }
        }
    }
    return 0;
}

// Simplified forbidden check for now. 
// Implementing full Renju 3-3 and 4-4 detection is complex and requires analyzing open ends.
// I will implement Overline check and a placeholder for 3-3/4-4.
// TODO: Implement full 3-3 and 4-4 detection.

static int isOverline(const GameState *game, int r, int c) {
    for (int i = 0; i < 4; i++) {
        if (countLine(game, r, c, i, PLAYER_BLACK) > 5) return 1;
    }
    return 0;
}

int isForbidden(const GameState *game, int row, int col) {
    if (game->ruleType != RULE_STANDARD) return 0;
    // Only Black has forbidden moves
    if (game->currentPlayer != PLAYER_BLACK) return 0;

    // Temporarily place the stone to check patterns
    // Note: game is const, so we can't modify it. 
    // We need to simulate. But countLine takes game.
    // We can cast away const or pass a modified board.
    // Let's cast away const for a moment or use a local copy? 
    // Local copy of board is big.
    // Better: modify countLine to accept a "virtual" stone.
    // Or just cast away const, modify, check, restore.
    
    GameState *mutableGame = (GameState *)game;
    CellState original = mutableGame->board[row][col];
    mutableGame->board[row][col] = BLACK;

    int forbidden = 0;

    // 1. Overline
    if (isOverline(mutableGame, row, col)) {
        forbidden = ERR_FORBIDDEN_OVERLINE;
    }
    
    // 2. 3-3 and 4-4 (Placeholder)
    // Real implementation requires checking open ends.
    
    mutableGame->board[row][col] = original; // Restore
    return forbidden;
}

int checkValidMove(const GameState *game, int row, int col) {
    if (!isValidPos(row, col)) return ERR_OUT_OF_BOUNDS;
    if (game->board[row][col] != EMPTY) return ERR_OCCUPIED;

    if (game->currentPlayer == PLAYER_BLACK && game->ruleType == RULE_STANDARD) {
        int forbidden = isForbidden(game, row, col);
        if (forbidden) return forbidden;
    }

    return VALID_MOVE;
}
