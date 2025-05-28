#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>

// Basic types
using Square = int;
using File = int;
using Rank = int;
using Color = int;
using Piece = int;
using PieceType = int;
using Move = uint16_t;
using Score = int;

// Constants
constexpr int NUM_SQUARES = 64;
constexpr int NUM_FILES = 8;
constexpr int NUM_RANKS = 8;

// Colors
constexpr Color WHITE = 0;
constexpr Color BLACK = 1;
constexpr Color NO_COLOR = 2;

// Piece types
constexpr PieceType PAWN = 0;
constexpr PieceType KNIGHT = 1;
constexpr PieceType BISHOP = 2;
constexpr PieceType ROOK = 3;
constexpr PieceType QUEEN = 4;
constexpr PieceType KING = 5;
constexpr PieceType NO_PIECE_TYPE = 6;

// Pieces (color + piece type)
constexpr Piece WHITE_PAWN = 0;
constexpr Piece WHITE_KNIGHT = 1;
constexpr Piece WHITE_BISHOP = 2;
constexpr Piece WHITE_ROOK = 3;
constexpr Piece WHITE_QUEEN = 4;
constexpr Piece WHITE_KING = 5;
constexpr Piece BLACK_PAWN = 6;
constexpr Piece BLACK_KNIGHT = 7;
constexpr Piece BLACK_BISHOP = 8;
constexpr Piece BLACK_ROOK = 9;
constexpr Piece BLACK_QUEEN = 10;
constexpr Piece BLACK_KING = 11;
constexpr Piece NO_PIECE = 12;

// Squares
constexpr Square A1 = 0, B1 = 1, C1 = 2, D1 = 3, E1 = 4, F1 = 5, G1 = 6, H1 = 7;
constexpr Square A8 = 56, B8 = 57, C8 = 58, D8 = 59, E8 = 60, F8 = 61, G8 = 62, H8 = 63;

// Move flags
constexpr uint16_t MOVE_NORMAL = 0;
constexpr uint16_t MOVE_CAPTURE = 1 << 14;
constexpr uint16_t MOVE_CASTLE = 1 << 15;
constexpr uint16_t MOVE_EN_PASSANT = (1 << 14) | (1 << 15);
constexpr uint16_t MOVE_PROMOTION = 1 << 13;

// Castling rights
constexpr int WHITE_KINGSIDE = 1;
constexpr int WHITE_QUEENSIDE = 2;
constexpr int BLACK_KINGSIDE = 4;
constexpr int BLACK_QUEENSIDE = 8;

// Utility functions
inline Square makeSquare(File f, Rank r) {
    return r * 8 + f;
}

inline File fileOf(Square s) {
    return s & 7;
}

inline Rank rankOf(Square s) {
    return s >> 3;
}

inline Color colorOf(Piece p) {
    return p < 6 ? WHITE : BLACK;
}

inline PieceType typeOf(Piece p) {
    return p % 6;
}

inline Piece makePiece(Color c, PieceType pt) {
    return c * 6 + pt;
}

inline std::string squareToString(Square s) {
    return std::string(1, 'a' + fileOf(s)) + std::string(1, '1' + rankOf(s));
}

#endif // TYPES_H