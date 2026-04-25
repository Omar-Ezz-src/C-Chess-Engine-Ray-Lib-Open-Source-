#include "renderer.h"
#include "movegen.h"
#include "search.h"
#include "evaluate.h"
#include "raymath.h"
#include <iostream>

using namespace std;

Color boardDarkColor = BROWN;
Color boardLightColor = GRAY;
Color availableMoveColor = GREEN;
Color backGroundColor = DARKBROWN;

void drawOutline(int file, int rank, Color clr = BLACK) {
    int size = squareEdgeSide;
    Rectangle rec = {
        (float)(size * file + margin),
        (float)(size * rank + margin),
        (float)size, (float)size
    };
    DrawRectangleLinesEx(rec, 2, clr);
}

void drawPiece(Piece piece, Vector2 pos, Color tint = WHITE) {
    DrawTexture(textures[piece], (int)(pos.x + margin), (int)(pos.y + margin), tint);
}

void drawSquare(int file, int rank) {
    int size = squareEdgeSide;
    Color color = (rank + file) % 2 == 1 ? boardDarkColor : boardLightColor;
    DrawRectangle(size * file + margin, size * rank + margin, size, size, color);
}

void drawMoveIndicator(int file, int rank, bool isCapture) {
    int size = squareEdgeSide;
    int cx = (size * file + margin) + size / 2;
    int cy = (size * rank + margin) + size / 2;
    if (isCapture) drawOutline(file, rank, GREEN);
    else           DrawCircle(cx, cy, size / 6, availableMoveColor);
}

void drawPieceAt(Board* board, int sq, int file, int rank) {
    int size = squareEdgeSide;
    for (int i = 0; i < 12; i++) {
        if ((board->bitboards[i] >> sq) & 1) {
            drawPiece(static_cast<Piece>(i), { (float)(size * file), (float)(size * rank) });
            break;
        }
    }
}

void drawEvaluationBar(Board* board) {
    int barWidth = 16;
    int barHeight = squareEdgeSide * 8 + margin * 2; // full board height
    int barX = squareEdgeSide * 8 + margin * 2;
    int barY = 0;

    // Clamp score to [-1000, 1000] centipawns then map to pixel height
    int score = evaluate(board);
    score = score < -1000 ? -1000 : score > 1000 ? 1000 : score;

    // whiteHeight: how many pixels belong to white (grows from bottom)
    // At score=0 → exactly half. Positive → white gets more.
    int whiteHeight = (int)((score + 1000) / 2000.0f * barHeight);
    whiteHeight = whiteHeight < 4 ? 4 : whiteHeight > barHeight - 4 ? barHeight - 4 : whiteHeight;
    int blackHeight = barHeight - whiteHeight;

    // Black on top, white on bottom (chess.com style)
    DrawRectangle(barX, barY, barWidth, blackHeight, { 31,  31,  31,  255 });
    DrawRectangle(barX, barY + blackHeight, barWidth, whiteHeight, { 255, 255, 255, 255 });

    // Score label
    const char* label = TextFormat("%+.1f", score / 100.0f);
    int fontSize = 8;
    int textX = barX + barWidth / 2 - MeasureText(label, fontSize) / 2;

    if (score >= 0) // white advantage: label near top of white region
        DrawText(label, textX, barY + blackHeight + 4, fontSize, BLACK);
    else            // black advantage: label near bottom of black region
        DrawText(label, textX, barY + blackHeight - fontSize - 4, fontSize, WHITE);
}

void drawBoard(Board* board) {
    int size = squareEdgeSide;
    DrawRectangle(0, 0, size * 8 + margin * 2, size * 8 + margin * 2, backGroundColor);

    // Use getLegalMoves so indicators only show truly legal squares
    uint64_t availableMoves = 0;
    if (board->heldSquare != -1)
        availableMoves = getLegalMoves(
            board->heldPieceType,
            board->heldSquare,
            board,
            board->sideToMove
        );

    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            drawSquare(file, rank);

            if (Vector2Equals(board->highlightedSquare, { (float)file, (float)rank }))
                drawOutline(file, rank);

            int sq = (7 - rank) * 8 + file;
            bool isAvailable = (availableMoves >> sq) & 1;
            bool isOccupied = (board->occupied() >> sq) & 1;

            if (isAvailable) drawMoveIndicator(file, rank, isOccupied);
            if (!isOccupied) continue;

            drawPieceAt(board, sq, file, rank);
        }
    }
    drawEvaluationBar(board);
}