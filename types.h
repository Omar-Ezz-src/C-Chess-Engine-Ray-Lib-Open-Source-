#pragma once
#include "raylib.h"
#include <cstdint>

#define NOT_A_FILE  0xFEFEFEFEFEFEFEFEULL
#define NOT_H_FILE  0x7F7F7F7F7F7F7F7FULL

enum Piece : uint8_t { R, N, B, Q, K, P, r, n, b, q, k, p, None };

struct Move {
    int from = -1, to = -1;
    Piece piece = None;
    Piece captured = None;
    Piece promotedTo = None;
    bool wasCastleWK = false;
    bool wasCastleWQ = false;
    bool wasCastleBK = false;
    bool wasCastleBQ = false;
};

struct Board {
    uint64_t bitboards[12] = { 0 };
    bool sideToMove = true;

    bool castleWK = false;
    bool castleWQ = false;
    bool castleBK = false;
    bool castleBQ = false;

    uint64_t whitePieces() const {
        return bitboards[P] | bitboards[N] | bitboards[B]
            | bitboards[R] | bitboards[Q] | bitboards[K];
    }
    uint64_t blackPieces() const {
        return bitboards[p] | bitboards[n] | bitboards[b]
            | bitboards[r] | bitboards[q] | bitboards[k];
    }
    uint64_t occupied() const { return whitePieces() | blackPieces(); }

    Vector2 highlightedSquare = { 0, 0 };
    Piece   heldPieceType = None;
    int     heldSquare = -1;
};

// Bit helpers
inline uint64_t squareBit(int sq) { return 1ULL << sq; }
inline int square(int file, int rank) { return rank * 8 + file; }
inline int bitSquare(uint64_t bit) {
    static const int table[64] = {
         0,  1, 56,  2, 57, 49, 28,  3, 61, 58, 42, 50, 38, 29, 17,  4,
        62, 47, 59, 36, 45, 43, 51, 22, 53, 39, 33, 30, 24, 18, 12,  5,
        63, 55, 48, 27, 60, 41, 37, 16, 46, 35, 44, 21, 52, 32, 23, 11,
        54, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19,  9, 13,  8,  7,  6
    };
    return table[(bit * 0x03f79d71b4ca8b09ULL) >> 58];
}