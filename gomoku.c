// A main function to start a gomoku game
// 开启一局五子棋游戏的主函数
// All code here can be modified and extended as needed
// 所有代码可根据需要进行修改和扩展

#include <stdio.h>
#include <stdlib.h>

// define board size
// 定义棋盘大小
#define BOARD_SIZE 15

// enum of board cells
// 棋盘格子的状态枚举
typedef enum {
    EMPTY,
    BLACK,
    WHITE
} CellState;

// enum of players
// 玩家枚举
typedef enum {
    PLAYER_BLACK,
    PLAYER_WHITE
} Player;

// enum of game modes
// 游戏模式枚举
typedef enum {
    MODE_PVP, // Player vs Player
    MODE_PVE  // Player vs AI
} GameMode;

// structure of board position
// 棋盘位置结构体
typedef struct {
    int row;
    int col; // column
} Position;

// structure of the game state can be defined here
// 游戏状态结构体
typedef struct {
    CellState board[BOARD_SIZE][BOARD_SIZE];
    Player currentPlayer; // whose turn it is
    Position lastMove; // where the last move made
} GameState;


// Function to initialize the game state
// 初始化游戏状态的函数
void initializeGame(GameState *gameState) {
    gameState->currentPlayer = PLAYER_BLACK;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            gameState->board[i][j] = EMPTY;
        }
    }
}


// Function to print the board
// Use numbers to represent columns, characters to represent rows
// 打印棋盘的函数。使用数字表示列，字符表示行
void printBoard(GameState *gameState) {
    // Display the last move
    printf("Your last move was: %c%d\n", (char)gameState->lastMove.col + 'A', gameState->lastMove.row + 1);

    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%2d ", BOARD_SIZE - i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (gameState->board[i][j] == BLACK) { // is black stone
                if (j == BOARD_SIZE - 1) {
                    if (i == gameState->lastMove.row && j == gameState->lastMove.col)
                        printf("▲\n");
                    else
                        printf("●\n");
                } else {
                    if (i == gameState->lastMove.row && j == gameState->lastMove.col)
                        printf("▲─");
                    else
                        printf("●─");
                }
            }
            else if (gameState->board[i][j] == WHITE) { // is white stone
                if (j == BOARD_SIZE - 1) {
                    if (i == gameState->lastMove.row && j == gameState->lastMove.col)
                        printf("△\n");
                    else
                        printf("◎\n");
                } else {
                    if (i == gameState->lastMove.row && j == gameState->lastMove.col)
                        printf("△─");
                    else
                        printf("◎─");
                }
            } else if (i == 0) { // top row empty
                if (j == 0)                   { printf("┌─");  }
                else if (j == BOARD_SIZE - 1) { printf("┐\n"); }
                else                          { printf("┬─");  }
            } else if (i == BOARD_SIZE - 1) { // bottom row empty
                if (j == 0)                   { printf("└─");  }
                else if (j == BOARD_SIZE - 1) { printf("┘\n"); }
                else                          { printf("┴─");  }
            } else { // middle empty
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


// Function of win checking. 0: no win, 1: black wins, 2: white wins
// 判断胜利的函数。0: 无胜利，1: 黑方胜利，2: 白方胜利
int checkWin(GameState *gameState, int row, int col) {
    // TODO: Implement win checking logic
    return 0;
}


// Function to check valid move
// 检查合法落子的函数
int isValidMove(GameState *gameState, int row, int col) {
    // TODO: Implement move validation logic
    return 0;
}


// Main function to start the game
// 主函数，开启游戏
int main(int argc, char *argv[]) {
    printf("Welcome to Gomoku!\n");

    // Initialize the game state
    // 初始化游戏状态
    GameState gameState;
    initializeGame(&gameState);
    GameMode gameMode = MODE_PVP;

    // Handle input before starting the game loop
    // 在开始游戏循环前处理输入
    char input[100];

    // Default to Player vs Player
    // 默认选择玩家对玩家模式
    int mode = 1;

    // TODO: Use argc and argv to set up game mode. Default to Player vs Player.
    // 可以选择使用 argc 和 argv 来设置游戏模式，默认玩家对玩家模式。也可以选择其他方式输入。

    if (mode == 1) {
        printf("You selected Player vs Player mode.\n");
    } else {
        printf("You selected Player vs AI mode.\n");
        printf("Who goes first? (1: You, 2: AI): ");
        // TODO: Use stdin to ask who goes first
        // 可以选择使用标准输入来询问谁先手。
    }

    // Start the game loop here
    // 在这里开始游戏主体的循环
    while (1) {
        // TODO: Implement game loop, input handling, move validation, win checking, etc.
        // 实现游戏循环、输入处理、落子合法性检查、胜利判断等功能。
    }

    return 0;
}
