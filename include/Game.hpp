#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include "Road.hpp"
#include <iostream>
#include <algorithm>
#include <filesystem>

// Forward declarations
class Car;
class Obstacle;
class Road;
class NeuralNetwork;
namespace sf { class Font; } // Não precisamos forward-declare Texture/Sprite

// Enumeração para os estados do jogo
enum class GameState {
    MENU,
    SIMULATION,
    HELP
};

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    // --- SFML & Window Members (Font e Texture declaradas PRIMEIRO) ---
    sf::RenderWindow window;
    sf::Font font;                   // <<< DECLARADA ANTES de statusText e menuTexts
    sf::Texture helpTexture;         // <<< DECLARADA ANTES de helpSprite
    sf::View carView;
    sf::View networkView;
    sf::View statusView;
    Road road;
    sf::RectangleShape statusPanelBackground;

    // --- Game State & Menu Members (Dependem de Font/Texture) ---
    GameState currentState;
    int selectedMenuItemIndex;
    sf::Text statusText;             // <<< DECLARADA DEPOIS de font
    std::vector<sf::Text> menuTexts; // <<< DECLARADA DEPOIS de font (será populada depois)
    sf::Sprite helpSprite;           // <<< DECLARADA DEPOIS de helpTexture
    bool helpTextureLoaded;
    const std::vector<std::string> menuItems = {
        "Iniciar Novo Treinamento",
        // "Visualizar Rede Treinada",
        "Ajuda"
    };
    const sf::Color menuNormalColor = sf::Color(200, 200, 200);
    const sf::Color menuSelectedColor = sf::Color::Yellow;
    const sf::Color menuTitleColor = sf::Color::White;
    const std::string VISUALIZE_BRAIN_FILENAME = "backups/bestBrain.dat";
    bool loadSpecificBrainOnStart;

    // --- Simulation Object Members ---
    std::vector<std::unique_ptr<Car>> cars;
    std::vector<std::unique_ptr<Obstacle>> obstacles;
    std::vector<Obstacle*> obstacleRawPtrs;

    std::unique_ptr<NeuralNetwork> bestBrainOfGeneration;
    std::vector<int> networkStructure;

    Car* bestCarVisual;
    Car* focusedCar;

    // --- Simulation State Members ---
    int generationCount;
    bool isPaused;
    bool manualNavigationActive;
    std::vector<Car*> navigableCars;
    int currentNavIndex;

    sf::Clock clock;
    sf::Clock generationClock;

    // --- Simulation Constants ---
    const std::string DEFAULT_BRAIN_FILENAME = "bestBrain.dat";
    const int NUM_AI_CARS = 1000;
    const int NUM_OBSTACLES = 30;
    const float START_Y_POSITION = 100.0f;
    const float OBSTACLE_REMOVAL_DISTANCE = 500.0f;
    const float GENERATION_ZONE_START_Y = -550.0f;
    const float GENERATION_ZONE_END_Y = -900.0f;

    // --- Private Helper Methods ---
    void setupWindowAndViews();
    void loadAssets();
    void setupMenu();
    void initializeSimulation();
    void processEvents();
    void handleMenuKeyPress(const sf::Event::KeyPressed& keyEvent);
    void handleSimulationKeyPress(const sf::Event::KeyPressed& keyEvent);
    void updateMenu();
    void updateSimulation(sf::Time deltaTime);
    void renderMenu();
    void renderHelp();
    void renderSimulation();
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