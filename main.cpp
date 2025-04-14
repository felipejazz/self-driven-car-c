#include "Game.hpp" // Inclui a nova classe Game
#include <iostream>

int main() {
    try {
        Game game; // Cria a instância do jogo
        game.run(); // Inicia o loop principal do jogo
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS; // Termina normalmente
}

// Remova as funções auxiliares (populateCarVector, generateObstacles, applyBrainsToGeneration, generateSingleObstacle)
// que foram movidas para a classe Game.