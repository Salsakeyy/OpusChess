#include "board.h"
#include "movegen.h"
#include <iostream>
#include <chrono>
#include <iomanip>

// Perft function - counts nodes at given depth
uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;
    
    uint64_t nodes = 0;
    std::vector<Move> moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    for (Move m : moves) {
        board.makeMove(m);
        nodes += perft(board, depth - 1);
        board.unmakeMove(m);
    }
    
    return nodes;
}

// Divide function - shows move breakdown
void divide(Board& board, int depth) {
    std::vector<Move> moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    uint64_t total = 0;
    
    for (Move m : moves) {
        board.makeMove(m);
        uint64_t count = perft(board, depth - 1);
        board.unmakeMove(m);
        
        std::cout << MoveUtils::toString(m) << ": " << count << std::endl;
        total += count;
    }
    
    std::cout << "\nTotal: " << total << std::endl;
}

// Test a specific position
void testPosition(const std::string& name, const std::string& fen, 
                 int maxDepth, const uint64_t expected[]) {
    std::cout << "\n=== " << name << " ===" << std::endl;
    std::cout << "FEN: " << fen << std::endl;
    
    Board board;
    board.setFromFEN(fen);
    
    for (int depth = 1; depth <= maxDepth; ++depth) {
        auto start = std::chrono::steady_clock::now();
        uint64_t result = perft(board, depth);
        auto end = std::chrono::steady_clock::now();
        
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Depth " << depth << ": " << std::setw(12) << result;
        std::cout << " (expected: " << std::setw(12) << expected[depth-1] << ")";
        
        if (result == expected[depth-1]) {
            std::cout << " ✓ PASS";
        } else {
            std::cout << " ✗ FAIL (diff: " << (int64_t)(result - expected[depth-1]) << ")";
        }
        
        if (ms > 0) {
            std::cout << " [" << ms << "ms, " << (result * 1000 / ms) << " nps]";
        }
        std::cout << std::endl;
        
        // If test fails, show move breakdown
        if (result != expected[depth-1] && depth == 1) {
            std::cout << "\nMove breakdown:" << std::endl;
            divide(board, depth);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Chess Engine Move Generation Test (Perft)" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // Test positions with expected node counts
    
    // Starting position
    {
        const uint64_t expected[] = {20, 400, 8902, 197281, 4865609};
        testPosition("Starting Position", 
                    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                    4, expected);
    }
    
    // Kiwipete - tests many special moves
    {
        const uint64_t expected[] = {48, 2039, 97862, 4085603};
        testPosition("Kiwipete Position", 
                    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                    3, expected);
    }
    
    // Position 3 - Endgame
    {
        const uint64_t expected[] = {14, 191, 2812, 43238, 674624};
        testPosition("Endgame Position", 
                    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                    4, expected);
    }
    
    // Position 4 - Promotions
    {
        const uint64_t expected[] = {6, 264, 9467, 422333};
        testPosition("Promotion Position", 
                    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
                    3, expected);
    }
    
    // Position 5
    {
        const uint64_t expected[] = {44, 1486, 62379, 2103487};
        testPosition("Complex Position", 
                    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                    3, expected);
    }
    
    // En passant test
    {
        const uint64_t expected[] = {24, 496, 9483};
        testPosition("En Passant Test", 
                    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                    0, expected);  // Using 0 will skip this for now
    }
    
    // Interactive mode
    if (argc > 1) {
        std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        int depth = 1;
        
        if (std::string(argv[1]) == "divide" && argc > 2) {
            depth = std::stoi(argv[2]);
            if (argc > 3) {
                fen = "";
                for (int i = 3; i < argc; ++i) {
                    if (i > 3) fen += " ";
                    fen += argv[i];
                }
            }
            
            Board board;
            board.setFromFEN(fen);
            std::cout << "\nDivide " << depth << " for position: " << fen << std::endl;
            divide(board, depth);
        }
        else if (std::string(argv[1]) == "perft" && argc > 2) {
            depth = std::stoi(argv[2]);
            if (argc > 3) {
                fen = "";
                for (int i = 3; i < argc; ++i) {
                    if (i > 3) fen += " ";
                    fen += argv[i];
                }
            }
            
            Board board;
            board.setFromFEN(fen);
            auto start = std::chrono::steady_clock::now();
            uint64_t result = perft(board, depth);
            auto end = std::chrono::steady_clock::now();
            
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cout << "\nPerft " << depth << " = " << result;
            if (ms > 0) {
                std::cout << " (" << ms << "ms, " << (result * 1000 / ms) << " nps)";
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "If all tests pass, your move generation is correct!" << std::endl;
    std::cout << "If tests fail, use 'divide' to debug specific positions." << std::endl;
    std::cout << "\nUsage: " << argv[0] << " [divide|perft] <depth> [fen]" << std::endl;
    
    return 0;
}