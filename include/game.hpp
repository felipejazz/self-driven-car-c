#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include "car.hpp"
#include "road.hpp"
#include "controls.hpp"
#include "neural_network.hpp"

class Game {
private:
    sf::RenderWindow window;
    unsigned int windowWidth;
    unsigned int windowHeight;
    float windowScale;
    
    int numCars;
    ControlType mainCarType;
    Road road;
    std::vector<Car*> cars;
    
    // Variáveis para IA e evolução
    int m_generation;
    int m_populationSize;
    float m_mutationRate;
    float m_bestCarStuckTime;
    NeuralNetwork* m_bestBrain;
    
    // Variáveis para geração de tráfego
    float spawnTimer = 0.f;
    float spawnInterval = 1.f;
    
    void populate();
    void createCar(int xPos, int yPos, ControlType ctype);
    void spawnTraffic(float deltaTime);
    void updateTraffic(float deltaTime);
    int getLaneAvailable();
    Car* findBestCar() const;
    void evolve();
    
public:
    Game(int numCars, ControlType cType);
    ~Game();
    
    void run();
    bool running();
    void handleEvents();
    void update(float deltaTime);
    void render();
    void resetGame();
    void clean();
};