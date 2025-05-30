#include "attacks.h"
#include <iostream>

namespace Attacks {

// Precomputed attack tables
std::array<uint64_t, 64> knightAttacks;
std::array<uint64_t, 64> kingAttacks;
std::array<uint64_t, 64> pawnAttacks[2];
std::array<std::array<uint64_t, 64>, 64> rookAttacks;
std::array<std::array<uint64_t, 64>, 64> bishopAttacks;

// Knight move deltas
constexpr int KNIGHT_DELTAS[8] = {-17, -15, -10, -6, 6, 10, 15, 17};

// King move deltas  
constexpr int KING_DELTAS[8] = {-9, -8, -7, -1, 1, 7, 8, 9};

void init() {
    // Initialize knight attacks
    for (Square sq = 0; sq < 64; ++sq) {
        uint64_t attacks = 0;
        int file = fileOf(sq);
        int rank = rankOf(sq);
        
        for (int delta : KNIGHT_DELTAS) {
            int to = sq + delta;
            if (!isValidSquare(to)) continue;
            
            int toFile = fileOf(to);
            int toRank = rankOf(to);
            int fileDiff = abs(file - toFile);
            int rankDiff = abs(rank - toRank);
            
            // Valid knight move: L-shape (2+1 or 1+2)
            if ((fileDiff == 2 && rankDiff == 1) || (fileDiff == 1 && rankDiff == 2)) {
                attacks = setBit(attacks, to);
            }
        }
        knightAttacks[sq] = attacks;
    }
    
    // Initialize king attacks
    for (Square sq = 0; sq < 64; ++sq) {
        uint64_t attacks = 0;
        int file = fileOf(sq);
        int rank = rankOf(sq);
        
        for (int delta : KING_DELTAS) {
            int to = sq + delta;
            if (!isValidSquare(to)) continue;
            
            int toFile = fileOf(to);
            int toRank = rankOf(to);
            int fileDiff = abs(file - toFile);
            int rankDiff = abs(rank - toRank);
            
            // Valid king move: max 1 square in any direction
            if (fileDiff <= 1 && rankDiff <= 1) {
                attacks = setBit(attacks, to);
            }
        }
        kingAttacks[sq] = attacks;
    }
    
    // Initialize pawn attacks
    for (Square sq = 0; sq < 64; ++sq) {
        int file = fileOf(sq);
        int rank = rankOf(sq);
        
        // White pawn attacks (moving up)
        uint64_t whiteAttacks = 0;
        if (rank < 7) { // Not on 8th rank
            if (file > 0) { // Can capture left
                whiteAttacks = setBit(whiteAttacks, sq + 7);
            }
            if (file < 7) { // Can capture right
                whiteAttacks = setBit(whiteAttacks, sq + 9);
            }
        }
        pawnAttacks[WHITE][sq] = whiteAttacks;
        
        // Black pawn attacks (moving down)
        uint64_t blackAttacks = 0;
        if (rank > 0) { // Not on 1st rank
            if (file > 0) { // Can capture left
                blackAttacks = setBit(blackAttacks, sq - 9);
            }
            if (file < 7) { // Can capture right
                blackAttacks = setBit(blackAttacks, sq - 7);
            }
        }
        pawnAttacks[BLACK][sq] = blackAttacks;
    }
    
    // Initialize sliding piece attacks (simplified - can be optimized with magic bitboards later)
    for (Square sq = 0; sq < 64; ++sq) {
        for (int occupancyIndex = 0; occupancyIndex < 64; ++occupancyIndex) {
            uint64_t occupancy = 0;
            // Create a simple occupancy pattern for now
            for (int i = 0; i < 6; ++i) {
                if (occupancyIndex & (1 << i)) {
                    occupancy = setBit(occupancy, (sq + i * 8 + i) % 64);
                }
            }
            
            // Compute rook attacks
            uint64_t rookAtt = 0;
            for (int dir : ROOK_DIRECTIONS) {
                for (int i = 1; i < 8; ++i) {
                    int to = sq + dir * i;
                    if (!isValidSquare(to)) break;
                    
                    // Check for rank/file wrap
                    if (dir == 1 || dir == -1) { // Horizontal
                        if (rankOf(to) != rankOf(sq)) break;
                    }
                    
                    rookAtt = setBit(rookAtt, to);
                    if (testBit(occupancy, to)) break; // Blocked
                }
            }
            rookAttacks[sq][occupancyIndex] = rookAtt;
            
            // Compute bishop attacks
            uint64_t bishopAtt = 0;
            for (int dir : BISHOP_DIRECTIONS) {
                for (int i = 1; i < 8; ++i) {
                    int to = sq + dir * i;
                    if (!isValidSquare(to)) break;
                    
                    // Check for diagonal wrap
                    int fileDiff = abs(fileOf(to) - fileOf(sq));
                    int rankDiff = abs(rankOf(to) - rankOf(sq));
                    if (fileDiff != i || rankDiff != i) break;
                    
                    bishopAtt = setBit(bishopAtt, to);
                    if (testBit(occupancy, to)) break; // Blocked
                }
            }
            bishopAttacks[sq][occupancyIndex] = bishopAtt;
        }
    }
}

uint64_t getRookAttacks(Square sq, uint64_t occupancy) {
    // Simplified implementation - use a hash of occupancy for now
    int index = occupancy % 64;
    return rookAttacks[sq][index];
}

uint64_t getBishopAttacks(Square sq, uint64_t occupancy) {
    // Simplified implementation - use a hash of occupancy for now  
    int index = occupancy % 64;
    return bishopAttacks[sq][index];
}

uint64_t getQueenAttacks(Square sq, uint64_t occupancy) {
    return getRookAttacks(sq, occupancy) | getBishopAttacks(sq, occupancy);
}

}