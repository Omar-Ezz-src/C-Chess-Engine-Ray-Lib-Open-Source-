#include <iostream>
#include "board.h"
#include "types.h"


const uint64_t RANK_3 = 0x0000000000FF0000ULL;
const uint64_t RANK_6 = 0x0000FF0000000000ULL;


uint64_t getRookAttacks(uint64_t rook, uint64_t occupied, uint64_t friendly) {
    uint64_t result = 0, ray;

    ray = rook;
    while ((ray = (ray << 8))) {
        if (ray & friendly) break;
        result |= ray;
        if (ray & occupied) break;
    }
    ray = rook;
    while ((ray = (ray >> 8))) {
        if (ray & friendly) break;
        result |= ray;
        if (ray & occupied) break;
    }
    ray = rook;
    while ((ray = (ray << 1) & NOT_A_FILE)) {
        if (ray & friendly) break;
        result |= ray;
        if (ray & occupied) break;
    }
    ray = rook;
    while ((ray = (ray >> 1) & NOT_H_FILE)) {
        if (ray & friendly) break;
        result |= ray;
        if (ray & occupied) break;
    }
    return result;
}

uint64_t getValidRookMoves(uint64_t pieceBitboard, Board* board, bool isWhite) {
    uint64_t friendly = isWhite ? board->whitePieces() : board->blackPieces();
    uint64_t occupied = board->occupied();
    uint64_t result = 0;
    while (pieceBitboard) {
        uint64_t single = pieceBitboard & (0ULL - pieceBitboard);
        result |= getRookAttacks(single, occupied, friendly);
        pieceBitboard &= pieceBitboard - 1;
    }
    return result;
}

uint64_t getValidKnightMoves(uint64_t pieceBitboard, Board* board, bool isWhite) {
    uint64_t friendly = isWhite ? board->whitePieces() : board->blackPieces();
    uint64_t notAFile = NOT_A_FILE;
    uint64_t notHFile = NOT_H_FILE;
    uint64_t notABFile = notAFile & (notAFile << 1);
    uint64_t notGHFile = notHFile & (notHFile >> 1);
    uint64_t result = 0;

    result |= (pieceBitboard << 17) & notAFile;
    result |= (pieceBitboard << 15) & notHFile;
    result |= (pieceBitboard << 10) & notABFile;
    result |= (pieceBitboard << 6) & notGHFile;
    result |= (pieceBitboard >> 17) & notHFile;
    result |= (pieceBitboard >> 15) & notAFile;
    result |= (pieceBitboard >> 10) & notGHFile;
    result |= (pieceBitboard >> 6) & notABFile;

    return result & ~friendly;
}

uint64_t getBishopAttacks(uint64_t bishop, uint64_t occupied, uint64_t friendly) {
    uint64_t result = 0, ray;

    ray = bishop;
    while ((ray = (ray << 9) & NOT_A_FILE)) {
        if (ray & friendly) break;
        result |= ray;
        if (ray & occupied) break;
    }
    ray = bishop;
    while ((ray = (ray << 7) & NOT_H_FILE)) {
        if (ray & friendly) break;
        result |= ray;
        if (ray & occupied) break;
    }
    ray = bishop;
    while ((ray = (ray >> 7) & NOT_A_FILE)) {
        if (ray & friendly) break;
        result |= ray;
        if (ray & occupied) break;
    }
    ray = bishop;
    while ((ray = (ray >> 9) & NOT_H_FILE)) {
        if (ray & friendly) break;
        result |= ray;
        if (ray & occupied) break;
    }
    return result;
}

uint64_t getValidBishopMoves(uint64_t pieceBitboard, Board* board, bool isWhite) {
    uint64_t friendly = isWhite ? board->whitePieces() : board->blackPieces();
    uint64_t occupied = board->occupied();
    uint64_t result = 0;
    while (pieceBitboard) {
        uint64_t single = pieceBitboard & (0ULL - pieceBitboard);
        result |= getBishopAttacks(single, occupied, friendly);
        pieceBitboard &= pieceBitboard - 1;
    }
    return result;
}

uint64_t getValidQueenMoves(uint64_t pieceBitboard, Board* board, bool isWhite) {
    return getValidRookMoves(pieceBitboard, board, isWhite)
        | getValidBishopMoves(pieceBitboard, board, isWhite);
}

uint64_t getValidPawnMoves(uint64_t pieceBitboard, Board* board, bool isWhite) {
    uint64_t friendly = isWhite ? board->whitePieces() : board->blackPieces();
    uint64_t against = isWhite ? board->blackPieces() : board->whitePieces();
    uint64_t empty = ~(friendly | against);
    uint64_t result = 0;

    if (isWhite) {
        uint64_t singlePush = (pieceBitboard << 8) & empty;
        uint64_t doublePush = ((singlePush & RANK_3) << 8) & empty;
        
        result |= singlePush | doublePush;
        result |= (pieceBitboard << 9) & NOT_A_FILE & against;
        result |= (pieceBitboard << 7) & NOT_H_FILE & against;
    }
    else {
        uint64_t singlePush = (pieceBitboard >> 8) & empty;
        uint64_t doublePush = ((singlePush & RANK_6) >> 8) & empty;

        result |= singlePush | doublePush;
        result |= (pieceBitboard >> 7) & NOT_A_FILE & against;
        result |= (pieceBitboard >> 9) & NOT_H_FILE & against;
    }
    return result;
}

bool isSquareAttacked(int sq, bool byWhite, Board* board) {
    uint64_t sqBit = squareBit(sq);

    uint64_t knights = byWhite ? board->bitboards[N] : board->bitboards[n];
    if (getValidKnightMoves(sqBit, board, !byWhite) & knights) return true;

    uint64_t bishops = byWhite ? (board->bitboards[B] | board->bitboards[Q])
        : (board->bitboards[b] | board->bitboards[q]);
    if (getValidBishopMoves(sqBit, board, !byWhite) & bishops) return true;

    uint64_t rooks = byWhite ? (board->bitboards[R] | board->bitboards[Q])
        : (board->bitboards[r] | board->bitboards[q]);
    if (getValidRookMoves(sqBit, board, !byWhite) & rooks) return true;

    uint64_t pawns = byWhite ? board->bitboards[P] : board->bitboards[p];
    if (byWhite) {
        if (((sqBit >> 9) & NOT_H_FILE & pawns) || ((sqBit >> 7) & NOT_A_FILE & pawns)) return true;
    }
    else {
        if (((sqBit << 9) & NOT_A_FILE & pawns) || ((sqBit << 7) & NOT_H_FILE & pawns)) return true;
    }

    uint64_t king = byWhite ? board->bitboards[K] : board->bitboards[k];
    uint64_t kingAttacks = 0;
    kingAttacks |= (sqBit << 8);
    kingAttacks |= (sqBit >> 8);
    kingAttacks |= (sqBit << 1) & NOT_A_FILE;
    kingAttacks |= (sqBit >> 1) & NOT_H_FILE;
    kingAttacks |= (sqBit << 9) & NOT_A_FILE;
    kingAttacks |= (sqBit << 7) & NOT_H_FILE;
    kingAttacks |= (sqBit >> 7) & NOT_A_FILE;
    kingAttacks |= (sqBit >> 9) & NOT_H_FILE;
    if (kingAttacks & king) return true;

    return false;
}

uint64_t getValidKingMoves(uint64_t pieceBitboard, Board* board, bool isWhite) {
    uint64_t friendly = isWhite ? board->whitePieces() : board->blackPieces();
    uint64_t occ = board->occupied();
    uint64_t result = 0;

    result |= (pieceBitboard << 8);
    result |= (pieceBitboard >> 8);
    result |= (pieceBitboard << 1) & NOT_A_FILE;
    result |= (pieceBitboard << 9) & NOT_A_FILE;
    result |= (pieceBitboard >> 7) & NOT_A_FILE;
    result |= (pieceBitboard >> 1) & NOT_H_FILE;
    result |= (pieceBitboard << 7) & NOT_H_FILE;
    result |= (pieceBitboard >> 9) & NOT_H_FILE;

    if (isWhite) {
        if (pieceBitboard & squareBit(4) && !isSquareAttacked(4, !isWhite, board)) {
            if (board->castleWK
                && !(occ & squareBit(5)) && !(occ & squareBit(6))
                && !isSquareAttacked(5, !isWhite, board)
                && !isSquareAttacked(6, !isWhite, board))
                result |= squareBit(6);

            if (board->castleWQ
                && !(occ & squareBit(3)) && !(occ & squareBit(2)) && !(occ & squareBit(1))
                && !isSquareAttacked(3, !isWhite, board)
                && !isSquareAttacked(2, !isWhite, board))
                result |= squareBit(2);
        }
    }
    else {
        if (pieceBitboard & squareBit(60) && !isSquareAttacked(60, !isWhite, board)) {
            if (board->castleBK
                && !(occ & squareBit(61)) && !(occ & squareBit(62))
                && !isSquareAttacked(61, !isWhite, board)
                && !isSquareAttacked(62, !isWhite, board))
                result |= squareBit(62);

            if (board->castleBQ
                && !(occ & squareBit(59)) && !(occ & squareBit(58)) && !(occ & squareBit(57))
                && !isSquareAttacked(59, !isWhite, board)
                && !isSquareAttacked(58, !isWhite, board))
                result |= squareBit(58);
        }
    }

    return result & ~friendly;
}

uint64_t getValidPieceMoves(Piece piece, uint64_t pieceBitboard, Board* board, bool isWhite) {
    if (piece == N || piece == n) return getValidKnightMoves(pieceBitboard, board, isWhite);
    if (piece == R || piece == r) return getValidRookMoves(pieceBitboard, board, isWhite);
    if (piece == B || piece == b) return getValidBishopMoves(pieceBitboard, board, isWhite);
    if (piece == Q || piece == q) return getValidQueenMoves(pieceBitboard, board, isWhite);
    if (piece == K || piece == k) return getValidKingMoves(pieceBitboard, board, isWhite);
    if (piece == P || piece == p) return getValidPawnMoves(pieceBitboard, board, isWhite);
    return 0;
}

uint64_t getLegalMoves(Piece piece, int fromSq, Board* board, bool isWhite) {
    uint64_t pseudoMoves = getValidPieceMoves(piece, squareBit(fromSq), board, isWhite);
    uint64_t legalMoves = 0;

    while (pseudoMoves) {
        uint64_t lsb = pseudoMoves & (0ULL - pseudoMoves);
        int toSq = bitSquare(lsb);

        Move move = buildMove(board, piece, fromSq, toSq);
        makeMove(board, move);

        int kSq = isWhite ? bitSquare(board->bitboards[K])
            : bitSquare(board->bitboards[k]);
        if (!isSquareAttacked(kSq, !isWhite, board))
            legalMoves |= squareBit(toSq);

        unmakeMove(board, move);
        pseudoMoves &= pseudoMoves - 1;
    }
    return legalMoves;
}
