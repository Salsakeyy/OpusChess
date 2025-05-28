#ifndef UCI_H
#define UCI_H

#include "board.h"
#include "search.h"
#include <string>
#include <thread>

class UCI {
public:
    UCI();
    ~UCI();
    
    // Main UCI loop
    void loop();
    
private:
    Board board;
    Search* search;
    std::thread searchThread;
    
    // UCI command handlers
    void handleUCI();
    void handleIsReady();
    void handleUCINewGame();
    void handlePosition(const std::string& line);
    void handleGo(const std::string& line);
    void handleStop();
    void handleQuit();
    void handleSetOption(const std::string& line);
    
    // Helper methods
    void printInfo(const SearchInfo& info);
    void printBestMove(Move m);
    Move parseMove(const std::string& str);
    int parseTime(const std::string& line, const std::string& token);
};

#endif // UCI_H