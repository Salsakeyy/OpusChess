#include "search.h"
#include "movegen.h"
#include "evaluation.h"
#include <algorithm>
#include <iostream>

constexpr Score MATE_SCORE = 30000;
constexpr Score DRAW_SCORE = 0;

Search::Search(Board& board) : board(board) {
    clearTT();
}

Move Search::think(int depth, int timeLimit, bool infinite) {
    info.depth = 0;
    info.seldepth = 0;
    info.nodes = 0;
    info.score = 0;
    info.pv.clear();
    info.startTime = std::chrono::steady_clock::now();
    info.timeLimit = timeLimit;
    info.infinite = infinite;
    info.stop = false;
    
    Move bestMove = 0;
    Score alpha = -MATE_SCORE;
    Score beta = MATE_SCORE;
    
    // Iterative deepening
    for (int d = 1; d <= depth && !info.stop; ++d) {
        info.depth = d;
        info.seldepth = 0;
        info.pv.clear();
        
        Score score = alphaBeta(d, alpha, beta);
        
        if (!info.stop) {
            info.score = score;
            if (!info.pv.empty()) {
                bestMove = info.pv[0];
            }
            
            // Print info
            auto elapsed = std::chrono::steady_clock::now() - info.startTime;
            int ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            
            std::cout << "info depth " << d;
            std::cout << " seldepth " << info.seldepth;
            std::cout << " score cp " << score;
            std::cout << " nodes " << info.nodes;
            std::cout << " time " << ms;
            if (ms > 0) {
                std::cout << " nps " << (info.nodes * 1000 / ms);
            }
            
            if (!info.pv.empty()) {
                std::cout << " pv";
                for (Move m : info.pv) {
                    std::cout << " " << MoveUtils::toString(m);
                }
            }
            std::cout << std::endl;
        }
        
        // Check time
        if (timeUp()) break;
    }
    
    return bestMove;
}

Score Search::alphaBeta(int depth, Score alpha, Score beta) {
    if (info.stop) return 0;
    
    info.nodes++;
    info.seldepth = std::max(info.seldepth, info.depth - depth);
    
    // Check for time
    if ((info.nodes & 2047) == 0 && timeUp()) {
        info.stop = true;
        return 0;
    }
    
    // Terminal node checks
    if (board.isDrawByRepetition() || board.isDrawByFiftyMoves()) {
        return DRAW_SCORE;
    }
    
    // Probe transposition table
    uint64_t hash = board.getHash();
    TTEntry* ttEntry = probeTT(hash);
    if (ttEntry && ttEntry->depth >= depth) {
        if (ttEntry->type == TTEntry::EXACT) {
            return ttEntry->score;
        } else if (ttEntry->type == TTEntry::LOWER && ttEntry->score >= beta) {
            return ttEntry->score;
        } else if (ttEntry->type == TTEntry::UPPER && ttEntry->score <= alpha) {
            return ttEntry->score;
        }
    }
    
    // Quiescence search at leaf nodes
    if (depth <= 0) {
        return quiescence(alpha, beta);
    }
    
    // Generate moves
    std::vector<Move> moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    // Checkmate or stalemate
    if (moves.empty()) {
        if (board.isInCheck(board.sideToMove())) {
            return -MATE_SCORE + (info.depth - depth); // Checkmate
        } else {
            return DRAW_SCORE; // Stalemate
        }
    }
    
    // Order moves
    orderMoves(moves);
    
    Move bestMove = 0;
    Score bestScore = -MATE_SCORE;
    int searchedMoves = 0;
    
    for (Move m : moves) {
        board.makeMove(m);
        
        Score score;
        if (searchedMoves == 0) {
            // First move - full window search
            score = -alphaBeta(depth - 1, -beta, -alpha);
        } else {
            // Late moves - null window search
            score = -alphaBeta(depth - 1, -alpha - 1, -alpha);
            if (score > alpha && score < beta) {
                // Re-search with full window
                score = -alphaBeta(depth - 1, -beta, -alpha);
            }
        }
        
        board.unmakeMove(m);
        searchedMoves++;
        
        if (info.stop) return 0;
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
            
            if (score > alpha) {
                alpha = score;
                updatePV(m);
                
                if (score >= beta) {
                    // Beta cutoff
                    storeTT(hash, m, score, depth, TTEntry::LOWER);
                    return score;
                }
            }
        }
    }
    
    // Store in transposition table
    if (bestScore <= alpha) {
        storeTT(hash, bestMove, bestScore, depth, TTEntry::UPPER);
    } else {
        storeTT(hash, bestMove, bestScore, depth, TTEntry::EXACT);
    }
    
    return bestScore;
}

Score Search::quiescence(Score alpha, Score beta) {
    if (info.stop) return 0;
    
    info.nodes++;
    
    // Stand pat score
    Score standPat = Evaluator::evaluate(board);
    
    if (standPat >= beta) {
        return beta;
    }
    
    if (standPat > alpha) {
        alpha = standPat;
    }
    
    // Generate only captures
    std::vector<Move> moves;
    MoveGenerator::generateCaptures(board, moves);
    
    // Order captures by MVV-LVA
    std::sort(moves.begin(), moves.end(), [this](Move a, Move b) {
        return mvvLva(a) > mvvLva(b);
    });
    
    for (Move m : moves) {
        board.makeMove(m);
        Score score = -quiescence(-beta, -alpha);
        board.unmakeMove(m);
        
        if (info.stop) return 0;
        
        if (score >= beta) {
            return beta;
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    return alpha;
}

void Search::orderMoves(std::vector<Move>& moves) {
    // Simple move ordering
    // 1. TT move
    // 2. Captures ordered by MVV-LVA
    // 3. Non-captures
    
    std::sort(moves.begin(), moves.end(), [this](Move a, Move b) {
        // TT move first
        TTEntry* tt = probeTT(board.getHash());
        if (tt) {
            if (a == tt->bestMove) return true;
            if (b == tt->bestMove) return false;
        }
        
        // Then captures
        bool aCapture = MoveUtils::isCapture(a);
        bool bCapture = MoveUtils::isCapture(b);
        
        if (aCapture && !bCapture) return true;
        if (!aCapture && bCapture) return false;
        
        if (aCapture && bCapture) {
            return mvvLva(a) > mvvLva(b);
        }
        
        // Non-captures - could add history heuristic here
        return false;
    });
}

Score Search::mvvLva(Move m) {
    // Most Valuable Victim - Least Valuable Attacker
    if (!MoveUtils::isCapture(m)) return 0;
    
    Square from = MoveUtils::from(m);
    Square to = MoveUtils::to(m);
    
    Piece attacker = board.pieceAt(from);
    Piece victim = board.pieceAt(to);
    
    if (victim == NO_PIECE) {
        // En passant
        victim = makePiece(1 - board.sideToMove(), PAWN);
    }
    
    // Victim value - attacker value
    static const Score pieceValues[] = {100, 320, 330, 500, 900, 10000};
    return pieceValues[typeOf(victim)] - pieceValues[typeOf(attacker)] / 10;
}

void Search::updatePV(Move m) {
    // Update principal variation
    std::vector<Move> newPV;
    newPV.push_back(m);
    
    // Add moves from the next ply's PV
    // This is a simplified version - proper implementation would
    // maintain a triangular PV array
    
    info.pv = newPV;
}

bool Search::timeUp() const {
    if (info.infinite) return false;
    if (info.timeLimit == 0) return false;
    
    auto elapsed = std::chrono::steady_clock::now() - info.startTime;
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    
    return ms >= info.timeLimit;
}

void Search::clearTT() {
    for (size_t i = 0; i < TT_SIZE; ++i) {
        tt[i] = TTEntry{0, 0, 0, 0, TTEntry::EXACT};
    }
}

Search::TTEntry* Search::probeTT(uint64_t hash) {
    size_t index = hash % TT_SIZE;
    if (tt[index].hash == hash) {
        return &tt[index];
    }
    return nullptr;
}

void Search::storeTT(uint64_t hash, Move m, Score score, int depth, int type) {
    size_t index = hash % TT_SIZE;
    
    // Simple replacement strategy - always replace
    tt[index].hash = hash;
    tt[index].bestMove = m;
    tt[index].score = score;
    tt[index].depth = depth;
    tt[index].type = static_cast<TTEntry::Type>(type);
}
