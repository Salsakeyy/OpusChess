#include "uci.h"
#include <iostream>

int main() {
    std::cout << "Simple Chess Engine v1.0" << std::endl;
    std::cout << "Type 'uci' to start UCI mode" << std::endl;
    
    UCI uci;
    uci.loop();
    
    return 0;
}