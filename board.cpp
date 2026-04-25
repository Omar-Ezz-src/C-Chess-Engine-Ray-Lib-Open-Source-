#include <iostream>
#include <string>
#include "types.h"
#include "renderer.h"
#include <vector>
const uint64_t RANK_1 = 0x00000000000000FFULL;
const uint64_t RANK_8 = 0xFF00000000000000ULL;

using namespace std;

bool validateFEN(const string& fen) {
    vector<string> fields;
    string token;
    for (char c : fen) {
        if (c == ' ') { fields.push_back(token); token.clear(); }
        else token += c;
    }
    fields.push_back(token);

    if (fields.size() != 6) {
        cout << "FEN Error: Expected 6 fields, got " << fields.size() << "\n";
        return false;
    }

    vector<string> ranks;
    string rank;
    for (char c : fields[0]) {
        if (c == '/') { ranks.push_back(rank); rank.clear(); }
        else rank += c;
    }
    ranks.push_back(rank);

    if (ranks.size() != 8) {
        cout << "FEN Error: Expected 8 ranks, got " << ranks.size() << "\n";
        return false;
    }

    int whiteKings = 0, blackKings = 0;
    for (int i = 0; i < 8; i++) {
        int count = 0;
        for (char c : ranks[i]) {
            if (isdigit(c)) count += (c - '0');
            else if (string("PNBRQKpnbrqk").find(c) != string::npos) {
                count++;
                if (c == 'K') whiteKings++;
                if (c == 'k') blackKings++;
            }
            else {
                cout << "FEN Error: Invalid character '" << c << "' in rank " << (8 - i) << "\n";
                return false;
            }
        }
        if (count != 8) {
            cout << "FEN Error: Rank " << (8 - i) << " has " << count << " squares, expected 8\n";
            return false;
        }
    }

    if (whiteKings != 1) { cout << "FEN Error: Expected 1 white king, found " << whiteKings << "\n"; return false; }
    if (blackKings != 1) { cout << "FEN Error: Expected 1 black king, found " << blackKings << "\n"; return false; }

    if (fields[1] != "w" && fields[1] != "b") {
        cout << "FEN Error: Side to move must be 'w' or 'b', got '" << fields[1] << "'\n";
        return false;
    }

    if (fields[2] != "-") {
        for (char c : fields[2]) {
            if (string("KQkq").find(c) == string::npos) {
                cout << "FEN Error: Invalid castling character '" << c << "'\n";
                return false;
            }
        }
    }

    if (fields[3] != "-") {
        if (fields[3].size() != 2
            || fields[3][0] < 'a' || fields[3][0] > 'h'
            || (fields[3][1] != '3' && fields[3][1] != '6')) {
            cout << "FEN Error: Invalid en passant square '" << fields[3] << "'\n";
            return false;
        }
    }

    for (char c : fields[4]) {
        if (!isdigit(c)) {
            cout << "FEN Error: Halfmove clock must be a number, got '" << fields[4] << "'\n";
            return false;
        }
    }

    for (char c : fields[5]) {
        if (!isdigit(c)) {
            cout << "FEN Error: Fullmove number must be a number, got '" << fields[5] << "'\n";
            return false;
        }
    }
    if (stoi(fields[5]) < 1) {
        cout << "FEN Error: Fullmove number must be >= 1\n";
        return false;
    }

    return true;
}

void parseFEN(const std::string& fen, Board& board) {
    int rank = 7, file = 0;
    char lastChar = '.';
    for (char c : fen) {
        if (c == ' ') { lastChar = c; break; }
        if (lastChar == ' ') {
            switch (c) {
            case 'w': board.sideToMove = true;  break;
            case 'b': board.sideToMove = false; break;
            }
        }
        lastChar = c;

        if (c == '/') { rank--; file = 0; }
        else if (isdigit(c)) { file += (c - '0'); }
        else {
            Piece piece = None;
            switch (c) {
            case 'P': piece = P; break; case 'N': piece = N; break;
            case 'B': piece = B; break; case 'R': piece = R; break;
            case 'Q': piece = Q; break; case 'K': piece = K; break;
            case 'p': piece = p; break; case 'n': piece = n; break;
            case 'b': piece = b; break; case 'r': piece = r; break;
            case 'q': piece = q; break; case 'k': piece = k; break;
            }
            if (piece != None) {
                board.bitboards[piece] |= (1ULL << (rank * 8 + file));
                file++;
            }
        }
    }

    int spaces = 0;
    for (int i = 0; i < (int)fen.size(); i++) {
        if (fen[i] == ' ') spaces++;
        if (spaces == 2) {
            for (int j = i + 1; j < (int)fen.size() && fen[j] != ' '; j++) {
                if (fen[j] == 'K') board.castleWK = true;
                if (fen[j] == 'Q') board.castleWQ = true;
                if (fen[j] == 'k') board.castleBK = true;
                if (fen[j] == 'q') board.castleBQ = true;
            }
            break;
        }
    }
}

void movePiece(uint64_t& pieceBitboard, int from, int to) {
    pieceBitboard &= ~squareBit(from);
    pieceBitboard |= squareBit(to);
}

Move buildMove(Board* board, Piece piece, int from, int to) {
    Move move;
    move.from = from;
    move.to = to;
    move.piece = piece;
    move.captured = None;
    move.wasCastleWK = board->castleWK;
    move.wasCastleWQ = board->castleWQ;
    move.wasCastleBK = board->castleBK;
    move.wasCastleBQ = board->castleBQ;

    // Detect captured piece (search enemy range)
    int start = (piece < 6) ? 6 : 0;
    int end = (piece < 6) ? 12 : 6;
    for (int i = start; i < end; i++) {
        if ((board->bitboards[i] >> to) & 1) {
            move.captured = static_cast<Piece>(i);
            break;
        }
    }
    return move;
}

void makeMove(Board* board, Move& move,bool finale=false) {//finale = true : draw outline triggers on the last bot move
    movePiece(board->bitboards[move.piece], move.from, move.to);

    // Remove captured piece
    if (move.captured != None)
        board->bitboards[move.captured] &= ~squareBit(move.to);

    // Promotion
    if (move.piece == P && move.to / 8 == 7) {
        board->bitboards[P] &= ~squareBit(move.to);
        board->bitboards[Q] |= squareBit(move.to);
        move.promotedTo = Q;
    }
    if (move.piece == p && move.to / 8 == 0) {
        board->bitboards[p] &= ~squareBit(move.to);
        board->bitboards[q] |= squareBit(move.to);
        move.promotedTo = q;
    }
    

    // Move castling rook and revoke rights
    if (move.piece == K) {
        board->castleWK = board->castleWQ = false;
        if (move.from == 4 && move.to == 6) movePiece(board->bitboards[R], 7, 5);
        if (move.from == 4 && move.to == 2) movePiece(board->bitboards[R], 0, 3);
    }
    if (move.piece == k) {
        board->castleBK = board->castleBQ = false;
        if (move.from == 60 && move.to == 62) movePiece(board->bitboards[r], 63, 61);
        if (move.from == 60 && move.to == 58) movePiece(board->bitboards[r], 56, 59);
    }

    // Revoke castling rights if a rook moves from its starting square
    if (move.piece == R) {
        if (move.from == 0) board->castleWQ = false;
        if (move.from == 7) board->castleWK = false;
    }
    if (move.piece == r) {
        if (move.from == 56) board->castleBQ = false;
        if (move.from == 63) board->castleBK = false;
    }

    // Revoke castling rights if a rook is captured on its starting square
    if (move.captured == R) {
        if (move.to == 0) board->castleWQ = false;
        if (move.to == 7) board->castleWK = false;
    }
    if (move.captured == r) {
        if (move.to == 56) board->castleBQ = false;
        if (move.to == 63) board->castleBK = false;
    }

    

    board->sideToMove = !board->sideToMove;
}

void unmakeMove(Board* board, Move& move) {
    board->sideToMove = !board->sideToMove;

    // Undo promotion first
    if (move.promotedTo != None) {
        board->bitboards[move.promotedTo] &= ~squareBit(move.to);
        board->bitboards[move.piece] |= squareBit(move.from);
        if (move.captured != None)
            board->bitboards[move.captured] |= squareBit(move.to);
        board->castleWK = move.wasCastleWK;
        board->castleWQ = move.wasCastleWQ;
        board->castleBK = move.wasCastleBK;
        board->castleBQ = move.wasCastleBQ;
        return;
    }

    movePiece(board->bitboards[move.piece], move.to, move.from);

    if (move.captured != None)
        board->bitboards[move.captured] |= squareBit(move.to);

    if (move.piece == K) {
        if (move.from == 4 && move.to == 6) movePiece(board->bitboards[R], 5, 7);
        if (move.from == 4 && move.to == 2) movePiece(board->bitboards[R], 3, 0);
    }
    if (move.piece == k) {
        if (move.from == 60 && move.to == 62) movePiece(board->bitboards[r], 61, 63);
        if (move.from == 60 && move.to == 58) movePiece(board->bitboards[r], 59, 56);
    }

    board->castleWK = move.wasCastleWK;
    board->castleWQ = move.wasCastleWQ;
    board->castleBK = move.wasCastleBK;
    board->castleBQ = move.wasCastleBQ;
}