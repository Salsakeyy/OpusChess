#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"
#include "move.h"
#include "board.h"
#include <vector>

class MoveGenerator {
public:
    // Generate all pseudo-legal moves
    static void generateMoves(const Board& board, std::vector<Move>& moves);
    
    // Generate only captures
    static void generateCaptures(const Board& board, std::vector<Move>& moves);
    
    // Generate legal moves (checks for legality)
    static void generateLegalMoves(const Board& board, std::vector<Move>& moves);
    
private:
    // Piece-specific move generators
    static void generatePawnMoves(const Board& board, Square from, std::vector<Move>& moves);
    static void generateKnightMoves(const Board& board, Square from, std::vector<Move>& moves);
    static void generateBishopMoves(const Board& board, Square from, std::vector<Move>& moves);
    static void generateRookMoves(const Board& board, Square from, std::vector<Move>& moves);
    static void generateQueenMoves(const Board& board, Square from, std::vector<Move>& moves);
    static void generateKingMoves(const Board& board, Square from, std::vector<Move>& moves);
    
    // Helper methods
    static void generateSlidingMoves(const Board& board, Square from, 
                                   const int* deltas, int numDeltas, 
                                   std::vector<Move>& moves);
    static void addMove(const Board& board, Square from, Square to, 
                       std::vector<Move>& moves, uint16_t flags = MOVE_NORMAL);
    static void addPawnMove(const Board& board, Square from, Square to, 
                           std::vector<Move>& moves);
    static void addPawnCapture(const Board& board, Square from, Square to, 
                              std::vector<Move>& moves);
};

#endif // MOVEGEN_H