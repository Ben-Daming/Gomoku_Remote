#include <stdlib.h>
#include <string.h>
#include "../include/history.h"

//当前状态进栈
void pushState(GameState *game) {
    HistoryNode *node = (HistoryNode *)malloc(sizeof(HistoryNode));
    if (!node) return; 

    // Copy
    memcpy(node->board, game->board, sizeof(game->board));
    node->bitBoard = game->bitBoard; 
    node->currentPlayer = game->currentPlayer;
    node->lastMove = game->lastMove;
    node->moveCount = game->moveCount;
    
    // Push
    node->next = game->historyHead;
    game->historyHead = node;
}

//悔棋出栈
int undoMove(GameState *game) {
    if (!game->historyHead) {
        return 0; // Empty
    }

    HistoryNode *node = game->historyHead;
    game->historyHead = node->next;

    // Restore state
    memcpy(game->board, node->board, sizeof(node->board));
    game->bitBoard = node->bitBoard;
    game->currentPlayer = node->currentPlayer;
    game->lastMove = node->lastMove;
    game->moveCount = node->moveCount;

    free(node);
    return 1;
}

//清空历史记录
void clearHistory(GameState *game) {
    while (game->historyHead) {
        HistoryNode *temp = game->historyHead;
        game->historyHead = temp->next;
        free(temp);
    }
}
