// evaluate.cpp
#include "evaluate.h"
#include "types.h"
#include "board.h"

static const int PST_PAWN[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10,-20,-20, 10, 10,  5,
     5, -5,-10,  0,  0,-10, -5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
     0,  0,  0,  0,  0,  0,  0,  0
};
static const int PST_KNIGHT[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
static const int PST_BISHOP[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
static const int PST_ROOK[64] = {
     0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     5, 10, 10, 10, 10, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
static const int PST_QUEEN[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
      0,  0,  5,  5,  5,  5,  0, -5,
     -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};
static const int PST_KING_MID[64] = {
     20, 30, 10,  0,  0, 10, 30, 20,
     20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
};
static const int PST_KING_END[64] = {
    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -50,-40,-30,-20,-20,-30,-40,-50
};

int pieceValues[] = { 500, 300, 320, 900, 0, 100, 500, 300, 320, 900, 0, 100, 0 };

static const int* PST_TABLE[12] = {
    PST_ROOK,   // R
    PST_KNIGHT, // N
    PST_BISHOP, // B
    PST_QUEEN,  // Q
    nullptr,    // K
    PST_PAWN,   // P
    PST_ROOK,   // r
    PST_KNIGHT, // n
    PST_BISHOP, // b
    PST_QUEEN,  // q
    nullptr,    // k
    PST_PAWN,   // p
};

static int scorePST(uint64_t bb, const int* pst, bool isBlack) {
    int score = 0;
    while (bb) {
        int sq = bitSquare(bb & (0ULL - bb));
        score += pst[isBlack ? (sq ^ 56) : sq];
        bb &= bb - 1;
    }
    return score;
}

static int getGamePhase(Board* board) {
    int phase = 24;
    phase -= popcount64(board->bitboards[N] | board->bitboards[n]) * 1;
    phase -= popcount64(board->bitboards[B] | board->bitboards[b]) * 1;
    phase -= popcount64(board->bitboards[R] | board->bitboards[r]) * 2;
    phase -= popcount64(board->bitboards[Q] | board->bitboards[q]) * 4;
    if (phase < 0) phase = 0;
    return (phase * 256 + 12) / 24;
}

int evaluate(Board* board) {
    int score = 0;
    int endgamePhase = getGamePhase(board);

    for (int i = 0; i < 12; i++) {
        if (i == K || i == k) continue;
        bool isBlack = (i >= 6);
        int  sign = isBlack ? -1 : 1;
        int  count = popcount64(board->bitboards[i]);
        score += sign * count * pieceValues[i];
        if (PST_TABLE[i])
            score += sign * scorePST(board->bitboards[i], PST_TABLE[i], isBlack);
    }

    // Tapered king PST
    int wKingMid = scorePST(board->bitboards[K], PST_KING_MID, false);
    int wKingEnd = scorePST(board->bitboards[K], PST_KING_END, false);
    score += ((wKingMid * (256 - endgamePhase)) + (wKingEnd * endgamePhase)) / 256;

    int bKingMid = scorePST(board->bitboards[k], PST_KING_MID, true);
    int bKingEnd = scorePST(board->bitboards[k], PST_KING_END, true);
    score -= ((bKingMid * (256 - endgamePhase)) + (bKingEnd * endgamePhase)) / 256;

    // Bishop pair bonus
    if (popcount64(board->bitboards[B]) >= 2) score += 30;
    if (popcount64(board->bitboards[b]) >= 2) score -= 30;

    // Doubled & isolated pawn penalties
    static const uint64_t FILE_MASK[8] = {
        0x0101010101010101ULL,      0x0101010101010101ULL << 1,
        0x0101010101010101ULL << 2, 0x0101010101010101ULL << 3,
        0x0101010101010101ULL << 4, 0x0101010101010101ULL << 5,
        0x0101010101010101ULL << 6, 0x0101010101010101ULL << 7,
    };
    static const uint64_t ADJ_FILE[8] = {
        FILE_MASK[1],
        FILE_MASK[0] | FILE_MASK[2],
        FILE_MASK[1] | FILE_MASK[3],
        FILE_MASK[2] | FILE_MASK[4],
        FILE_MASK[3] | FILE_MASK[5],
        FILE_MASK[4] | FILE_MASK[6],
        FILE_MASK[5] | FILE_MASK[7],
        FILE_MASK[6],
    };

    uint64_t wP = board->bitboards[P];
    uint64_t bP = board->bitboards[p];

    for (int f = 0; f < 8; f++) {
        int wCount = popcount64(wP & FILE_MASK[f]);
        int bCount = popcount64(bP & FILE_MASK[f]);
        if (wCount > 1) score -= (wCount - 1) * 20;
        if (bCount > 1) score += (bCount - 1) * 20;
    }

    uint64_t wp = wP;
    while (wp) {
        int sq = bitSquare(wp & (0ULL - wp));
        if (!(wP & ADJ_FILE[sq % 8])) score -= 15;
        wp &= wp - 1;
    }
    uint64_t bp = bP;
    while (bp) {
        int sq = bitSquare(bp & (0ULL - bp));
        if (!(bP & ADJ_FILE[sq % 8])) score += 15;
        bp &= bp - 1;
    }

    return board->sideToMove? score : -score;
}