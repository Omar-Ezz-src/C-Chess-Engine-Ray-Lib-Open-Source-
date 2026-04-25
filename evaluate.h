// evaluate.h
#pragma once
#include "types.h"

extern int pieceValues[13];
int evaluate(Board* board);
inline int popcount64(uint64_t bb) {
    int count = 0;
    while (bb) { bb &= bb - 1; count++; }
    return count;
}