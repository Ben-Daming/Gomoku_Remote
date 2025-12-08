#include <stdio.h>
#include "../include/board.h"
#include "../include/rules.h"
#include "../include/history.h"
#include "../include/bitboard.h"

#include "../include/evaluate.h"
#include "../include/ascii_art.h"

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

    // 选择合适的ASCII艺术
    int face_flag = getAsciiFaceFlag();
    const char **art_arr = ANGEL_COMMON;
    if (face_flag == 1) {
        art_arr = ANGEL_SMILE;
    } else if (face_flag == -1) {
        art_arr = ANGEL_AFRAID;
    }
    int art_lines = ANGEL_LINES;
    for (int i = 0; i < BOARD_SIZE; i++) {
        char rowBuf[256];
        int pos = 0;
        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "%2d ", BOARD_SIZE - i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] == BLACK) { // 是黑子
                if (j == BOARD_SIZE - 1) {
                    if (i == game->lastMove.row && j == game->lastMove.col)
                        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "▲");
                    else
                        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "●");
                } else {
                    if (i == game->lastMove.row && j == game->lastMove.col)
                        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "▲─");
                    else
                        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "●─");
                }
            }
            else if (game->board[i][j] == WHITE) { // 是白子
                if (j == BOARD_SIZE - 1) {
                    if (i == game->lastMove.row && j == game->lastMove.col)
                        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "△");
                    else
                        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "◎");
                } else {
                    if (i == game->lastMove.row && j == game->lastMove.col)
                        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "△─");
                    else
                        pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "◎─");
                }
            } else if (i == 0) { // 顶部空格
                if (j == 0)                   { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "┌─");  }
                else if (j == BOARD_SIZE - 1) { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "┐"); }
                else                          { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "┬─");  }
            } else if (i == BOARD_SIZE - 1) { // 底部空格
                if (j == 0)                   { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "└─");  }
                else if (j == BOARD_SIZE - 1) { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "┘"); }
                else                          { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "┴─");  }
            } else { // 中间空格
                if (j == 0)                   { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "├─");  }
                else if (j == BOARD_SIZE - 1) { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "┤"); }
                else                          { pos += snprintf(rowBuf + pos, sizeof(rowBuf) - pos, "┼─");  }
            }
        }
        // 打印当前行和对应的ASCII艺术
        printf("%s    %s\n", rowBuf, art_arr[i]);
    }

    printf("  ");
    for (int i = 0; i < BOARD_SIZE; i++)
        printf(" %c", 'A' + i);
    printf("\n");

    // 如果艺术数组的行数多于棋盘，打印它们并右对齐
    if (art_lines > BOARD_SIZE) {
        for (int k = BOARD_SIZE; k < art_lines; ++k) {
            printf("   ");
            for (int z = 0; z < BOARD_SIZE; ++z) printf("   ");
            printf("    %s\n", art_arr[k]);
        }
    }
    //调试日志
    // printf("\n--- BitBoard Debug Info (Cols) ---\n");
    // printf("Row | Black           | White           | Occupy          | Move Mask\n");
    // printf("----|-----------------|-----------------|-----------------|-----------------\n");
    // for (int i = 0; i < BOARD_SIZE; i++) {
    //     printf("%2d  | ", i);
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.black.cols[i] >> j) & 1);
    //     printf(" | ");
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.white.cols[i] >> j) & 1);
    //     printf(" | ");
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.occupy[i] >> j) & 1);
    //     printf(" | ");
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.move_mask[i] >> j) & 1);
    //     printf("\n");
    // }

    // printf("\n--- Rows (Horizontal) ---\n");
    // printf("Idx | Black           | White\n");
    // for (int i = 0; i < BOARD_SIZE; i++) {
    //     printf("%2d  | ", i);
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.black.rows[i] >> j) & 1);
    //     printf(" | ");
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.white.rows[i] >> j) & 1);
    //     printf("\n");
    // }

    // printf("\n--- Diag1 (Main \\) ---\n");
    // printf("Idx | Black           | White\n");
    // for (int i = 0; i < BOARD_SIZE * 2; i++) {
    //     printf("%2d  | ", i);
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.black.diag1[i] >> j) & 1);
    //     printf(" | ");
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.white.diag1[i] >> j) & 1);
    //     printf("\n");
    // }

    // printf("\n--- Diag2 (Anti /) ---\n");
    // printf("Idx | Black           | White\n");
    // for (int i = 0; i < BOARD_SIZE * 2; i++) {
    //     printf("%2d  | ", i);
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.black.diag2[i] >> j) & 1);
    //     printf(" | ");
    //     for (int j = 0; j < BOARD_SIZE; j++) printf("%d", (game->bitBoard.white.diag2[i] >> j) & 1);
    //     printf("\n");
    // }

}

int makeMove(GameState *game, int row, int col) {
    int valid = checkValidMove(game, row, col);
    if (valid != VALID_MOVE) {
        return valid; 
    }

    pushState(game);

    game->board[row][col] = (game->currentPlayer == PLAYER_BLACK) ? BLACK : WHITE;
    
    Line backup_mask[BOARD_SIZE]; // 用于备份掩码
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
