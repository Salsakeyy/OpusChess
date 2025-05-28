#include "movegen.h"

// Direction deltas for each piece type
static const int KING_DELTAS[] = {-9, -8, -7, -1, 1, 7, 8, 9};
static const int KNIGHT_DELTAS[] = {-17, -15, -10, -6, 6, 10, 15, 17};
static const int BISHOP_DELTAS[] = {-9, -7, 7, 9};
static const int ROOK_DELTAS[] = {-8, -1, 1, 8};

void MoveGenerator::generateMoves(const Board& board, std::vector<Move>& moves) {
    moves.clear();
    Color us = board.sideToMove();
    
    // Generate moves for all our pieces
    for (Square sq = 0; sq < 64; ++sq) {
        Piece p = board.pieceAt(sq);
        if (p == NO_PIECE || colorOf(p) != us) continue;
        
        switch (typeOf(p)) {
            case PAWN:
                generatePawnMoves(board, sq, moves);
                break;
            case KNIGHT:
                generateKnightMoves(board, sq, moves);
                break;
            case BISHOP:
                generateBishopMoves(board, sq, moves);
                break;
            case ROOK:
                generateRookMoves(board, sq, moves);
                break;
            case QUEEN:
                generateQueenMoves(board, sq, moves);
                break;
            case KING:
                generateKingMoves(board, sq, moves);
                break;
        }
    }
}

void MoveGenerator::generateCaptures(const Board& board, std::vector<Move>& moves) {
    moves.clear();
    Color us = board.sideToMove();
    Color them = 1 - us;
    
    // Generate captures for all our pieces
    for (Square sq = 0; sq < 64; ++sq) {
        Piece p = board.pieceAt(sq);
        if (p == NO_PIECE || colorOf(p) != us) continue;
        
        switch (typeOf(p)) {
            case PAWN: {
                int direction = (us == WHITE) ? 8 : -8;
                int promoRank = (us == WHITE) ? 6 : 1;
                
                // Pawn captures
                for (int delta : {direction - 1, direction + 1}) {
                    if (abs(delta) == 7 || abs(delta) == 9) {
                        Square to = sq + delta;
                        if (to >= 0 && to < 64 && abs(fileOf(sq) - fileOf(to)) == 1) {
                            Piece captured = board.pieceAt(to);
                            if (captured != NO_PIECE && colorOf(captured) == them) {
                                if (rankOf(sq) == promoRank) {
                                    for (PieceType pt = QUEEN; pt >= KNIGHT; --pt) {
                                        Move m = MoveUtils::makePromotion(sq, to, pt);
                                        moves.push_back(m | MOVE_CAPTURE);
                                    }
                                } else {
                                    moves.push_back(MoveUtils::makeMove(sq, to, MOVE_CAPTURE));
                                }
                            }
                        }
                    }
                }
                
                // En passant
                Square ep = board.enPassantSquare();
                if (ep != -1 && abs(fileOf(sq) - fileOf(ep)) == 1 && 
                    rankOf(ep) == rankOf(sq) + (us == WHITE ? 1 : -1)) {
                    moves.push_back(MoveUtils::makeMove(sq, ep, MOVE_EN_PASSANT));
                }
                break;
            }
            
            case KNIGHT: {
                const int deltas[] = {-17, -15, -10, -6, 6, 10, 15, 17};
                for (int delta : deltas) {
                    Square to = sq + delta;
                    if (to < 0 || to >= 64) continue;
                    int fd = abs(fileOf(sq) - fileOf(to));
                    int rd = abs(rankOf(sq) - rankOf(to));
                    if ((fd == 2 && rd == 1) || (fd == 1 && rd == 2)) {
                        Piece target = board.pieceAt(to);
                        if (target != NO_PIECE && colorOf(target) == them) {
                            moves.push_back(MoveUtils::makeMove(sq, to, MOVE_CAPTURE));
                        }
                    }
                }
                break;
            }
            
            case BISHOP:
            case ROOK:
            case QUEEN: {
                const int* deltas;
                int numDeltas;
                if (typeOf(p) == BISHOP) {
                    static const int bd[] = {-9, -7, 7, 9};
                    deltas = bd;
                    numDeltas = 4;
                } else if (typeOf(p) == ROOK) {
                    static const int rd[] = {-8, -1, 1, 8};
                    deltas = rd;
                    numDeltas = 4;
                } else { // QUEEN
                    static const int qd[] = {-9, -8, -7, -1, 1, 7, 8, 9};
                    deltas = qd;
                    numDeltas = 8;
                }
                
                for (int i = 0; i < numDeltas; ++i) {
                    int delta = deltas[i];
                    Square to = sq;
                    
                    while (true) {
                        to += delta;
                        if (to < 0 || to >= 64) break;
                        int fileDiff = abs(fileOf(to) - fileOf(to - delta));
                        if (fileDiff > 1) break;
                        
                        Piece target = board.pieceAt(to);
                        if (target != NO_PIECE) {
                            if (colorOf(target) == them) {
                                moves.push_back(MoveUtils::makeMove(sq, to, MOVE_CAPTURE));
                            }
                            break;
                        }
                    }
                }
                break;
            }
            
            case KING: {
                const int deltas[] = {-9, -8, -7, -1, 1, 7, 8, 9};
                for (int delta : deltas) {
                    Square to = sq + delta;
                    if (to < 0 || to >= 64) continue;
                    if (abs(fileOf(sq) - fileOf(to)) <= 1 && 
                        abs(rankOf(sq) - rankOf(to)) <= 1) {
                        Piece target = board.pieceAt(to);
                        if (target != NO_PIECE && colorOf(target) == them) {
                            moves.push_back(MoveUtils::makeMove(sq, to, MOVE_CAPTURE));
                        }
                    }
                }
                break;
            }
        }
    }
}

void MoveGenerator::generateLegalMoves(const Board& board, std::vector<Move>& moves) {
    std::vector<Move> pseudoLegal;
    generateMoves(board, pseudoLegal);
    
    moves.clear();
    for (Move m : pseudoLegal) {
        if (board.isLegalMove(m)) {
            moves.push_back(m);
        }
    }
}

void MoveGenerator::generatePawnMoves(const Board& board, Square from, std::vector<Move>& moves) {
    Color us = board.sideToMove();
    int direction = (us == WHITE) ? 8 : -8;
    int startRank = (us == WHITE) ? 1 : 6;
    int promoRank = (us == WHITE) ? 6 : 1;
    
    // One square forward
    Square to = from + direction;
    if (to >= 0 && to < 64 && board.pieceAt(to) == NO_PIECE) {
        if (rankOf(from) == promoRank) {
            // Add all promotions
            for (PieceType pt = QUEEN; pt >= KNIGHT; --pt) {
                moves.push_back(MoveUtils::makePromotion(from, to, pt));
            }
        } else {
            addMove(board, from, to, moves);
            
            // Two squares forward from starting position
            if (rankOf(from) == startRank) {
                Square to2 = from + 2 * direction;
                if (board.pieceAt(to2) == NO_PIECE) {
                    addMove(board, from, to2, moves);
                }
            }
        }
    }
    
    // Captures
    for (int delta : {direction - 1, direction + 1}) {
        if (abs(delta) == 7 || abs(delta) == 9) {
            Square captureSquare = from + delta;
            if (captureSquare >= 0 && captureSquare < 64) {
                // Check if we're not wrapping around the board
                if (abs(fileOf(from) - fileOf(captureSquare)) == 1) {
                    Piece captured = board.pieceAt(captureSquare);
                    if (captured != NO_PIECE && colorOf(captured) != us) {
                        if (rankOf(from) == promoRank) {
                            // Capture with promotion
                            for (PieceType pt = QUEEN; pt >= KNIGHT; --pt) {
                                Move m = MoveUtils::makePromotion(from, captureSquare, pt);
                                moves.push_back(m | MOVE_CAPTURE);
                            }
                        } else {
                            addMove(board, from, captureSquare, moves, MOVE_CAPTURE);
                        }
                    }
                }
            }
        }
    }
    
    // En passant
    Square ep = board.enPassantSquare();
    if (ep != -1) {
        if (abs(fileOf(from) - fileOf(ep)) == 1 && rankOf(ep) == rankOf(from) + (us == WHITE ? 1 : -1)) {
            addMove(board, from, ep, moves, MOVE_EN_PASSANT);
        }
    }
}

void MoveGenerator::generateKnightMoves(const Board& board, Square from, std::vector<Move>& moves) {
    for (int delta : KNIGHT_DELTAS) {
        Square to = from + delta;
        if (to < 0 || to >= 64) continue;
        
        // Check for wrapping
        int fileDiff = abs(fileOf(from) - fileOf(to));
        int rankDiff = abs(rankOf(from) - rankOf(to));
        if ((fileDiff == 2 && rankDiff == 1) || (fileDiff == 1 && rankDiff == 2)) {
            Piece target = board.pieceAt(to);
            if (target == NO_PIECE) {
                addMove(board, from, to, moves);
            } else if (colorOf(target) != board.sideToMove()) {
                addMove(board, from, to, moves, MOVE_CAPTURE);
            }
        }
    }
}

void MoveGenerator::generateBishopMoves(const Board& board, Square from, std::vector<Move>& moves) {
    generateSlidingMoves(board, from, BISHOP_DELTAS, 4, moves);
}

void MoveGenerator::generateRookMoves(const Board& board, Square from, std::vector<Move>& moves) {
    generateSlidingMoves(board, from, ROOK_DELTAS, 4, moves);
}

void MoveGenerator::generateQueenMoves(const Board& board, Square from, std::vector<Move>& moves) {
    generateBishopMoves(board, from, moves);
    generateRookMoves(board, from, moves);
}

void MoveGenerator::generateKingMoves(const Board& board, Square from, std::vector<Move>& moves) {
    Color us = board.sideToMove();
    
    // Normal moves
    for (int delta : KING_DELTAS) {
        Square to = from + delta;
        if (to < 0 || to >= 64) continue;
        
        // Check for wrapping
        int fileDiff = abs(fileOf(from) - fileOf(to));
        int rankDiff = abs(rankOf(from) - rankOf(to));
        if (fileDiff <= 1 && rankDiff <= 1) {
            Piece target = board.pieceAt(to);
            if (target == NO_PIECE) {
                addMove(board, from, to, moves);
            } else if (colorOf(target) != us) {
                addMove(board, from, to, moves, MOVE_CAPTURE);
            }
        }
    }
    
    // Castling
    if (!board.isInCheck(us)) {
        if (us == WHITE) {
            // White kingside
            if ((board.castlingRights() & WHITE_KINGSIDE) &&
                board.pieceAt(F1) == NO_PIECE &&
                board.pieceAt(G1) == NO_PIECE &&
                !board.isAttacked(F1, BLACK)) {
                addMove(board, from, G1, moves, MOVE_CASTLE);
            }
            // White queenside
            if ((board.castlingRights() & WHITE_QUEENSIDE) &&
                board.pieceAt(D1) == NO_PIECE &&
                board.pieceAt(C1) == NO_PIECE &&
                board.pieceAt(B1) == NO_PIECE &&
                !board.isAttacked(D1, BLACK)) {
                addMove(board, from, C1, moves, MOVE_CASTLE);
            }
        } else {
            // Black kingside
            if ((board.castlingRights() & BLACK_KINGSIDE) &&
                board.pieceAt(F8) == NO_PIECE &&
                board.pieceAt(G8) == NO_PIECE &&
                !board.isAttacked(F8, WHITE)) {
                addMove(board, from, G8, moves, MOVE_CASTLE);
            }
            // Black queenside
            if ((board.castlingRights() & BLACK_QUEENSIDE) &&
                board.pieceAt(D8) == NO_PIECE &&
                board.pieceAt(C8) == NO_PIECE &&
                board.pieceAt(B8) == NO_PIECE &&
                !board.isAttacked(D8, WHITE)) {
                addMove(board, from, C8, moves, MOVE_CASTLE);
            }
        }
    }
}

void MoveGenerator::generateSlidingMoves(const Board& board, Square from, 
                                       const int* deltas, int numDeltas, 
                                       std::vector<Move>& moves) {
    Color us = board.sideToMove();
    
    for (int i = 0; i < numDeltas; ++i) {
        int delta = deltas[i];
        Square to = from;
        
        while (true) {
            to += delta;
            if (to < 0 || to >= 64) break;
            
            // Check for wrapping
            int fileDiff = abs(fileOf(to) - fileOf(to - delta));
            if (fileDiff > 1) break;
            
            Piece target = board.pieceAt(to);
            if (target == NO_PIECE) {
                addMove(board, from, to, moves);
            } else {
                if (colorOf(target) != us) {
                    addMove(board, from, to, moves, MOVE_CAPTURE);
                }
                break;
            }
        }
    }
}

void MoveGenerator::addMove(const Board& board, Square from, Square to, 
                           std::vector<Move>& moves, uint16_t flags) {
    moves.push_back(MoveUtils::makeMove(from, to, flags));
}