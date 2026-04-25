#pragma once
#include "types.h"
#include <string>

bool validateFEN(const std::string& fen);
void parseFEN(const std::string& fen, Board& board);

void makeMove(Board* board, Move& move,bool finale=true);
void unmakeMove(Board* board, Move& move);
Move buildMove(Board* board, Piece piece, int from, int to);
void movePiece(uint64_t& bb, int from, int to);