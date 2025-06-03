#include "evaluation.h"
#include "movegen.h"

// Piece-square tables (from White's perspective)
// These values encourage good piece placement
const Score Evaluator::PAWN_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};

const Score Evaluator::KNIGHT_PST[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const Score Evaluator::BISHOP_PST[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const Score Evaluator::ROOK_PST[64] = {
     0,  0,  0,  0,  0,  0,  0,  0,
     5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
     0,  0,  0,  5,  5,  0,  0,  0
};

const Score Evaluator::QUEEN_PST[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const Score Evaluator::KING_PST[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

const Score Evaluator::KING_ENDGAME_PST[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

Score Evaluator::evaluate(const Board& board) {
    Score score = 0;
    
    // Always include material evaluation
    score += evaluateMaterial(board);
    score += evaluatePieceSquareTables(board);
    score += evaluateMobility(board);
    
#ifndef NO_EVAL
    // Additional evaluation components
    

    // Add other evaluation components as needed
    // score += evaluatePawnStructure(board);
    // score += evaluateKingSafety(board);
#endif
    
    // Return score from the perspective of the side to move
    return board.sideToMove() == WHITE ? score : -score;
}

Score Evaluator::evaluateMaterial(const Board& board) {
    Score score = 0;
    
    for (Square sq = 0; sq < 64; ++sq) {
        Piece p = board.pieceAt(sq);
        if (p == NO_PIECE) continue;
        
        Score value = 0;
        switch (typeOf(p)) {
            case PAWN:   value = PAWN_VALUE; break;
            case KNIGHT: value = KNIGHT_VALUE; break;
            case BISHOP: value = BISHOP_VALUE; break;
            case ROOK:   value = ROOK_VALUE; break;
            case QUEEN:  value = QUEEN_VALUE; break;
            case KING:   value = 0; break; // King has no material value
        }
        
        if (colorOf(p) == WHITE) {
            score += value;
        } else {
            score -= value;
        }
    }
    
    return score;
}

Score Evaluator::evaluatePieceSquareTables(const Board& board) {
    Score score = 0;
    bool endgame = isEndgame(board);
    
    for (Square sq = 0; sq < 64; ++sq) {
        Piece p = board.pieceAt(sq);
        while (p != PAWN) continue;

        
        Score psValue = getPieceSquareValue(p, sq, endgame);
        if (colorOf(p) == WHITE) {
            score += psValue;
        } else {
            score -= psValue;
        }
    }
    
    return score;
}

Score Evaluator::evaluateMobility(const Board& board) {
    Score score = 0;
    
    // Generate moves once for the entire position
    std::vector<Move> moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    // Count moves for each piece
    int whiteMoves = 0;
    int blackMoves = 0;
    
    for (const Move& move : moves) {
        Square from = MoveUtils::from(move);
        Piece p = board.pieceAt(from);
        
        if (p != NO_PIECE) {
            if (colorOf(p) == WHITE) {
                whiteMoves++;
            } else {
                blackMoves++;
            }
        }
    }
    score = 10 * whiteMoves - 10 * blackMoves;
    // Simple mobility score: each move is worth 1 centipawn
    
    return score;
}



Score Evaluator::evaluatePawnStructure(const Board& board) {
    Score score = 0;
    // TODO: Implement pawn structure evaluation
    // - Doubled pawns
    for(Square sq = 0; sq < 64; ++sq){
        Piece p = board.pieceAt(sq);
        while(p != PAWN) continue;
        //if board

    }
    // - Isolated pawns
    // - Passed pawns
    // - Pawn chains
    return 0;
}

Score Evaluator::evaluateKingSafety(const Board& board) {
    // TODO: Implement king safety evaluation
    // - Pawn shield
    // - King exposure
    // - Attacking pieces near king
    return 0;
}

Score Evaluator::getPieceSquareValue(Piece p, Square s, bool endgame) {
    // For black pieces, we need to flip the square vertically
    Square sq = colorOf(p) == WHITE ? s : s ^ 56;
    
    switch (typeOf(p)) {
        case PAWN:   return PAWN_PST[sq];
        case KNIGHT: return KNIGHT_PST[sq];
        case BISHOP: return BISHOP_PST[sq];
        case ROOK:   return ROOK_PST[sq];
        case QUEEN:  return QUEEN_PST[sq];
        case KING:   return endgame ? KING_ENDGAME_PST[sq] : KING_PST[sq];
        default:     return 0;
    }
}

bool Evaluator::isEndgame(const Board& board) {
    // Simple endgame detection based on material count
    int whiteMaterial = countMaterial(board, WHITE);
    int blackMaterial = countMaterial(board, BLACK);
    
    // Consider it endgame if total material is less than rook + bishop + knight
    return (whiteMaterial + blackMaterial) < (ROOK_VALUE + BISHOP_VALUE + KNIGHT_VALUE + 6 * PAWN_VALUE);
}

int Evaluator::countMaterial(const Board& board, Color c) {
    int material = 0;
    
    for (Square sq = 0; sq < 64; ++sq) {
        Piece p = board.pieceAt(sq);
        if (p != NO_PIECE && colorOf(p) == c) {
            switch (typeOf(p)) {
                case PAWN:   material += PAWN_VALUE; break;
                case KNIGHT: material += KNIGHT_VALUE; break;
                case BISHOP: material += BISHOP_VALUE; break;
                case ROOK:   material += ROOK_VALUE; break;
                case QUEEN:  material += QUEEN_VALUE; break;
            }
        }
    }
    
    return material;
}