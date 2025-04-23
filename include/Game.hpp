#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include <deque>
#include "Road.hpp"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <numeric>
#include <cmath>

// Forward declarations
class Car;
class Obstacle;
class Road;
class NeuralNetwork;
namespace sf { class Font; }

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
    // --- Core SFML & Asset Members ---
    sf::RenderWindow window;
    sf::Font font;
    sf::Texture helpTexture;
    sf::Texture menuBackgroundTexture;
    sf::View carView;
    sf::View networkView;
    sf::View statusView;
    sf::View graphView;
    Road road;
    sf::RectangleShape statusPanelBackground;
    sf::RectangleShape graphPanelBackground;

    // --- Game State & Menu Members ---
    GameState currentState;
    int selectedMenuItemIndex;
    sf::Text statusText;
    std::vector<sf::Text> menuTexts;
    sf::Sprite helpSprite;
    sf::Sprite menuBackgroundSprite;
    bool helpTextureLoaded;
    bool menuBackgroundTextureLoaded;
    const std::vector<std::string> menuItems = {
        "Start new network train",
        "Visualize trained network ",
        "Help",
        "Exit"
    };
    const sf::Color menuNormalColor = sf::Color(200, 200, 200);
    const sf::Color menuSelectedColor = sf::Color::Yellow;
    const sf::Color menuTitleColor = sf::Color::White;
    const std::string VISUALIZE_BRAIN_FILENAME = "backups/bestBrain.dat"; // File to load for visualization
    bool loadSpecificBrainOnStart;

    // --- Simulation Object Members ---
    std::vector<std::unique_ptr<Car>> cars;
    std::vector<std::unique_ptr<Obstacle>> obstacles;
    std::vector<Obstacle*> obstacleRawPtrs; // Raw pointers for faster collision checks (non-owning)

    std::unique_ptr<NeuralNetwork> bestBrainOfGeneration;
    std::vector<int> networkStructure; // e.g., {5, 6, 4}

    Car* bestCarVisual; // Pointer to the car visually furthest ahead (non-owning)
    Car* focusedCar;    // Pointer to the car the camera/NN view follows (non-owning)

    // --- Simulation State Members ---
    int generationCount;
    bool isPaused;
    bool manualNavigationActive;
    std::vector<Car*> navigableCars; // List of cars for manual navigation cycle
    int currentNavIndex;

    sf::Clock clock;
    sf::Clock generationClock;

    // --- Simulation Speed Control ---
    float simulationSpeedMultiplier = 1.0f;
    bool renderingEnabled = true;
    const float MIN_SIMULATION_SPEED = 0.1f;
    const float MAX_SIMULATION_SPEED = 10.0f;
    const float SPEED_ADJUSTMENT_FACTOR = 1.2f;

    // --- Graphing Members ---
    std::deque<float> averageFitnessHistory;
    std::deque<float> bestFitnessHistory;
    std::deque<float> mutationRateHistory;
    float currentMutationRate;
    const size_t MAX_HISTORY_POINTS = 200; // Max data points for graphs
    const float INITIAL_MUTATION_RATE = 0.15f;
    const float MIN_MUTATION_RATE = 0.005f;
    const float MUTATION_DECAY_FACTOR = 0.025f; // Controls how fast mutation decays

    // --- Simulation Constants ---
    const std::string DEFAULT_BRAIN_FILENAME = "bestBrain.dat";
    const int NUM_AI_CARS = 1000;
    const int NUM_OBSTACLES = 30; // Target number of obstacles
    const float START_Y_POSITION = 100.0f;
    const float OBSTACLE_REMOVAL_DISTANCE = 2000.0f; // Distance behind best car to remove obstacles
    const float GENERATION_ZONE_START_Y = -550.0f;  // Y offset ahead for new obstacle spawns
    const float GENERATION_ZONE_END_Y = -2000.0f; // Furthest Y offset for new obstacle spawns

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
    void updateFocus();
    void updateStatusPanel();
    void updateMutationRate();
    void updateGraphData(float avgFit, float bestFit, float mutRate);

    void renderMenu();
    void renderHelp();
    void renderSimulation();
    void renderGraphs();

    void resetGeneration();
    void manageInfiniteObstacles(); // Create/destroy obstacles based on best car position
    void saveBestBrain();
    void discardSavedBrain();
    void togglePause();

    void startManualNavigation();
    void navigateManual(sf::Keyboard::Key key);
    void stopManualNavigation();

    void populateCarVector(int N, float startY);
    void generateInitialObstacles(int N, float minY, float maxY, float minW, float maxW, float minH, float maxH);
    void applyBrainsToGeneration(int N); // Apply best brain + mutations to cars
    int getLaneIndex(float xPos);
    std::unique_ptr<Obstacle> generateSingleObstacle(
        float minY, float maxY,
        float minW, float maxW, float minH, float maxH,
        const sf::Color& color,
        float minVerticalGapAdjacentLane, float minVerticalGapSameLane,
        int maxPlacementRetries = 25);
};

#endif // GAME_HPP