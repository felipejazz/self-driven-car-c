#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include "Road.hpp" 
// Forward declarations (evita includes desnecessários no header)
class Car;
class Obstacle;
class Road;
class NeuralNetwork;
namespace sf { class Font; }

class Game {
public:
    Game(); // Construtor
    ~Game(); // Destrutor (se necessário para limpeza manual)

    void run(); // Método principal que contém o game loop

private:
    // --- Membros Privados (Configuração e Estado) ---
    sf::RenderWindow window;
    sf::View carView;
    sf::View networkView;
    sf::View statusView;
    sf::Font font;
    Road road;
    sf::RectangleShape statusPanelBackground;
    sf::Text statusText;

    std::vector<std::unique_ptr<Car>> cars;
    std::vector<std::unique_ptr<Obstacle>> obstacles;
    std::vector<Obstacle*> obstacleRawPtrs; // Para passar para Car::update

    std::unique_ptr<NeuralNetwork> bestBrainOfGeneration; // Usar unique_ptr ou optional
    std::vector<int> networkStructure;

    Car* bestCarVisual = nullptr; // Carro mais avançado visualmente
    Car* focusedCar = nullptr;    // Carro atualmente focado (para view e rede)

    int generationCount = 1;
    bool isPaused = false;
    bool manualNavigationActive = false;
    std::vector<Car*> navigableCars; // Carros não danificados para navegação
    int currentNavIndex = -1;

    sf::Clock clock;           // Para delta time
    sf::Clock generationClock; // Para tempo da geração

    // --- Constantes (pode movê-las para cá ou deixá-las globais) ---
    const std::string BRAIN_FILENAME = "bestBrain.dat";
    const int NUM_AI_CARS = 1000; // Mantido do main original
    const int NUM_OBSTACLES = 30; // Número alvo de obstáculos
    const float START_Y_POSITION = 100.0f;
    const float OBSTACLE_REMOVAL_DISTANCE = 500.0f;
    const float GENERATION_ZONE_START_Y = -550.0f;
    const float GENERATION_ZONE_END_Y = -900.0f;

    // --- Métodos Privados (Helpers) ---
    void setupWindowAndViews();
    void loadAssets();
    void initializeSimulation();
    void processEvents();
    void handleKeyPress(const sf::Event::KeyPressed& keyEvent);
    void update(sf::Time deltaTime);
    void render();
    void resetGeneration();
    void manageInfiniteObstacles();
    void updateFocus();
    void updateStatusPanel();
    void saveBestBrain();
    void discardSavedBrain();
    void togglePause();
    void startManualNavigation();
    void navigateManual(sf::Keyboard::Key key);
    void stopManualNavigation();

    // Funções que eram globais no main original (agora podem ser privadas)
    void populateCarVector(int N, float startY);
    void generateInitialObstacles(int N, float minY, float maxY, float minW, float maxW, float minH, float maxH);
    void applyBrainsToGeneration(int N);
    std::unique_ptr<Obstacle> generateSingleObstacle(
        float minY, float maxY,
        float minW, float maxW, float minH, float maxH,
        const sf::Color& color,
        float minVerticalGapAdjacentLane, float minVerticalGapSameLane,
        int maxPlacementRetries = 25);
};

#endif // GAME_HPP