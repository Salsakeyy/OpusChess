#include "board.h"
#include "utils.h"
#include <cctype>
#include <sstream>

// Zobrist keys for hashing
static uint64_t zobristPieces[12][64];
static uint64_t zobristCastling[16];
static uint64_t zobristEpFile[8];
static uint64_t zobristSideToMove;

// Initialize Zobrist keys
static void initZobrist() {
    static bool initialized = false;
    if (initialized) return;
    
    for (int p = 0; p < 12; ++p) {
        for (int s = 0; s < 64; ++s) {
            zobristPieces[p][s] = random64();
        }
    }
    for (int i = 0; i < 16; ++i) {
        zobristCastling[i] = random64();
    }
    for (int f = 0; f < 8; ++f) {
        zobristEpFile[f] = random64();
    }
    zobristSideToMove = random64();
    initialized = true;
}

Board::Board() {
    initZobrist();
    reset();
}

void Board::reset() {
    setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Board::setFromFEN(const std::string& fen) {
    // Clear board
    for (int i = 0; i < 64; ++i) {
        squares[i] = NO_PIECE;
    }
    
    std::vector<std::string> parts = split(fen);
    if (parts.size() < 4) return;
    
    // Parse pieces
    int rank = 7, file = 0;
    for (char c : parts[0]) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (isdigit(c)) {
            file += c - '0';
        } else {
            Piece p = NO_PIECE;
            switch (tolower(c)) {
                case 'p': p = islower(c) ? BLACK_PAWN : WHITE_PAWN; break;
                case 'n': p = islower(c) ? BLACK_KNIGHT : WHITE_KNIGHT; break;
                case 'b': p = islower(c) ? BLACK_BISHOP : WHITE_BISHOP; break;
                case 'r': p = islower(c) ? BLACK_ROOK : WHITE_ROOK; break;
                case 'q': p = islower(c) ? BLACK_QUEEN : WHITE_QUEEN; break;
                case 'k': p = islower(c) ? BLACK_KING : WHITE_KING; break;
            }
            if (p != NO_PIECE) {
                squares[makeSquare(file, rank)] = p;
                file++;
            }
        }
    }
    
    // Parse side to move
    stm = (parts[1] == "w") ? WHITE : BLACK;
    
    // Parse castling rights
    castling = 0;
    for (char c : parts[2]) {
        switch (c) {
            case 'K': castling |= WHITE_KINGSIDE; break;
            case 'Q': castling |= WHITE_QUEENSIDE; break;
            case 'k': castling |= BLACK_KINGSIDE; break;
            case 'q': castling |= BLACK_QUEENSIDE; break;
        }
    }
    
    // Parse en passant square
    if (parts[3] != "-") {
        int f = parts[3][0] - 'a';
        int r = parts[3][1] - '1';
        epSquare = makeSquare(f, r);
    } else {
        epSquare = -1;
    }
    
    // Parse move counters
    halfmoves = (parts.size() > 4) ? std::stoi(parts[4]) : 0;
    fullmoves = (parts.size() > 5) ? std::stoi(parts[5]) : 1;
    
    // Calculate hash
    hash = 0;
    for (int s = 0; s < 64; ++s) {
        if (squares[s] != NO_PIECE) {
            hash ^= zobristPieces[squares[s]][s];
        }
    }
    hash ^= zobristCastling[castling];
    if (epSquare != -1) {
        hash ^= zobristEpFile[fileOf(epSquare)];
    }
    if (stm == BLACK) {
        hash ^= zobristSideToMove;
    }
    
    // Clear history
    history.clear();
    hashHistory.clear();
    hashHistory.push_back(hash);
}

std::string Board::toFEN() const {
    std::stringstream ss;
    
    // Pieces
    for (int rank = 7; rank >= 0; --rank) {
        int emptyCount = 0;
        for (int file = 0; file < 8; ++file) {
            Square s = makeSquare(file, rank);
            Piece p = squares[s];
            
            if (p == NO_PIECE) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    ss << emptyCount;
                    emptyCount = 0;
                }
                
                char c = "PNBRQKpnbrqk"[p];
                ss << c;
            }
        }
        if (emptyCount > 0) {
            ss << emptyCount;
        }
        if (rank > 0) {
            ss << '/';
        }
    }
    
    // Side to move
    ss << ' ' << (stm == WHITE ? 'w' : 'b');
    
    // Castling rights
    ss << ' ';
    if (castling == 0) {
        ss << '-';
    } else {
        if (castling & WHITE_KINGSIDE) ss << 'K';
        if (castling & WHITE_QUEENSIDE) ss << 'Q';
        if (castling & BLACK_KINGSIDE) ss << 'k';
        if (castling & BLACK_QUEENSIDE) ss << 'q';
    }
    
    // En passant
    ss << ' ';
    if (epSquare == -1) {
        ss << '-';
    } else {
        ss << squareToString(epSquare);
    }
    
    // Move counters
    ss << ' ' << halfmoves << ' ' << fullmoves;
    
    return ss.str();
}

void Board::makeMove(Move m) {
    // Save undo info
    UndoInfo undo;
    undo.move = m;
    undo.captured = NO_PIECE;
    undo.castling = castling;
    undo.epSquare = epSquare;
    undo.halfmoves = halfmoves;
    undo.hash = hash;
    
    Square from = MoveUtils::from(m);
    Square to = MoveUtils::to(m);
    Piece moving = squares[from];
    Piece captured = squares[to];
    
    // Update hash for moving piece
    hash ^= zobristPieces[moving][from];
    hash ^= zobristPieces[moving][to];
    
    // Handle captures
    if (captured != NO_PIECE) {
        undo.captured = captured;
        hash ^= zobristPieces[captured][to];
    }
    
    // Move the piece
    squares[to] = moving;
    squares[from] = NO_PIECE;
    
    // Handle special moves
    if (MoveUtils::isCastle(m)) {
        // Move the rook
        if (to == G1) { // White kingside
            squares[F1] = WHITE_ROOK;
            squares[H1] = NO_PIECE;
            hash ^= zobristPieces[WHITE_ROOK][H1];
            hash ^= zobristPieces[WHITE_ROOK][F1];
        } else if (to == C1) { // White queenside
            squares[D1] = WHITE_ROOK;
            squares[A1] = NO_PIECE;
            hash ^= zobristPieces[WHITE_ROOK][A1];
            hash ^= zobristPieces[WHITE_ROOK][D1];
        } else if (to == G8) { // Black kingside
            squares[F8] = BLACK_ROOK;
            squares[H8] = NO_PIECE;
            hash ^= zobristPieces[BLACK_ROOK][H8];
            hash ^= zobristPieces[BLACK_ROOK][F8];
        } else if (to == C8) { // Black queenside
            squares[D8] = BLACK_ROOK;
            squares[A8] = NO_PIECE;
            hash ^= zobristPieces[BLACK_ROOK][A8];
            hash ^= zobristPieces[BLACK_ROOK][D8];
        }
    } else if (MoveUtils::isEnPassant(m)) {
        // Remove captured pawn
        Square captureSquare = makeSquare(fileOf(to), rankOf(from));
        undo.captured = squares[captureSquare];
        hash ^= zobristPieces[squares[captureSquare]][captureSquare];
        squares[captureSquare] = NO_PIECE;
    } else if (MoveUtils::isPromotion(m)) {
        // Replace pawn with promoted piece
        PieceType pt = MoveUtils::promotionType(m);
        Piece promoted = makePiece(stm, pt);
        squares[to] = promoted;
        hash ^= zobristPieces[moving][to];
        hash ^= zobristPieces[promoted][to];
    }
    
    // Update castling rights
    hash ^= zobristCastling[castling];
    updateCastlingRights(from, to);
    hash ^= zobristCastling[castling];
    
    // Update en passant square
    if (epSquare != -1) {
        hash ^= zobristEpFile[fileOf(epSquare)];
    }
    epSquare = -1;
    if (typeOf(moving) == PAWN && abs(rankOf(to) - rankOf(from)) == 2) {
        epSquare = makeSquare(fileOf(from), (rankOf(from) + rankOf(to)) / 2);
        hash ^= zobristEpFile[fileOf(epSquare)];
    }
    
    // Update move counters
    halfmoves++;
    if (typeOf(moving) == PAWN || captured != NO_PIECE) {
        halfmoves = 0;
    }
    if (stm == BLACK) {
        fullmoves++;
    }
    
    // Switch side to move
    stm = 1 - stm;
    hash ^= zobristSideToMove;
    
    // Save state
    history.push_back(undo);
    hashHistory.push_back(hash);
}

void Board::unmakeMove(Move m) {
    // Restore from history
    UndoInfo undo = history.back();
    history.pop_back();
    hashHistory.pop_back();
    
    // Switch side back
    stm = 1 - stm;
    
    // Restore simple fields
    castling = undo.castling;
    epSquare = undo.epSquare;
    halfmoves = undo.halfmoves;
    hash = undo.hash;
    
    if (stm == BLACK) {
        fullmoves--;
    }
    
    // Move piece back
    Square from = MoveUtils::from(m);
    Square to = MoveUtils::to(m);
    Piece moving = squares[to];
    
    // Handle promotion
    if (MoveUtils::isPromotion(m)) {
        moving = makePiece(stm, PAWN);
    }
    
    squares[from] = moving;
    squares[to] = undo.captured;
    
    // Handle special moves
    if (MoveUtils::isCastle(m)) {
        // Move the rook back
        if (to == G1) {
            squares[H1] = WHITE_ROOK;
            squares[F1] = NO_PIECE;
        } else if (to == C1) {
            squares[A1] = WHITE_ROOK;
            squares[D1] = NO_PIECE;
        } else if (to == G8) {
            squares[H8] = BLACK_ROOK;
            squares[F8] = NO_PIECE;
        } else if (to == C8) {
            squares[A8] = BLACK_ROOK;
            squares[D8] = NO_PIECE;
        }
    } else if (MoveUtils::isEnPassant(m)) {
        // Restore captured pawn
        Square captureSquare = makeSquare(fileOf(to), rankOf(from));
        squares[captureSquare] = undo.captured;
        squares[to] = NO_PIECE;
    }
}

void Board::updateCastlingRights(Square from, Square to) {
    // Remove rights if king or rook moves
    if (from == E1 || to == E1) {
        castling &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
    }
    if (from == E8 || to == E8) {
        castling &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
    }
    if (from == A1 || to == A1) {
        castling &= ~WHITE_QUEENSIDE;
    }
    if (from == H1 || to == H1) {
        castling &= ~WHITE_KINGSIDE;
    }
    if (from == A8 || to == A8) {
        castling &= ~BLACK_QUEENSIDE;
    }
    if (from == H8 || to == H8) {
        castling &= ~BLACK_KINGSIDE;
    }
}

Square Board::kingSquare(Color c) const {
    Piece king = makePiece(c, KING);
    for (int s = 0; s < 64; ++s) {
        if (squares[s] == king) {
            return s;
        }
    }
    return -1; // Should never happen
}

bool Board::isAttacked(Square s, Color by) const {
    // Check pawn attacks
    int pawnDir = (by == WHITE) ? 8 : -8;
    Piece pawn = makePiece(by, PAWN);
    
    // Pawn attacks from left and right
    for (int delta : {pawnDir - 1, pawnDir + 1}) {
        Square from = s - delta;
        if (from >= 0 && from < 64 && abs(fileOf(from) - fileOf(s)) == 1) {
            if (squares[from] == pawn) return true;
        }
    }
    
    // Knight attacks
    const int knightMoves[] = {-17, -15, -10, -6, 6, 10, 15, 17};
    Piece knight = makePiece(by, KNIGHT);
    for (int delta : knightMoves) {
        Square from = s + delta;
        if (from >= 0 && from < 64) {
            int fd = abs(fileOf(from) - fileOf(s));
            int rd = abs(rankOf(from) - rankOf(s));
            if (((fd == 2 && rd == 1) || (fd == 1 && rd == 2)) && 
                squares[from] == knight) {
                return true;
            }
        }
    }
    
    // Sliding pieces (bishop, rook, queen)
    const int bishopDirs[] = {-9, -7, 7, 9};
    const int rookDirs[] = {-8, -1, 1, 8};
    
    // Check bishop/queen diagonals
    Piece bishop = makePiece(by, BISHOP);
    Piece queen = makePiece(by, QUEEN);
    for (int dir : bishopDirs) {
        Square from = s;
        int lastFile = fileOf(from);
        while (true) {
            from += dir;
            if (from < 0 || from >= 64) break;
            
            // Check if we wrapped around the board
            int currentFile = fileOf(from);
            if (abs(currentFile - lastFile) != 1) break;
            lastFile = currentFile;
            
            Piece p = squares[from];
            if (p != NO_PIECE) {
                if (p == bishop || p == queen) return true;
                break;
            }
        }
    }
    
    // Check rook/queen lines  
    Piece rook = makePiece(by, ROOK);
    for (int dir : rookDirs) {
        Square from = s;
        while (true) {
            from += dir;
            if (from < 0 || from >= 64) break;
            
            // For horizontal moves, check we didn't wrap
            if (abs(dir) == 1) {
                if (rankOf(from) != rankOf(from - dir)) break;
            }
            
            Piece p = squares[from];
            if (p != NO_PIECE) {
                if (p == rook || p == queen) return true;
                break;
            }
        }
    }
    
    // King attacks
    const int kingDirs[] = {-9, -8, -7, -1, 1, 7, 8, 9};
    Piece king = makePiece(by, KING);
    for (int dir : kingDirs) {
        Square from = s + dir;
        if (from >= 0 && from < 64) {
            if (abs(fileOf(from) - fileOf(s)) <= 1 && 
                abs(rankOf(from) - rankOf(s)) <= 1 &&
                squares[from] == king) {
                return true;
            }
        }
    }
    
    return false;
}


bool Board::isInCheck(Color c) const {
    return isAttacked(kingSquare(c), 1 - c);
}

bool Board::isLegalMove(Move m) const {
    // Make the move on a copy and check if king is in check
    Board copy = *this;
    copy.makeMove(m);
    return !copy.isInCheck(1 - copy.sideToMove());
}

bool Board::isDrawByRepetition() const {
    if (hashHistory.size() < 9) return false;
    
    int count = 0;
    uint64_t currentHash = hashHistory.back();
    
    // Check for threefold repetition
    for (size_t i = hashHistory.size() - 3; i > 0; i -= 2) {
        if (hashHistory[i] == currentHash) {
            count++;
            if (count >= 2) return true;
        }
    }
    
    return false;
}

bool Board::isDrawByFiftyMoves() const {
    return halfmoves >= 100;
}