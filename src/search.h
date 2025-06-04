#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include "move.h"
#include "board.h"
#include <vector>
#include <chrono>
#include <atomic>

struct SearchInfo {
    int depth = 0;
    int seldepth = 0;
    uint64_t nodes = 0;
    Score score = 0;
    std::vector<Move> pv;
    std::chrono::steady_clock::time_point startTime;
    int timeLimit = 0; // milliseconds
    bool infinite = false;
    std::atomic<bool> stop{false};
};

class Search {
public:
    Search(Board& board);
    
    // Main search function
    Move think(int depth, int timeLimit = 0, bool infinite = false);
    
    // Stop the search
    void stopSearch() { info.stop = true; }
    
    // Get search info
    const SearchInfo& getInfo() const { return info; }
    
private:
    Board& board;
    SearchInfo info;
    
    // Search algorithms
    Score alphaBeta(int depth, Score alpha, Score beta, bool canNull = true);
    Score quiescence(Score alpha, Score beta);
    
    // Move ordering
    void orderMoves(std::vector<Move>& moves);
    Score mvvLva(Move m);
    
    // Transposition table (simple implementation)
    struct TTEntry {
        uint64_t hash;
        Move bestMove;
        Score score;
        int depth;
        enum Type { EXACT, LOWER, UPPER } type;
    };
    static constexpr size_t TT_SIZE = 1 << 20; // 1M entries
    TTEntry tt[TT_SIZE];
    
    // Helper methods
    void updatePV(Move m);
    bool timeUp() const;
    void clearTT();
    TTEntry* probeTT(uint64_t hash);
    void storeTT(uint64_t hash, Move m, Score score, int depth, int type);
};

#endif // SEARCH_H
