#include <stdlib.h>
#include <string.h>
#include "../include/history.h"

void pushState(GameState *game) {
    HistoryNode *node = (HistoryNode *)malloc(sizeof(HistoryNode));
    if (!node) return; // Allocation failed

    // Copy state
    memcpy(node->board, game->board, sizeof(game->board));
    node->currentPlayer = game->currentPlayer;
    node->lastMove = game->lastMove;
    node->moveCount = game->moveCount;
    
    // Push to stack
    node->next = game->historyHead;
    game->historyHead = node;
}

int undoMove(GameState *game) {
    if (!game->historyHead) {
        return 0; // Empty stack
    }

    HistoryNode *node = game->historyHead;
    game->historyHead = node->next;

    // Restore state
    memcpy(game->board, node->board, sizeof(node->board));
    game->currentPlayer = node->currentPlayer;
    game->lastMove = node->lastMove;
    game->moveCount = node->moveCount;

    free(node);
    return 1;
}

void clearHistory(GameState *game) {
    while (game->historyHead) {
        HistoryNode *temp = game->historyHead;
        game->historyHead = temp->next;
        free(temp);
    }
}
