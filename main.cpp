<<<<<<< HEAD
#include "game.hpp"
=======
#include "Game.hpp" // Inclui a nova classe Game
>>>>>>> feat/v1Working
#include <iostream>

int main() {
    try {
<<<<<<< HEAD
        // Create a game with 50 AI cars
        Game game(50, ControlType::AI);
        game.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
=======
        Game game; // Cria a instância do jogo
        game.run(); // Inicia o loop principal do jogo
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS; 
}
>>>>>>> feat/v1Working
