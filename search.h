#pragma once
#include "types.h"

static const int MAX_DEPTH = 64;
static const int INF = 1'000'000;
static const int MATE_SCORE = 900'000;
static const int NULL_MOVE_R = 2;   // null move reduction depth

struct SearchResult {
    int  score = 0;
    Move move = {};
};

void         initZobrist();
uint64_t     computeHash(Board* board);
SearchResult findBestMove(Board* board, int maxDepth);
