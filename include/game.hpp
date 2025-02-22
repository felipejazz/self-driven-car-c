#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include "car.hpp"

class Game {
public:
    unsigned int windowWidth;
    unsigned int windowHeight;
    unsigned int windowScale;

    sf::RenderWindow window;

    Game();
    void run();
    void update();
    void render();
    void handleEvents();
    void clean();
    bool running();

private:
    Car car; 
};

#endif // GAME_HPP
