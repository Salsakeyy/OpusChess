#ifndef MOVE_H
#define MOVE_H

#include "types.h"
#include <string>

// Move encoding: 16 bits
// bits 0-5: from square
// bits 6-11: to square
// bits 12-15: flags (promotion piece, special moves)

class MoveUtils {
public:
    static Move makeMove(Square from, Square to, uint16_t flags = MOVE_NORMAL);
    static Move makePromotion(Square from, Square to, PieceType promotionType);
    
    static Square from(Move m);
    static Square to(Move m);
    static uint16_t flags(Move m);
    static bool isCapture(Move m);
    static bool isCastle(Move m);
    static bool isEnPassant(Move m);
    static bool isPromotion(Move m);
    static PieceType promotionType(Move m);
    
    static std::string toString(Move m);
    static Move fromString(const std::string& str);
};

inline Move MoveUtils::makeMove(Square from, Square to, uint16_t flags) {
    return from | (to << 6) | flags;
}

inline Square MoveUtils::from(Move m) {
    return m & 0x3F;
}

inline Square MoveUtils::to(Move m) {
    return (m >> 6) & 0x3F;
}

inline uint16_t MoveUtils::flags(Move m) {
    return m & 0xF000;
}

inline bool MoveUtils::isCapture(Move m) {
    return (m & MOVE_CAPTURE) != 0;
}

inline bool MoveUtils::isCastle(Move m) {
    return (m & 0xC000) == MOVE_CASTLE;
}

inline bool MoveUtils::isEnPassant(Move m) {
    return (m & 0xC000) == MOVE_EN_PASSANT;
}

inline bool MoveUtils::isPromotion(Move m) {
    return (m & MOVE_PROMOTION) != 0;
}

inline PieceType MoveUtils::promotionType(Move m) {
    if (!isPromotion(m)) return NO_PIECE_TYPE;
    return ((m >> 12) & 3) + KNIGHT; // Knight, Bishop, Rook, Queen
}

#endif // MOVE_H