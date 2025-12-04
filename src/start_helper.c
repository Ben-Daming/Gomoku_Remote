#include "../include/start_helper.h"
#include "../include/board.h"
#include "../include/rules.h"
#include "../include/history.h"
#include <stdio.h>

// Ported and adapted from hardcode_beginning.c
// This module implements some opening josekis and tries to place an AI move
// during first few rounds if a known opening is detected.

static int record_position(GameState *game, int round, Player player_turn, int r, int c) {
    // Only record if it's the player's turn; check if we can play here
    int valid = checkValidMove(game, r, c);
    if (valid != VALID_MOVE) return 0;
    int res = makeMove(game, r, c);
    return (res == VALID_MOVE) ? 1 : 0;
}

int start_helper(GameState *game) {
    int round = game->moveCount + 1; // 1-based
    Player player_turn = game->currentPlayer;

    // Helper shortcuts
    CellState (*inner)[BOARD_SIZE] = (CellState (*)[BOARD_SIZE])game->board;
    int row_now_global = game->lastMove.row;
    int col_now_global = game->lastMove.col;

    if (round == 1) {
        return record_position(game, round, player_turn, 7, 7);
    }
    else if (round == 2)
    {
        if (inner[7][7] == BLACK)
            return record_position(game, round, player_turn, 6, 7);

        else if (row_now_global < 3 && col_now_global > 3 && col_now_global < 11)
            return record_position(game, round, player_turn, row_now_global + 2, col_now_global);
        else if (row_now_global > 11 && col_now_global > 3 && col_now_global < 11)
            return record_position(game, round, player_turn, row_now_global - 2, col_now_global);
        else if (col_now_global < 3 && row_now_global > 3 && row_now_global < 11)
            return record_position(game, round, player_turn, row_now_global, col_now_global + 2);
        else if (col_now_global > 11 && row_now_global > 3 && row_now_global < 11)
            return record_position(game, round, player_turn, row_now_global, col_now_global - 2);

        else if (row_now_global <= 3 && col_now_global <= 3)
            return record_position(game, round, player_turn, row_now_global + 2, col_now_global + 2);
        else if (row_now_global <= 3 && col_now_global >= 11)
            return record_position(game, round, player_turn, row_now_global + 2, col_now_global - 2);
        else if (row_now_global >= 11 && col_now_global <= 3)
            return record_position(game, round, player_turn, row_now_global - 2, col_now_global + 2);
        else if (row_now_global >= 11 && col_now_global >= 11)
            return record_position(game, round, player_turn, row_now_global - 2, col_now_global - 2);
        else
            return 0;
    }
    else if (round == 3)
    {
        if (inner[7][7] == BLACK)
        {
            if (inner[0][0] == WHITE || inner[0][14] == WHITE || inner[14][0] == WHITE || inner[14][14] == WHITE)
                return record_position(game, round, player_turn, 7, 8);

            else if (inner[6][7] == WHITE || inner[7][8] == WHITE)
                return record_position(game, round, player_turn, 6, 8);
            else if (inner[7][6] == WHITE || inner[8][7] == WHITE)
                return record_position(game, round, player_turn, 8, 6);

            else if (inner[6][6] == WHITE || inner[8][8] == WHITE)
                return record_position(game, round, player_turn, 8, 6);
            else if (inner[8][6] == WHITE || inner[6][8] == WHITE)
                return record_position(game, round, player_turn, 6, 6);

            else if (inner[5][7] == WHITE)
                return record_position(game, round, player_turn, 8, 7);
            else if (inner[9][7] == WHITE)
                return record_position(game, round, player_turn, 6, 7);
            else if (inner[7][5] == WHITE)
                return record_position(game, round, player_turn, 7, 8);
            else if (inner[7][9] == WHITE)
                return record_position(game, round, player_turn, 7, 6);

            else if (inner[5][5] == WHITE || inner[9][9] == WHITE)
                return record_position(game, round, player_turn, 6, 8);
            else if (inner[5][9] == WHITE || inner[9][5] == WHITE)
                return record_position(game, round, player_turn, 6, 6);
            else
                return 0;
        }
        else
            return 0;
    }
    else if (round == 4)
    {
        if (inner[7][7] == BLACK && inner[6][7] == WHITE)
        {
            if (inner[6][6] == BLACK)
                return record_position(game, round, player_turn, 7, 6);
            else if (inner[6][8] == BLACK)
                return record_position(game, round, player_turn, 7, 8);

            else if (inner[7][6] == BLACK)
                return record_position(game, round, player_turn, 7, 5);
            else if (inner[7][8] == BLACK)
                return record_position(game, round, player_turn, 7, 9);

            else if (inner[8][6] == BLACK)
                return record_position(game, round, player_turn, 6, 8);
            else if (inner[8][8] == BLACK)
                return record_position(game, round, player_turn, 6, 6);

            else if (inner[8][7] == BLACK)
                return record_position(game, round, player_turn, 9, 7);

            else if (inner[9][7] == BLACK)
                return record_position(game, round, player_turn, 5, 6);

            else if (inner[5][7] == BLACK)
                return record_position(game, round, player_turn, 6, 6);

            else if (inner[7][5] == BLACK)
                return record_position(game, round, player_turn, 7, 6);
            else if (inner[7][9] == BLACK)
                return record_position(game, round, player_turn, 7, 8);

            else if (inner[5][5] == BLACK)
                return record_position(game, round, player_turn, 6, 6);
            else if (inner[5][9] == BLACK)
                return record_position(game, round, player_turn, 6, 8);

            else
                return 0;
        }
        else
            return 0;
    }
    else if (round == 5)
    {
        // 花月
        if (inner[7][7] == BLACK && inner[6][7] == WHITE && inner[6][8] == BLACK)
        {
            if (inner[7][8] == WHITE)
                return record_position(game, round, player_turn, 8, 9);
            else if (inner[5][9] == WHITE)
                return record_position(game, round, player_turn, 8, 6);
            else if (inner[8][6] == WHITE)
                return record_position(game, round, player_turn, 5, 9);
            else if (inner[5][6] == WHITE)
                return record_position(game, round, player_turn, 7, 8);
            else
                return 0;
        }
        else if (inner[7][7] == BLACK && inner[7][8] == WHITE && inner[6][8] == BLACK)
        {
            if (inner[6][7] == WHITE)
                return record_position(game, round, player_turn, 8, 9);
            else if (inner[5][9] == WHITE)
                return record_position(game, round, player_turn, 8, 6);
            else if (inner[8][6] == WHITE)
                return record_position(game, round, player_turn, 5, 9);
            else if (inner[8][9] == WHITE)
                return record_position(game, round, player_turn, 6, 7);
            else
                return 0;
        }
        else if (inner[7][7] == BLACK && inner[7][6] == WHITE && inner[8][6] == BLACK)
        {
            if (inner[8][7] == WHITE)
                return record_position(game, round, player_turn, 9, 8);
            else if (inner[9][5] == WHITE)
                return record_position(game, round, player_turn, 6, 8);
            else if (inner[6][8] == WHITE)
                return record_position(game, round, player_turn, 9, 5);
            else if (inner[6][5] == WHITE)
                return record_position(game, round, player_turn, 8, 7);
            else
                return 0;
        }
        else if (inner[7][7] == BLACK && inner[8][7] == WHITE && inner[8][6] == BLACK)
        {
            if (inner[7][6] == WHITE)
                return record_position(game, round, player_turn, 9, 8);
            else if (inner[9][5] == WHITE)
                return record_position(game, round, player_turn, 6, 8);
            else if (inner[6][8] == WHITE)
                return record_position(game, round, player_turn, 9, 5);
            else if (inner[9][8] == WHITE)
                return record_position(game, round, player_turn, 7, 6);
            else
                return 0;
        }

        // 浦月
        else if (inner[7][7] == BLACK && inner[6][6] == WHITE && inner[8][6] == BLACK)
        {
            if (inner[6][8] == WHITE)
                return record_position(game, round, player_turn, 8, 5);
            else if (inner[9][5] == WHITE || inner[8][7] == WHITE || inner[9][7] == WHITE || inner[9][6] == WHITE || inner[7][5] == WHITE || inner[7][6] == WHITE)
                return record_position(game, round, player_turn, 6, 8);
            else if (inner[8][5] == WHITE)
                return record_position(game, round, player_turn, 9, 5);
            else
                return 0;
        }
        else if (inner[7][7] == BLACK && inner[8][8] == WHITE && inner[8][6] == BLACK)
        {
            if (inner[6][8] == WHITE)
                return record_position(game, round, player_turn, 9, 6);
            else if (inner[9][5] == WHITE || inner[8][7] == WHITE || inner[9][7] == WHITE || inner[8][5] == WHITE || inner[7][5] == WHITE || inner[7][6] == WHITE)
                return record_position(game, round, player_turn, 6, 8);
            else if (inner[9][6] == WHITE)
                return record_position(game, round, player_turn, 9, 5);
            else
                return 0;
        }
        else if (inner[7][7] == BLACK && inner[8][6] == WHITE && inner[6][6] == BLACK)
        {
            if (inner[8][8] == WHITE)
                return record_position(game, round, player_turn, 6, 5);
            else if (inner[5][5] == WHITE || inner[6][7] == WHITE || inner[5][7] == WHITE || inner[5][6] == WHITE || inner[7][5] == WHITE || inner[7][6] == WHITE)
                return record_position(game, round, player_turn, 8, 8);
            else if (inner[6][5] == WHITE)
                return record_position(game, round, player_turn, 5, 5);
            else
                return 0;
        }
        else if (inner[7][7] == BLACK && inner[6][8] == WHITE && inner[6][6] == BLACK)
        {
            if (inner[8][8] == WHITE)
                return record_position(game, round, player_turn, 5, 6);
            else if (inner[5][5] == WHITE || inner[6][7] == WHITE || inner[5][7] == WHITE || inner[6][5] == WHITE || inner[7][5] == WHITE || inner[7][6] == WHITE)
                return record_position(game, round, player_turn, 8, 8);
            else if (inner[5][6] == WHITE)
                return record_position(game, round, player_turn, 5, 5);
            else
                return 0;
        }
        else
            return 0;
    }
    else
        return 0;

    return 0; // fallback
}
