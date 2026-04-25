#pragma once
#include "types.h"
#include "raylib.h"
#include "search.h"
#include <map>

extern std::map<Piece, Texture2D> textures;
extern int squareEdgeSide;
extern int margin;

void drawBoard(Board* board);
void drawOutline(int file, int rank, Color clr);