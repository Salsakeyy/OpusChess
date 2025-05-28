#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "move.h"
#include <string>
#include <vector>

class Board {
public:
    Board();
    
    // FEN operations
    void setFromFEN(const std::string& fen);
    std::string toFEN() const;
    
    // Board state
    Piece pieceAt(Square s) const { return squares[s]; }
    Color sideToMove() const { return stm; }
    int castlingRights() const { return castling; }
    Square enPassantSquare() const { return epSquare; }
    int halfmoveClock() const { return halfmoves; }
    int fullmoveNumber() const { return fullmoves; }
    
    // Move operations
    void makeMove(Move m);
    void unmakeMove(Move m);
    bool isLegalMove(Move m) const;
    
    // Position queries
    bool isInCheck(Color c) const;
    bool isAttacked(Square s, Color by) const;
    Square kingSquare(Color c) const;
    
    // Utility
    void reset();
    bool isDrawByRepetition() const;
    bool isDrawByFiftyMoves() const;
    uint64_t getHash() const { return hash; }
    
private:
    // Board representation
    Piece squares[64];
    Color stm;
    int castling;
    Square epSquare;
    int halfmoves;
    int fullmoves;
    
    // Position tracking
    uint64_t hash;
    std::vector<uint64_t> hashHistory;
    
    // Helper structures for unmake
    struct UndoInfo {
        Move move;
        Piece captured;
        int castling;
        Square epSquare;
        int halfmoves;
        uint64_t hash;
    };
    std::vector<UndoInfo> history;
    
    // Helper methods
    void clearSquare(Square s);
    void putPiece(Square s, Piece p);
    void movePiece(Square from, Square to);
    bool canCastle(int flag) const;
    void updateCastlingRights(Square from, Square to);
};

#endif // BOARD_H