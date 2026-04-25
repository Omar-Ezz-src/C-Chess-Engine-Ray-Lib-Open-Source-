#pragma once
#include "types.h"

uint64_t getValidPawnMoves(uint64_t bb, Board* board, bool isWhite);
uint64_t getValidKnightMoves(uint64_t bb, Board* board, bool isWhite);
uint64_t getValidBishopMoves(uint64_t bb, Board* board, bool isWhite);
uint64_t getValidRookMoves(uint64_t bb, Board* board, bool isWhite);
uint64_t getValidQueenMoves(uint64_t bb, Board* board, bool isWhite);
uint64_t getValidKingMoves(uint64_t bb, Board* board, bool isWhite);
uint64_t getValidPieceMoves(Piece piece, uint64_t bb, Board* board, bool isWhite);

uint64_t getLegalMoves(Piece piece, int fromSq, Board* board, bool isWhite);
bool     isSquareAttacked(int sq, bool byWhite, Board* board);