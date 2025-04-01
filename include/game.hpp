#pragma once
#include <SFML/Window/Keyboard.hpp>
#include "road.hpp"
#include "car.hpp"

class Game {
public:
    // Construtores
    Game(int numCars);                     
    Game(int numCars, ControlType cType); 

    ~Game();

    void run();

private:
    bool running();
    void handleEvents();
    void populate();
    void createCar(int xPos, int yPos, ControlType cType);
    int getLaneAvailable();
    void update(float deltaTime);
    void render();
    void clean();
    void resetGame();
    void spawnTraffic(float deltaTime);
    void updateTraffic(float deltaTime);

private:
    sf::RenderWindow window;
    Road road;

    std::vector<Car*> cars; 
    int numCars;

    ControlType mainCarType;
    unsigned int windowWidth;
    unsigned int windowHeight;
    float windowScale;
    float spawnTimer     = 0.f;
    float spawnInterval  = 1.f;
};
