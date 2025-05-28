#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>

// String utilities
inline std::vector<std::string> split(const std::string& str, char delimiter = ' ') {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// Bit manipulation utilities
inline int popcount(uint64_t b) {
    return __builtin_popcountll(b);
}

inline int lsb(uint64_t b) {
    return __builtin_ctzll(b);
}

inline int msb(uint64_t b) {
    return 63 - __builtin_clzll(b);
}

// Random number generation (for Zobrist hashing)
inline uint64_t random64() {
    static uint64_t seed = 1070372ull;
    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;
    return seed * 2685821657736338717ull;
}

#endif // UTILS_H