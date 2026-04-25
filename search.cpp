// search.cpp
#include "search.h"
#include "movegen.h"
#include "board.h"
#include "evaluate.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <climits>

// ─── Transposition Table ──────────────────────────────────────────────────────

enum TTFlag : uint8_t { TT_EXACT, TT_LOWER, TT_UPPER };

struct TTEntry {
    uint64_t hash = 0;
    int      score = 0;
    int      depth = 0;
    TTFlag   flag = TT_EXACT;
    Move     best = {};
};

static const int TT_SIZE = 1 << 20; // ~1M entries
static TTEntry transpositionTable[TT_SIZE];

// ─── Zobrist Hashing ──────────────────────────────────────────────────────────

static uint64_t zobristTable[12][64];
static uint64_t zobristSide;
static bool zobristInit = false;

static uint64_t randU64() {
    // xorshift64
    static uint64_t s = 0x123456789ABCDEFULL;
    s ^= s << 13; s ^= s >> 7; s ^= s << 17;
    return s;
}

void initZobrist() {
    if (zobristInit) return;
    for (int p = 0; p < 12; p++)
        for (int sq = 0; sq < 64; sq++)
            zobristTable[p][sq] = randU64();
    zobristSide = randU64();
    zobristInit = true;
}

uint64_t computeHash(Board* board) {
    uint64_t h = 0;
    for (int p = 0; p < 12; p++) {
        uint64_t bb = board->bitboards[p];
        while (bb) {
            int sq = bitSquare(bb & (0ULL - bb));
            h ^= zobristTable[p][sq];
            bb &= bb - 1;
        }
    }
    if (!board->sideToMove) h ^= zobristSide;
    return h;
}

// ─── Move Generation (all legal moves for search) ────────────────────────────

static std::vector<Move> generateLegalMoves(Board* board) {
    std::vector<Move> moves;
    moves.reserve(64);

    bool isWhite = board->sideToMove;
    int  start = isWhite ? 0 : 6;
    int  end = isWhite ? 6 : 12;

    for (int i = start; i < end; i++) {
        uint64_t bb = board->bitboards[i];
        while (bb) {
            int fromSq = bitSquare(bb & (0ULL - bb));
            uint64_t legal = getLegalMoves(static_cast<Piece>(i), fromSq, board, isWhite);
            while (legal) {
                int toSq = bitSquare(legal & (0ULL - legal));
                moves.push_back(buildMove(board, static_cast<Piece>(i), fromSq, toSq));
                legal &= legal - 1;
            }
            bb &= bb - 1;
        }
    }
    return moves;
}

// ─── Move Ordering ────────────────────────────────────────────────────────────

// Killer moves: 2 quiet moves per ply that caused a beta cutoff
static Move killerMoves[MAX_DEPTH][2];

// History heuristic: [piece][toSq] bonus for quiet moves
static int historyTable[12][64];

static void clearHeuristics() {
    for (int d = 0; d < MAX_DEPTH; d++)
        killerMoves[d][0] = killerMoves[d][1] = Move{};
    for (int p = 0; p < 12; p++)
        for (int s = 0; s < 64; s++)
            historyTable[p][s] = 0;
}

static void storeKiller(int ply, const Move& m) {
    if (ply >= MAX_DEPTH) return;
    if (m.from == killerMoves[ply][0].from && m.to == killerMoves[ply][0].to) return;
    killerMoves[ply][1] = killerMoves[ply][0];
    killerMoves[ply][0] = m;
}

static bool isKiller(int ply, const Move& m) {
    if (ply >= MAX_DEPTH) return false;
    return (m.from == killerMoves[ply][0].from && m.to == killerMoves[ply][0].to)
        || (m.from == killerMoves[ply][1].from && m.to == killerMoves[ply][1].to);
}

// MVV-LVA table [attacker][victim]
static const int MVV_LVA[12][12] = {
    //       R    N    B    Q    K    P    r    n    b    q    k    p
    /* R */ {54, 52,  53,  51,  50,  55,  54,  52,  53,  51,  50,  55},
    /* N */ {44, 42,  43,  41,  40,  45,  44,  42,  43,  41,  40,  45},
    /* B */ {44, 42,  43,  41,  40,  45,  44,  42,  43,  41,  40,  45},
    /* Q */ {34, 32,  33,  31,  30,  35,  34,  32,  33,  31,  30,  35},
    /* K */ {64, 62,  63,  61,  60,  65,  64,  62,  63,  61,  60,  65},
    /* P */ {14, 12,  13,  11,  10,  15,  14,  12,  13,  11,  10,  15},
    /* r */ {54, 52,  53,  51,  50,  55,  54,  52,  53,  51,  50,  55},
    /* n */ {44, 42,  43,  41,  40,  45,  44,  42,  43,  41,  40,  45},
    /* b */ {44, 42,  43,  41,  40,  45,  44,  42,  43,  41,  40,  45},
    /* q */ {34, 32,  33,  31,  30,  35,  34,  32,  33,  31,  30,  35},
    /* k */ {64, 62,  63,  61,  60,  65,  64,  62,  63,  61,  60,  65},
    /* p */ {14, 12,  13,  11,  10,  15,  14,  12,  13,  11,  10,  15},
};

static int scoreMoveForOrdering(const Move& m, int ply, const Move& ttBest) {
    // 1. TT best move first
    if (m.from == ttBest.from && m.to == ttBest.to && ttBest.from != -1)
        return 1'000'000;

    // 2. Captures scored by MVV-LVA
    if (m.captured != None)
        return 100'000 + MVV_LVA[m.piece][m.captured];

    // 3. Killer moves
    if (isKiller(ply, m))
        return 90'000;

    // 4. History heuristic for quiet moves
    return historyTable[m.piece][m.to];
}

static void sortMoves(std::vector<Move>& moves, int ply, const Move& ttBest) {
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return scoreMoveForOrdering(a, ply, ttBest) >
            scoreMoveForOrdering(b, ply, ttBest);
        });
}

// ─── Quiescence Search ───────────────────────────────────────────────────────

static int quiescence(Board* board, int alpha, int beta) {
    int stand_pat = evaluate(board);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    // Only generate captures
    std::vector<Move> moves = generateLegalMoves(board);
    std::vector<Move> captures;
    captures.reserve(8);
    for (auto& m : moves)
        if (m.captured != None) captures.push_back(m);

    sortMoves(captures, 0, Move{});

    for (auto& m : captures) {
        makeMove(board, m);
        int score = -quiescence(board, -beta, -alpha);
        unmakeMove(board, m);

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

// ─── Negamax with Alpha-Beta + Enhancements ───────────────────────────────────

static SearchResult negamax(Board* board, int depth, int alpha, int beta,
    int ply, uint64_t hash)
{
    // ── TT Lookup ──────────────────────────────────────────────────────────
    TTEntry& tt = transpositionTable[hash % TT_SIZE];
    Move ttBest = {};
    if (tt.hash == hash && tt.depth >= depth) {
        ttBest = tt.best;
        if (tt.flag == TT_EXACT)              return { tt.score, tt.best };
        if (tt.flag == TT_LOWER && tt.score > alpha) alpha = tt.score;
        if (tt.flag == TT_UPPER && tt.score < beta)  beta = tt.score;
        if (alpha >= beta)                    return { tt.score, tt.best };
    }

    // ── Terminal / Leaf ────────────────────────────────────────────────────
    std::vector<Move> moves = generateLegalMoves(board);

    if (moves.empty()) {
        // Checkmate or stalemate
        bool isWhite = board->sideToMove;
        int kSq = bitSquare(board->bitboards[isWhite ? K : k]);
        if (isSquareAttacked(kSq, !isWhite, board))
            return { -MATE_SCORE + ply, Move{} }; // prefer faster mates
        return { 0, Move{} }; // stalemate
    }

    if (depth == 0)
        return { quiescence(board, alpha, beta), Move{} };

    // ── Null Move Pruning ──────────────────────────────────────────────────
    // Skip in endgame (few pieces) to avoid zugzwang errors
    bool isWhite = board->sideToMove;
    int nonPawnMat = 0;
    for (int i : {R, N, B, Q, r, n, b, q})
        nonPawnMat += popcount64(board->bitboards[i]);

    if (depth >= 3 && nonPawnMat > 4 && ply > 0) {
        board->sideToMove = !board->sideToMove;
        uint64_t nullHash = hash ^ zobristSide;
        int nullScore = -negamax(board, depth - 1 - NULL_MOVE_R, -beta, -beta + 1,
            ply + 1, nullHash).score;
        board->sideToMove = !board->sideToMove;
        if (nullScore >= beta)
            return { beta, Move{} };
    }

    // ── Main Search ────────────────────────────────────────────────────────
    sortMoves(moves, ply, ttBest);

    int  originalAlpha = alpha;
    Move bestMove = moves[0];
    int  bestScore = INT_MIN;

    for (int i = 0; i < (int)moves.size(); i++) {
        Move& m = moves[i];

        // ── Late Move Reduction (LMR) ──────────────────────────────────────
        // Reduce depth for quiet moves searched late in the list
        int newDepth = depth - 1;
        bool isQuiet = (m.captured == None);
        if (depth >= 3 && i >= 4 && isQuiet && ply > 0)
            newDepth = depth - 2; // reduce by 1 extra

        // ── Compute new hash ───────────────────────────────────────────────
        uint64_t newHash = hash;
        // XOR out piece from origin square
        newHash ^= zobristTable[m.piece][m.from];
        // XOR out captured piece
        if (m.captured != None)
            newHash ^= zobristTable[m.captured][m.to];
        // XOR in piece at destination
        newHash ^= zobristTable[m.piece][m.to];
        // Flip side
        newHash ^= zobristSide;

        makeMove(board, m);
        int score = -negamax(board, newDepth, -beta, -alpha, ply + 1, newHash).score;

        // Re-search at full depth if LMR raised alpha
        if (newDepth < depth - 1 && score > alpha)
            score = -negamax(board, depth - 1, -beta, -alpha, ply + 1, newHash).score;

        unmakeMove(board, m);

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            // Beta cutoff — store killer & history for quiet moves
            if (m.captured == None) {
                storeKiller(ply, m);
                historyTable[m.piece][m.to] += depth * depth;
            }
            break;
        }
    }

    // ── TT Store ──────────────────────────────────────────────────────────
    TTFlag flag;
    if (bestScore <= originalAlpha) flag = TT_UPPER;
    else if (bestScore >= beta)          flag = TT_LOWER;
    else                                 flag = TT_EXACT;

    transpositionTable[hash % TT_SIZE] = { hash, bestScore, depth, flag, bestMove };

    return { bestScore, bestMove };
}

// ─── Iterative Deepening ──────────────────────────────────────────────────────

SearchResult findBestMove(Board* board, int maxDepth) {
    initZobrist();
    clearHeuristics();

    uint64_t hash = computeHash(board);
    SearchResult best{};

    for (int depth = 1; depth <= maxDepth; depth++) {
        SearchResult result = negamax(board, depth, -INF, INF, 0, hash);
        if (result.move.from != -1)
            best = result;

        // Optional: print info like a UCI engine
        std::cout << "depth " << depth
            << "  score " << best.score
            << "  move " << best.move.from << "->" << best.move.to
            << "\n";
    }
    return best;
}