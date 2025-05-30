#ifndef MOVEGEN_FAST_H
#define MOVEGEN_FAST_H

#include "types.h"
#include "move.h"
#include "board.h"
#include "attacks.h"
#include <vector>

class FastMoveGenerator {
public:
    // Generate all pseudo-legal moves using piece lists and precomputed tables
    static void generateMoves(const Board& board, std::vector<Move>& moves);
    
    // Generate only captures for quiescence search
    static void generateCaptures(const Board& board, std::vector<Move>& moves);
    
    // Generate legal moves (checks for legality)
    static void generateLegalMoves(const Board& board, std::vector<Move>& moves);
    
private:
    // Fast piece-specific generators using precomputed tables
    static inline void generatePawnMoves(const Board& board, Square from, std::vector<Move>& moves);
    static inline void generateKnightMoves(const Board& board, Square from, std::vector<Move>& moves);
    static inline void generateBishopMoves(const Board& board, Square from, std::vector<Move>& moves);
    static inline void generateRookMoves(const Board& board, Square from, std::vector<Move>& moves);
    static inline void generateQueenMoves(const Board& board, Square from, std::vector<Move>& moves);
    static inline void generateKingMoves(const Board& board, Square from, std::vector<Move>& moves);
    
    // Fast castling generator
    static inline void generateCastling(const Board& board, std::vector<Move>& moves);
    
    // Helper methods for bitboard operations
    static inline uint64_t getOccupancy(const Board& board);
    static inline void addMovesFromBitboard(Square from, uint64_t targets, 
                                          std::vector<Move>& moves, 
                                          uint16_t flags = MOVE_NORMAL);
    static inline void addCapturesFromBitboard(const Board& board, Square from, 
                                             uint64_t targets, std::vector<Move>& moves);
};

#endif // MOVEGEN_FAST_H