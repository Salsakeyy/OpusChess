#include "movegen_fast.h"

void FastMoveGenerator::generateMoves(const Board& board, std::vector<Move>& moves) {
    moves.clear();
    moves.reserve(256); // Reserve space to avoid reallocations
    
    Color us = board.sideToMove();
    
    // Generate moves for each piece type using piece lists (NO 64-square scan!)
    // Pawns
    for (Square sq : board.getPieceList(makePiece(us, PAWN))) {
        generatePawnMoves(board, sq, moves);
    }
    
    // Knights  
    for (Square sq : board.getPieceList(makePiece(us, KNIGHT))) {
        generateKnightMoves(board, sq, moves);
    }
    
    // Bishops
    for (Square sq : board.getPieceList(makePiece(us, BISHOP))) {
        generateBishopMoves(board, sq, moves);
    }
    
    // Rooks
    for (Square sq : board.getPieceList(makePiece(us, ROOK))) {
        generateRookMoves(board, sq, moves);
    }
    
    // Queens
    for (Square sq : board.getPieceList(makePiece(us, QUEEN))) {
        generateQueenMoves(board, sq, moves);
    }
    
    // King
    for (Square sq : board.getPieceList(makePiece(us, KING))) {
        generateKingMoves(board, sq, moves);
    }
    
    // Castling
    generateCastling(board, moves);
}

void FastMoveGenerator::generateCaptures(const Board& board, std::vector<Move>& moves) {
    moves.clear();
    moves.reserve(64);
    
    Color us = board.sideToMove();
    
    // Generate captures for each piece type
    for (Square sq : board.getPieceList(makePiece(us, PAWN))) {
        uint64_t attacks = Attacks::getPawnAttacks(sq, us);
        addCapturesFromBitboard(board, sq, attacks, moves);
        
        // En passant
        Square ep = board.enPassantSquare();
        if (ep != -1 && Attacks::testBit(attacks, ep)) {
            moves.push_back(MoveUtils::makeMove(sq, ep, MOVE_EN_PASSANT));
        }
    }
    
    for (Square sq : board.getPieceList(makePiece(us, KNIGHT))) {
        uint64_t attacks = Attacks::getKnightAttacks(sq);
        addCapturesFromBitboard(board, sq, attacks, moves);
    }
    
    for (Square sq : board.getPieceList(makePiece(us, BISHOP))) {
        uint64_t occupancy = getOccupancy(board);
        uint64_t attacks = Attacks::getBishopAttacks(sq, occupancy);
        addCapturesFromBitboard(board, sq, attacks, moves);
    }
    
    for (Square sq : board.getPieceList(makePiece(us, ROOK))) {
        uint64_t occupancy = getOccupancy(board);
        uint64_t attacks = Attacks::getRookAttacks(sq, occupancy);
        addCapturesFromBitboard(board, sq, attacks, moves);
    }
    
    for (Square sq : board.getPieceList(makePiece(us, QUEEN))) {
        uint64_t occupancy = getOccupancy(board);
        uint64_t attacks = Attacks::getQueenAttacks(sq, occupancy);
        addCapturesFromBitboard(board, sq, attacks, moves);
    }
    
    for (Square sq : board.getPieceList(makePiece(us, KING))) {
        uint64_t attacks = Attacks::getKingAttacks(sq);
        addCapturesFromBitboard(board, sq, attacks, moves);
    }
}

void FastMoveGenerator::generateLegalMoves(const Board& board, std::vector<Move>& moves) {
    generateMoves(board, moves);
    
    // Filter out illegal moves (simplified - full implementation would be more complex)
    moves.erase(std::remove_if(moves.begin(), moves.end(),
        [&board](Move m) { return !board.isLegalMove(m); }), moves.end());
}

inline void FastMoveGenerator::generatePawnMoves(const Board& board, Square from, std::vector<Move>& moves) {
    Color us = board.sideToMove();
    int direction = (us == WHITE) ? 8 : -8;
    int startRank = (us == WHITE) ? 1 : 6;
    int promoRank = (us == WHITE) ? 6 : 1;
    
    // Forward moves
    Square to = from + direction;
    if (to >= 0 && to < 64 && board.pieceAt(to) == NO_PIECE) {
        if (rankOf(from) == promoRank) {
            // Promotion
            for (PieceType pt = QUEEN; pt >= KNIGHT; --pt) {
                Move m = MoveUtils::makePromotion(from, to, pt);
                moves.push_back(m);
            }
        } else {
            moves.push_back(MoveUtils::makeMove(from, to));
            
            // Double push from starting rank
            if (rankOf(from) == startRank) {
                to = from + 2 * direction;
                if (to >= 0 && to < 64 && board.pieceAt(to) == NO_PIECE) {
                    moves.push_back(MoveUtils::makeMove(from, to));
                }
            }
        }
    }
    
    // Captures using precomputed attack table
    uint64_t attacks = Attacks::getPawnAttacks(from, us);
    addCapturesFromBitboard(board, from, attacks, moves);
    
    // En passant
    Square ep = board.enPassantSquare();
    if (ep != -1 && Attacks::testBit(attacks, ep)) {
        moves.push_back(MoveUtils::makeMove(from, ep, MOVE_EN_PASSANT));
    }
}

inline void FastMoveGenerator::generateKnightMoves(const Board& board, Square from, std::vector<Move>& moves) {
    uint64_t attacks = Attacks::getKnightAttacks(from);
    
    // Split into captures and quiet moves
    addCapturesFromBitboard(board, from, attacks, moves);
    addMovesFromBitboard(from, attacks & ~getOccupancy(board), moves);
}

inline void FastMoveGenerator::generateBishopMoves(const Board& board, Square from, std::vector<Move>& moves) {
    uint64_t occupancy = getOccupancy(board);
    uint64_t attacks = Attacks::getBishopAttacks(from, occupancy);
    
    addCapturesFromBitboard(board, from, attacks, moves);
    addMovesFromBitboard(from, attacks & ~occupancy, moves);
}

inline void FastMoveGenerator::generateRookMoves(const Board& board, Square from, std::vector<Move>& moves) {
    uint64_t occupancy = getOccupancy(board);
    uint64_t attacks = Attacks::getRookAttacks(from, occupancy);
    
    addCapturesFromBitboard(board, from, attacks, moves);
    addMovesFromBitboard(from, attacks & ~occupancy, moves);
}

inline void FastMoveGenerator::generateQueenMoves(const Board& board, Square from, std::vector<Move>& moves) {
    uint64_t occupancy = getOccupancy(board);
    uint64_t attacks = Attacks::getQueenAttacks(from, occupancy);
    
    addCapturesFromBitboard(board, from, attacks, moves);
    addMovesFromBitboard(from, attacks & ~occupancy, moves);
}

inline void FastMoveGenerator::generateKingMoves(const Board& board, Square from, std::vector<Move>& moves) {
    uint64_t attacks = Attacks::getKingAttacks(from);
    
    addCapturesFromBitboard(board, from, attacks, moves);
    addMovesFromBitboard(from, attacks & ~getOccupancy(board), moves);
}

inline void FastMoveGenerator::generateCastling(const Board& board, std::vector<Move>& moves) {
    Color us = board.sideToMove();
    
    if (us == WHITE) {
        if ((board.castlingRights() & WHITE_KINGSIDE) && 
            board.pieceAt(F1) == NO_PIECE && board.pieceAt(G1) == NO_PIECE &&
            !board.isAttacked(E1, BLACK) && !board.isAttacked(F1, BLACK) && !board.isAttacked(G1, BLACK)) {
            moves.push_back(MoveUtils::makeMove(E1, G1, MOVE_CASTLE));
        }
        if ((board.castlingRights() & WHITE_QUEENSIDE) &&
            board.pieceAt(D1) == NO_PIECE && board.pieceAt(C1) == NO_PIECE && board.pieceAt(B1) == NO_PIECE &&
            !board.isAttacked(E1, BLACK) && !board.isAttacked(D1, BLACK) && !board.isAttacked(C1, BLACK)) {
            moves.push_back(MoveUtils::makeMove(E1, C1, MOVE_CASTLE));
        }
    } else {
        if ((board.castlingRights() & BLACK_KINGSIDE) &&
            board.pieceAt(F8) == NO_PIECE && board.pieceAt(G8) == NO_PIECE &&
            !board.isAttacked(E8, WHITE) && !board.isAttacked(F8, WHITE) && !board.isAttacked(G8, WHITE)) {
            moves.push_back(MoveUtils::makeMove(E8, G8, MOVE_CASTLE));
        }
        if ((board.castlingRights() & BLACK_QUEENSIDE) &&
            board.pieceAt(D8) == NO_PIECE && board.pieceAt(C8) == NO_PIECE && board.pieceAt(B8) == NO_PIECE &&
            !board.isAttacked(E8, WHITE) && !board.isAttacked(D8, WHITE) && !board.isAttacked(C8, WHITE)) {
            moves.push_back(MoveUtils::makeMove(E8, C8, MOVE_CASTLE));
        }
    }
}

inline uint64_t FastMoveGenerator::getOccupancy(const Board& board) {
    uint64_t occupancy = 0;
    for (Square sq = 0; sq < 64; ++sq) {
        if (board.pieceAt(sq) != NO_PIECE) {
            occupancy = Attacks::setBit(occupancy, sq);
        }
    }
    return occupancy;
}

inline void FastMoveGenerator::addMovesFromBitboard(Square from, uint64_t targets, 
                                                   std::vector<Move>& moves, uint16_t flags) {
    while (targets) {
        Square to = __builtin_ctzll(targets); // Find first set bit
        targets = Attacks::clearBit(targets, to);
        moves.push_back(MoveUtils::makeMove(from, to, flags));
    }
}

inline void FastMoveGenerator::addCapturesFromBitboard(const Board& board, Square from, 
                                                      uint64_t targets, std::vector<Move>& moves) {
    Color them = 1 - board.sideToMove();
    
    while (targets) {
        Square to = __builtin_ctzll(targets);
        targets = Attacks::clearBit(targets, to);
        
        Piece captured = board.pieceAt(to);
        if (captured != NO_PIECE && colorOf(captured) == them) {
            moves.push_back(MoveUtils::makeMove(from, to, MOVE_CAPTURE));
        }
    }
}