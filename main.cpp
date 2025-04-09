#include "game.hpp"
#include <iostream>

int main() {
    try {
        // Create a game with 50 AI cars
        Game game(50, ControlType::AI);
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}