#include <stdio.h>
#include "../include/board.h"
#include "../include/rules.h"
#include "../include/history.h"
#include "../include/bitboard.h"

//初始化棋盘
void initGame(GameState *game, GameMode mode, RuleType rule) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            game->board[i][j] = EMPTY;
        }
    }
    initBitBoard(&game->bitBoard); 
    game->currentPlayer = PLAYER_BLACK;
    game->moveCount = 0;
    game->mode = mode;
    game->ruleType = rule;
    game->historyHead = NULL;
    game->lastMove.row = -1;
    game->lastMove.col = -1;
}

//打印棋盘
void printBoard(const GameState *game) {
    // 显示上一步的落子位置
    if (game->moveCount > 0) {
        printf("last move was: %c%d\n", (char)game->lastMove.col + 'A', BOARD_SIZE - game->lastMove.row);
    }

    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%2d ", BOARD_SIZE - i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] == BLACK) { // 是黑子
                if (j == BOARD_SIZE - 1) {
                    if (i == game->lastMove.row && j == game->lastMove.col)
                        printf("▲\n");
                    else
                        printf("●\n");
                } else {
                    if (i == game->lastMove.row && j == game->lastMove.col)
                        printf("▲─");
                    else
                        printf("●─");
                }
            }
            else if (game->board[i][j] == WHITE) { // 是白子
                if (j == BOARD_SIZE - 1) {
                    if (i == game->lastMove.row && j == game->lastMove.col)
                        printf("△\n");
                    else
                        printf("◎\n");
                } else {
                    if (i == game->lastMove.row && j == game->lastMove.col)
                        printf("△─");
                    else
                        printf("◎─");
                }
            } else if (i == 0) { // 顶部空格
                if (j == 0)                   { printf("┌─");  }
                else if (j == BOARD_SIZE - 1) { printf("┐\n"); }
                else                          { printf("┬─");  }
            } else if (i == BOARD_SIZE - 1) { // 底部空格
                if (j == 0)                   { printf("└─");  }
                else if (j == BOARD_SIZE - 1) { printf("┘\n"); }
                else                          { printf("┴─");  }
            } else { // 中间空格
                if (j == 0)                   { printf("├─");  }
                else if (j == BOARD_SIZE - 1) { printf("┤\n"); }
                else                          { printf("┼─");  }
            }
        }
    }

    printf("  ");
    for (int i = 0; i < BOARD_SIZE; i++)
        printf(" %c", 'A' + i);
    printf("\n");
}

int makeMove(GameState *game, int row, int col) {
    int valid = checkValidMove(game, row, col);
    if (valid != VALID_MOVE) {
        return valid; 
    }

    pushState(game);

    game->board[row][col] = (game->currentPlayer == PLAYER_BLACK) ? BLACK : WHITE;
    
    Line backup_mask[BOARD_SIZE]; // Buffer for BitBoard
    updateBitBoard(&game->bitBoard, row, col, game->currentPlayer, backup_mask); 

    game->lastMove.row = row;
    game->lastMove.col = col;
    game->moveCount++;
    
    game->currentPlayer = (game->currentPlayer == PLAYER_BLACK) ? PLAYER_WHITE : PLAYER_BLACK;

    return VALID_MOVE;
}

int isBoardFull(const GameState *game) {
    return game->moveCount >= BOARD_SIZE * BOARD_SIZE;
}
