#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/types.h"
#include "../include/board.h"
#include "../include/rules.h"
#include "../include/history.h"
#include "../include/ai.h"

void printHelp() {
    printf("Usage: gomoku [options]\n");
    printf("Options:\n");
    printf("  --mode <pvp|pve>      Set game mode (default: pvp)\n");
    printf("  --rules <std|simple>  Set rules (default: std)\n");
}

int parseCoord(const char *str, int *row, int *col) {
    if (!str) return 0;
    int len = strlen(str);
    if (len < 2 || len > 3) return 0;

    char colChar = 0;
    char rowStr[3] = {0};
    int rowVal = 0;

    if (isalpha(str[0])) {
        colChar = toupper(str[0]);
        if (isdigit(str[1])) {
            rowStr[0] = str[1];
            if (len == 3 && isdigit(str[2])) rowStr[1] = str[2];
        } else return 0;
    } else if (isdigit(str[0])) {
        // 15A format?
        // Let's support A15 or 15A
        if (isdigit(str[1])) {
             rowStr[0] = str[0];
             rowStr[1] = str[1];
             if (len == 3 && isalpha(str[2])) colChar = toupper(str[2]);
             else return 0;
        } else {
            rowStr[0] = str[0];
            if (isalpha(str[1])) colChar = toupper(str[1]);
            else return 0;
        }
    } else return 0;

    rowVal = atoi(rowStr);
    if (colChar < 'A' || colChar > 'O') return 0;
    if (rowVal < 1 || rowVal > 15) return 0;

    *col = colChar - 'A';
    *row = BOARD_SIZE - rowVal; // 15 is row 0
    return 1;
}

int main(int argc, char *argv[]) {
    GameMode mode = MODE_PVP;
    RuleType rule = RULE_STANDARD;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "pve") == 0) mode = MODE_PVE;
            else if (strcmp(argv[i+1], "pvp") == 0) mode = MODE_PVP;
            i++;
        } else if (strcmp(argv[i], "--rules") == 0 && i + 1 < argc) {
            if (strcmp(argv[i+1], "simple") == 0) rule = RULE_NO_FORBIDDEN;
            else if (strcmp(argv[i+1], "std") == 0) rule = RULE_STANDARD;
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            printHelp();
            return 0;
        }
    }

    GameState game;
    initGame(&game, mode, rule);

    printf("Gomoku Game Started!\n");
    printf("Mode: %s, Rules: %s\n", mode == MODE_PVP ? "PvP" : "PvE", rule == RULE_STANDARD ? "Standard (Renju-like)" : "Simple");
    printf("Enter moves as 'H8', 'undo' to undo, 'quit' to exit.\n");

    char input[64];
    int running = 1;

    while (running) {
        printBoard(&game);
        
        int winner = checkWin(&game);
        if (winner) {
            printf("Player %s wins!\n", winner == PLAYER_BLACK ? "Black" : "White");
            break;
        }
        if (isBoardFull(&game)) {
            printf("Draw!\n");
            break;
        }

        if (mode == MODE_PVE && game.currentPlayer == PLAYER_WHITE) {
            // AI Turn
            printf("AI is thinking...\n");
            Position aiMove = getAIMove(&game);
            if (aiMove.row == -1) {
                printf("AI cannot move! Draw?\n");
                break;
            }
            makeMove(&game, aiMove.row, aiMove.col);
            continue;
        }

        printf("Player %s's turn > ", game.currentPlayer == PLAYER_BLACK ? "Black" : "White");
        if (!fgets(input, sizeof(input), stdin)) break;
        
        // Trim newline
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "quit") == 0) {
            running = 0;
            break;
        } else if (strcmp(input, "undo") == 0) {
            if (undoMove(&game)) {
                printf("Undo successful.\n");
                // If PvE, undo twice to get back to human turn
                if (mode == MODE_PVE) {
                    undoMove(&game);
                }
            } else {
                printf("Cannot undo.\n");
            }
            continue;
        }

        int r, c;
        if (parseCoord(input, &r, &c)) {
            int result = makeMove(&game, r, c);
            if (result == VALID_MOVE) {
                // Move accepted
            } else {
                printf("Invalid move: ");
                switch (result) {
                    case ERR_OUT_OF_BOUNDS: printf("Out of bounds\n"); break;
                    case ERR_OCCUPIED: printf("Position occupied\n"); break;
                    case ERR_FORBIDDEN_33: printf("Forbidden: Double Three\n"); break;
                    case ERR_FORBIDDEN_44: printf("Forbidden: Double Four\n"); break;
                    case ERR_FORBIDDEN_OVERLINE: printf("Forbidden: Overline\n"); break;
                    default: printf("Unknown error\n");
                }
            }
        } else {
            printf("Invalid input format. Use 'H8' etc.\n");
        }
    }

    clearHistory(&game);
    return 0;
}
