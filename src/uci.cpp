#include "uci.h"
#include "movegen.h"
#include "utils.h"
#include <iostream>
#include <sstream>

UCI::UCI() : search(nullptr) {
    board.reset();
}

UCI::~UCI() {
    if (searchThread.joinable()) {
        searchThread.join();
    }
    delete search;
}

void UCI::loop() {
    std::string line, command;
    
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        iss >> command;
        
        if (command == "uci") {
            handleUCI();
        } else if (command == "isready") {
            handleIsReady();
        } else if (command == "ucinewgame") {
            handleUCINewGame();
        } else if (command == "position") {
            handlePosition(line);
        } else if (command == "go") {
            handleGo(line);
        } else if (command == "stop") {
            handleStop();
        } else if (command == "quit") {
            handleQuit();
            break;
        } else if (command == "setoption") {
            handleSetOption(line);
        } else if (command == "d") {
            // Debug: display board
            std::cout << board.toFEN() << std::endl;
        }
    }
}

void UCI::handleUCI() {
    std::cout << "id name SimpleChessEngine" << std::endl;
    std::cout << "id author YourName" << std::endl;
    
    // Options can be added here
    std::cout << "option name Hash type spin default 64 min 1 max 1024" << std::endl;
    
    std::cout << "uciok" << std::endl;
}

void UCI::handleIsReady() {
    std::cout << "readyok" << std::endl;
}

void UCI::handleUCINewGame() {
    board.reset();
    if (search) {
        delete search;
        search = nullptr;
    }
}

void UCI::handlePosition(const std::string& line) {
    std::vector<std::string> tokens = split(line);
    
    size_t idx = 1;
    if (idx < tokens.size() && tokens[idx] == "startpos") {
        board.reset();
        idx++;
    } else if (idx < tokens.size() && tokens[idx] == "fen") {
        idx++;
        std::string fen;
        while (idx < tokens.size() && tokens[idx] != "moves") {
            fen += tokens[idx] + " ";
            idx++;
        }
        board.setFromFEN(fen);
    }
    
    // Process moves
    if (idx < tokens.size() && tokens[idx] == "moves") {
        idx++;
        while (idx < tokens.size()) {
            Move m = parseMove(tokens[idx]);
            if (m != 0) {
                board.makeMove(m);
            }
            idx++;
        }
    }
}

void UCI::handleGo(const std::string& line) {
    if (searchThread.joinable()) {
        searchThread.join();
    }
    
    std::vector<std::string> tokens = split(line);
    int depth = 64; // Default max depth
    int timeLimit = 0;
    bool infinite = false;
    
    // Time management variables
    int myTime = 0;
    int myInc = 0;
    int opTime = 0;
    int opInc = 0;
    int movesToGo = 40; // Assume 40 moves to time control if not specified
    
    for (size_t i = 1; i < tokens.size(); i++) {
        if (tokens[i] == "depth" && i + 1 < tokens.size()) {
            depth = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "infinite") {
            infinite = true;
        } else if (tokens[i] == "movetime" && i + 1 < tokens.size()) {
            timeLimit = std::stoi(tokens[i + 1]);
        } else if (tokens[i] == "wtime" && i + 1 < tokens.size()) {
            int wtime = std::stoi(tokens[i + 1]);
            if (board.sideToMove() == WHITE) {
                myTime = wtime;
            } else {
                opTime = wtime;
            }
        } else if (tokens[i] == "btime" && i + 1 < tokens.size()) {
            int btime = std::stoi(tokens[i + 1]);
            if (board.sideToMove() == BLACK) {
                myTime = btime;
            } else {
                opTime = btime;
            }
        } else if (tokens[i] == "winc" && i + 1 < tokens.size()) {
            int winc = std::stoi(tokens[i + 1]);
            if (board.sideToMove() == WHITE) {
                myInc = winc;
            } else {
                opInc = winc;
            }
        } else if (tokens[i] == "binc" && i + 1 < tokens.size()) {
            int binc = std::stoi(tokens[i + 1]);
            if (board.sideToMove() == BLACK) {
                myInc = binc;
            } else {
                opInc = binc;
            }
        } else if (tokens[i] == "movestogo" && i + 1 < tokens.size()) {
            movesToGo = std::stoi(tokens[i + 1]);
        }
    }
    
    // Calculate time limit if not explicitly set
    if (timeLimit == 0 && !infinite && myTime > 0) {
        // Basic time management formula
        // Reserve some time for safety
        int safetyTime = 50; // 50ms safety margin
        int availableTime = myTime - safetyTime;
        
        if (availableTime > 0) {
            // Base time calculation
            if (movesToGo > 0) {
                // We have a specific number of moves to make
                timeLimit = availableTime / movesToGo;
            } else {
                // Assume we need to make ~30 more moves
                timeLimit = availableTime / 30;
            }
            
            // Add increment consideration
            // Use most of the increment but keep some safety margin
            if (myInc > 0) {
                timeLimit += myInc * 9 / 10; // Use 90% of increment
            }
            
            // Adjust based on game phase (simple heuristic)
            int moveNum = board.fullmoveNumber();
            if (moveNum < 10) {
                // Opening: use a bit more time
                timeLimit = timeLimit * 12 / 10;
            } else if (moveNum > 40) {
                // Endgame: use less time per move
                timeLimit = timeLimit * 8 / 10;
            }
            
            // Never use more than 1/4 of remaining time on a single move
            int maxTime = availableTime / 4;
            if (timeLimit > maxTime) {
                timeLimit = maxTime;
            }
            
            // Minimum time per move
            if (timeLimit < 10) {
                timeLimit = 10;
            }
        }
    }
    
    // Debug output
    std::cerr << "Time management: myTime=" << myTime << " myInc=" << myInc 
              << " movesToGo=" << movesToGo << " -> timeLimit=" << timeLimit << "ms" << std::endl;
    
    // Start search in a separate thread
    searchThread = std::thread([this, depth, timeLimit, infinite]() {
        if (!search) {
            search = new Search(board);
        }
        Move bestMove = search->think(depth, timeLimit, infinite);
        printBestMove(bestMove);
    });
}
void UCI::handleStop() {
    if (search) {
        search->stopSearch();
    }
    if (searchThread.joinable()) {
        searchThread.join();
    }
}

void UCI::handleQuit() {
    handleStop();
}

void UCI::handleSetOption(const std::string& line) {
    // Parse and handle options
    // For now, we'll ignore options
}

void UCI::printInfo(const SearchInfo& info) {
    std::cout << "info";
    std::cout << " depth " << info.depth;
    std::cout << " seldepth " << info.seldepth;
    std::cout << " score cp " << info.score;
    std::cout << " nodes " << info.nodes;
    
    auto elapsed = std::chrono::steady_clock::now() - info.startTime;
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
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

void UCI::printBestMove(Move m) {
    std::cout << "bestmove " << MoveUtils::toString(m) << std::endl;
}

Move UCI::parseMove(const std::string& str) {
    if (str.length() < 4) return 0;
    
    int fromFile = str[0] - 'a';
    int fromRank = str[1] - '1';
    int toFile = str[2] - 'a';
    int toRank = str[3] - '1';
    
    Square from = makeSquare(fromFile, fromRank);
    Square to = makeSquare(toFile, toRank);
    
    // Generate legal moves to find the matching move
    std::vector<Move> moves;
    MoveGenerator::generateLegalMoves(board, moves);
    
    for (Move m : moves) {
        if (MoveUtils::from(m) == from && MoveUtils::to(m) == to) {
            // Handle promotion
            if (MoveUtils::isPromotion(m) && str.length() > 4) {
                PieceType pt = NO_PIECE_TYPE;
                switch (str[4]) {
                    case 'q': pt = QUEEN; break;
                    case 'r': pt = ROOK; break;
                    case 'b': pt = BISHOP; break;
                    case 'n': pt = KNIGHT; break;
                }
                if (MoveUtils::promotionType(m) == pt) {
                    return m;
                }
            } else if (!MoveUtils::isPromotion(m)) {
                return m;
            }
        }
    }
    
    return 0;
}