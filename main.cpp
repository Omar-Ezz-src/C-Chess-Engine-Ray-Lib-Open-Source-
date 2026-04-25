#include "raylib.h"
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "renderer.h"
#include "search.h"
#include "raymath.h"

#include <iostream>
#include <map>
#include <string>
using namespace std;

map<Piece, Texture2D> textures;
int squareEdgeSide = 64;
int margin = 32;

// ── UI interactions ──────────────────────────────────────────────────────────

void pickUpPiece(Board* board, int sq) {
    int start = board->sideToMove ? 0 : 6;
    int end = board->sideToMove ? 6 : 12;
    for (int i = start; i < end; i++) {
        if ((board->bitboards[i] >> sq) & 1) {
            board->heldPieceType = static_cast<Piece>(i);
            board->heldSquare = sq;
            return;
        }
    }
}

Move dropPiece(Board* board, int to) {
    if (board->heldSquare == -1) return Move{};
    if (!(getLegalMoves(board->heldPieceType, board->heldSquare, board, board->sideToMove) & squareBit(to))) {
        board->heldPieceType = None;
        board->heldSquare = -1;
        return Move{};
    }
    Move move = buildMove(board, board->heldPieceType, board->heldSquare, to);
    makeMove(board, move);
    board->heldPieceType = None;
    board->heldSquare = -1;
    return move;
}

// ── Entry point ──────────────────────────────────────────────────────────────

int main() {
    SetTraceLogLevel(LOG_WARNING);
    Board* board = new Board();

    string defFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    string FEN=defFEN;
    bool validFen = false;

    bool botVsBot = false;
    char bvbChoice = ' ';

    int depth = 4;

    cout << "custom game? Y/n : ";
    char customGame = ' ';
    cin >> customGame;

    if (customGame == 'Y') {
        cout << "Want bot vs bot? Y/n : ";
        cin >> bvbChoice;
        cout << "\n";
        if (bvbChoice == 'Y') botVsBot = true;

        char customDepthChoice = ' ';
        cout << "at custom depth? Y/n : ";
        cin >> customDepthChoice;
        cout << "\n";
        if (customDepthChoice=='Y') {
            cout << "at what depth? [ 3 - 10 (extremly slow) ] : ";
            cin >> depth;
            depth = Clamp(depth, 3, 10);
            cout << "\n";
        }

        if (bvbChoice == 'Y') botVsBot = true;

        while (!validFen) {
            char choice = ' ';
            cout << "Want to enter custom FEN? Y/n : ";
            cin >> choice;
            cout << "\n";
            if (choice == 'Y') {
                cout << "Enter FEN: ";
                cin.ignore();
                getline(cin, FEN);
                cout << "\n";
                if (validateFEN(FEN)) validFen = true;
                else cout << "Invalid FEN.\n";
            }
            else {
                validFen = true;
            }
        }
    }

    parseFEN(FEN, *board);

    const int size = squareEdgeSide * 8 + margin * 2;
    InitWindow(size + 16, size, "Chess");

    textures = {
        { P, LoadTexture("assets/w_pawn.png")   },
        { K, LoadTexture("assets/w_king.png")   },
        { Q, LoadTexture("assets/w_queen.png")  },
        { B, LoadTexture("assets/w_bishop.png") },
        { N, LoadTexture("assets/w_knight.png") },
        { R, LoadTexture("assets/w_rook.png")   },
        { p, LoadTexture("assets/b_pawn.png")   },
        { k, LoadTexture("assets/b_king.png")   },
        { q, LoadTexture("assets/b_queen.png")  },
        { b, LoadTexture("assets/b_bishop.png") },
        { n, LoadTexture("assets/b_knight.png") },
        { r, LoadTexture("assets/b_rook.png")   },
    };

    SetTargetFPS(60);

    bool engineMoved = false; // prevent engine running every frame
    Move lastMove = Move{};
    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();
        board->highlightedSquare = {
            floorf((mousePos.x - margin) / squareEdgeSide),
            floorf((mousePos.y - margin) / squareEdgeSide)
        };
       
        if (botVsBot) {
            // ── Bot vs Bot: alternate every frame using engineMoved flag ──
            if (!engineMoved) {
                SearchResult result = findBestMove(board, depth);
                if (result.move.from != -1) {
                    makeMove(board, result.move);
                    lastMove = (result.move);
                }
                engineMoved = true;
            }
            else {
                engineMoved = false; // wait one frame between moves so UI renders
            }
        }
        else {
            // ── Human (white) vs Engine (black) ───────────────────────────
            if (board->sideToMove == true) {
                // Human's turn
                
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    int file = (int)board->highlightedSquare.x;
                    int rank = (int)board->highlightedSquare.y;
                    int sq = square(file, 7 - rank);
                    if (board->heldSquare == -1) pickUpPiece(board, sq);
                    else {
                        lastMove = dropPiece(board, sq);
                        engineMoved = false;
                    }
                }
            }
            else {
                // Engine's turn — only run once per turn
                if (!engineMoved) {
                    engineMoved = true;
                    SearchResult result = findBestMove(board, depth);
                    if (result.move.from != -1) {
                        makeMove(board, result.move, true);
                        lastMove = (result.move);
                    }
                }
            }
        }

        BeginDrawing();
        drawBoard(board);
        if (engineMoved) {
            int fromFile = lastMove.from % 8;
            int fromRank = 7-floor(lastMove.from / 8);
            drawOutline(fromFile, fromRank, YELLOW);
            int toFile = lastMove.to % 8;
            int toRank = 7-floor(lastMove.to / 8);
            drawOutline(toFile, toRank, YELLOW);
        }
        EndDrawing();
    }

    for (auto& pair : textures)
        UnloadTexture(pair.second);
    CloseWindow();
    delete board;
}