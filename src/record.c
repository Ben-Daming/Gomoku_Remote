#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../include/record.h"
#include "../include/board.h"
#include "../include/history.h"

// Helper to get time string
static struct tm *get_time_tm() {
    time_t time_string;
    time(&time_string);
    return localtime(&time_string);
}

// Helper to print board to file
static void fprint_board(const GameState *game, FILE *fp) {
    fprintf(fp, "\n  ");
    for (int i = 0; i < BOARD_SIZE; i++)
        fprintf(fp, " %c", 'A' + i);
    fprintf(fp, "\n");

    for (int i = 0; i < BOARD_SIZE; i++) {
        fprintf(fp, "%2d ", BOARD_SIZE - i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (game->board[i][j] == BLACK) fprintf(fp, "X ");
            else if (game->board[i][j] == WHITE) fprintf(fp, "O ");
            else fprintf(fp, "+ ");
        }
        fprintf(fp, "%2d\n", BOARD_SIZE - i);
    }
    fprintf(fp, "  ");
    for (int i = 0; i < BOARD_SIZE; i++)
        fprintf(fp, " %c", 'A' + i);
    fprintf(fp, "\n");
}

int record(GameState* game) {
    char curtime[30];
    char file_name[100];
    struct tm *pcurtime;
    FILE *fp;
    
    // Ensure directory exists
    struct stat st = {0};
    if (stat("./game_records", &st) == -1) {
        #ifdef _WIN32
            mkdir("./game_records");
        #else
            mkdir("./game_records", 0700);
        #endif
    }

    pcurtime = get_time_tm();
    sprintf(curtime, "%04d-%02d-%02d_%02d_%02d_%02d",
            (pcurtime->tm_year + 1900) % 10000, pcurtime->tm_mon % 100 + 1,
            pcurtime->tm_mday % 100, pcurtime->tm_hour % 100, pcurtime->tm_min % 100, pcurtime->tm_sec % 100);

    sprintf(file_name, "./game_records/%s.txt", curtime);

    fp = fopen(file_name, "w");
    if (fp == NULL) {
        perror("Failed to open file for recording");
        return 0;
    }

    // Header
    fprintf(fp, "file name: %s\n", curtime);
    if (game->ruleType == RULE_STANDARD)
        fprintf(fp, "ruletype: Standard\n");
    else if(game->ruleType == RULE_NO_FORBIDDEN)
        fprintf(fp, "ruletype: NaN\n");
    if (game->mode == MODE_PVP)
        fprintf(fp, "gamemode: PVP\n");
    else
        fprintf(fp, "gamemode: PVE\n");

    fprintf(fp, "round: %-3d          winner: unknown\n", game->moveCount);
    fprintf(fp, "\n");

    // Collect moves from history
    // Max moves = 225.
    Position moves[BOARD_SIZE * BOARD_SIZE];
    int count = 0;
    
    if (game->moveCount > 0) {
        moves[count++] = game->lastMove;
        HistoryNode* curr = game->historyHead;
        while (curr) {
            if (curr->moveCount > 0) {
                moves[count++] = curr->lastMove;
            }
            curr = curr->next;
        }
    }
    
    // Print in reverse order (from count-1 down to 0)
    if (count < 100) {
        for (int i = count - 1; i >= 0; i--) {
            int step = count - i;
            Position p = moves[i];
            fprintf(fp, "%sstep %2d: %c%-2d%s",
                    (step % 2) ? "" : "\t\t",
                    step,
                    'A' + p.col,
                    BOARD_SIZE - p.row,
                    (step % 2) ? "" : "\n");
        }
    } else {
        for (int i = count - 1; i >= 0; i--) {
            int step = count - i;
            Position p = moves[i];
            fprintf(fp, "%sstep %3d: %c%-2d%s",
                    (step % 2) ? "" : "\t\t",
                    step,
                    'A' + p.col,
                    BOARD_SIZE - p.row,
                    (step % 2) ? "" : "\n");
        }
    }
    
    if (count % 2)
        fprintf(fp, "\n");
        
    fprintf(fp, "\nfinal chessboard:\n");
    fprint_board(game, fp);

    fclose(fp);
    printf("Game recorded to %s\n", file_name);
    return 1;
}

int load(GameState* game, char* filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Could not open file %s\n", filename);
        return 0;
    }
    
    char line[512];
    GameMode mode = MODE_PVE; 
    RuleType rule = RULE_STANDARD;
    // Parse header for gamemode
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "gamemode: PVP")) {
            mode = MODE_PVP;
        } else if (strstr(line, "gamemode: PVE")) {
            mode = MODE_PVE;
        } else if (strstr(line, "ruletype: Standard")){
            rule = RULE_STANDARD;
        } else if (strstr(line, "ruletype: NaN")){
            rule = RULE_NO_FORBIDDEN;
        }
        if (strstr(line, "step 1:")) break; 
    }
    
    initGame(game, mode, rule); 
    
    rewind(fp);
    char word[100];
    while (fscanf(fp, "%s", word) == 1) {
        if (strcmp(word, "step") == 0) {
            int stepNum;
            char moveStr[10];
            
            if (fscanf(fp, "%d:", &stepNum) != 1) {
                continue; 
            }
            
            if (fscanf(fp, "%s", moveStr) == 1) {
                char colChar = moveStr[0];
                int rowVal = atoi(moveStr + 1);
                
                int col = colChar - 'A';
                int row = BOARD_SIZE - rowVal;
                
                if (col >= 0 && col < BOARD_SIZE && row >= 0 && row < BOARD_SIZE) {
                    makeMove(game, row, col);
                }
            }
        }
    }
    
    fclose(fp);
    return 1;
}
