#ifndef ATTACKS_H
#define ATTACKS_H

#include "types.h"
#include <array>

// Precomputed attack tables for fast move generation
namespace Attacks {

// Knight attack table - precomputed for all squares
extern std::array<uint64_t, 64> knightAttacks;

// King attack table - precomputed for all squares  
extern std::array<uint64_t, 64> kingAttacks;

// Pawn attack tables - separate for each color
extern std::array<uint64_t, 64> pawnAttacks[2];

// Sliding piece attack tables (will be computed)
extern std::array<std::array<uint64_t, 64>, 64> rookAttacks;
extern std::array<std::array<uint64_t, 64>, 64> bishopAttacks;

// Direction deltas for sliding pieces
constexpr int ROOK_DIRECTIONS[4] = {-8, -1, 1, 8};
constexpr int BISHOP_DIRECTIONS[4] = {-9, -7, 7, 9};

// Initialize all attack tables
void init();

// Fast attack lookups
inline uint64_t getKnightAttacks(Square sq) {
    return knightAttacks[sq];
}

inline uint64_t getKingAttacks(Square sq) {
    return kingAttacks[sq];
}

inline uint64_t getPawnAttacks(Square sq, Color c) {
    return pawnAttacks[c][sq];
}

// Sliding piece attacks with occupancy
uint64_t getRookAttacks(Square sq, uint64_t occupancy);
uint64_t getBishopAttacks(Square sq, uint64_t occupancy);
uint64_t getQueenAttacks(Square sq, uint64_t occupancy);

// Helper functions
inline bool isValidSquare(int sq) {
    return sq >= 0 && sq < 64;
}

inline uint64_t squareBB(Square sq) {
    return 1ULL << sq;
}

inline bool testBit(uint64_t bb, Square sq) {
    return bb & squareBB(sq);
}

inline uint64_t setBit(uint64_t bb, Square sq) {
    return bb | squareBB(sq);
}

inline uint64_t clearBit(uint64_t bb, Square sq) {
    return bb & ~squareBB(sq);
}

}

#endif // ATTACKS_H