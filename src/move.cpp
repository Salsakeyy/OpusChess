#include "move.h"

Move MoveUtils::makePromotion(Square from, Square to, PieceType promotionType) {
    uint16_t flags = MOVE_PROMOTION;
    if (promotionType != NO_PIECE_TYPE) {
        // Encode promotion piece (0=Knight, 1=Bishop, 2=Rook, 3=Queen)
        flags |= ((promotionType - KNIGHT) << 12);
    }
    return makeMove(from, to, flags);
}

std::string MoveUtils::toString(Move m) {
    if (m == 0) return "0000";
    
    std::string result = squareToString(from(m)) + squareToString(to(m));
    
    if (isPromotion(m)) {
        const char* pieces = "nbrq";
        PieceType pt = promotionType(m);
        if (pt >= KNIGHT && pt <= QUEEN) {
            result += pieces[pt - KNIGHT];
        }
    }
    
    return result;
}

Move MoveUtils::fromString(const std::string& str) {
    if (str.length() < 4) return 0;
    
    int fromFile = str[0] - 'a';
    int fromRank = str[1] - '1';
    int toFile = str[2] - 'a';
    int toRank = str[3] - '1';
    
    if (fromFile < 0 || fromFile > 7 || fromRank < 0 || fromRank > 7 ||
        toFile < 0 || toFile > 7 || toRank < 0 || toRank > 7) {
        return 0;
    }
    
    Square from = makeSquare(fromFile, fromRank);
    Square to = makeSquare(toFile, toRank);
    
    uint16_t flags = MOVE_NORMAL;
    
    // Handle promotion
    if (str.length() > 4) {
        PieceType pt = NO_PIECE_TYPE;
        switch (str[4]) {
            case 'q': pt = QUEEN; break;
            case 'r': pt = ROOK; break;
            case 'b': pt = BISHOP; break;
            case 'n': pt = KNIGHT; break;
        }
        if (pt != NO_PIECE_TYPE) {
            return makePromotion(from, to, pt);
        }
    }
    
    return makeMove(from, to, flags);
}