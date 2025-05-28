#ifndef EVALUATION_H
#define EVALUATION_H

#include "types.h"
#include "board.h"

class Evaluator {
public:
    // Main evaluation function
    static Score evaluate(const Board& board);
    
    // Evaluation components (for tuning/debugging)
    static Score evaluateMaterial(const Board& board);
    static Score evaluatePieceSquareTables(const Board& board);
    static Score evaluateMobility(const Board& board);
    static Score evaluatePawnStructure(const Board& board);
    static Score evaluateKingSafety(const Board& board);
    
private:
    // Material values
    static constexpr Score PAWN_VALUE = 100;
    static constexpr Score KNIGHT_VALUE = 320;
    static constexpr Score BISHOP_VALUE = 330;
    static constexpr Score ROOK_VALUE = 500;
    static constexpr Score QUEEN_VALUE = 900;
    
    // Piece-square tables
    static const Score PAWN_PST[64];
    static const Score KNIGHT_PST[64];
    static const Score BISHOP_PST[64];
    static const Score ROOK_PST[64];
    static const Score QUEEN_PST[64];
    static const Score KING_PST[64];
    static const Score KING_ENDGAME_PST[64];
    
    // Helper methods
    static Score getPieceSquareValue(Piece p, Square s, bool endgame = false);
    static bool isEndgame(const Board& board);
    static int countMaterial(const Board& board, Color c);
};

#endif // EVALUATION_H