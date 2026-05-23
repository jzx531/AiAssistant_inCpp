#ifndef AI_SESSION_ID_GENERATOR_H
#define AI_SESSION_ID_GENERATOR_H

#include <chrono>
#include <random>
#include <cstdlib>
#include <ctime>
#include <string>


class AISessionIdGenerator {
public:
    AISessionIdGenerator() {
        
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }
    
    std::string generate();
};

#endif // AI_SESSION_ID_GENERATOR_H

