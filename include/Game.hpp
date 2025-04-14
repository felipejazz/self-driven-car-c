#ifndef GAME_HPP
#define GAME_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include <deque> // <<< Include deque for history limits
#include "Road.hpp"
#include <iostream>
#include <algorithm>
#include <filesystem> // Inclua para std::filesystem
#include <numeric>    // <<< Include for std::accumulate
#include <cmath>      // <<< Include for std::exp, std::max

// Forward declarations to avoid circular includes
class Car;
class Obstacle;
class Road;
class NeuralNetwork;
namespace sf { class Font; } // Forward declare sf::Font

// Enum for game states
enum class GameState {
    MENU,
    SIMULATION,
    HELP
};

class Game {
public:
    Game();
    ~Game(); // Destructor is important for resource cleanup (especially unique_ptr)

    void run(); // Main game loop entry point

private:
    // --- Core SFML & Asset Members ---
    sf::RenderWindow window;
    sf::Font font;
    sf::Texture helpTexture;
    sf::Texture menuBackgroundTexture; // For the menu background image
    sf::View carView;             // View for the car simulation area
    sf::View networkView;         // View for the neural network visualization
    sf::View statusView;          // View for the status/control panel text
    sf::View graphView;           // <<< NEW: View for the graphs
    Road road;                    // The road object
    sf::RectangleShape statusPanelBackground; // Background for the status text area
    sf::RectangleShape graphPanelBackground;  // <<< NEW: Background for the graph area

    // --- Game State & Menu Members (Depend on Assets) ---
    GameState currentState;
    int selectedMenuItemIndex;
    sf::Text statusText;             // Text displayed in the status panel
    std::vector<sf::Text> menuTexts; // Text objects for menu items
    sf::Sprite helpSprite;           // Sprite for the help image
    sf::Sprite menuBackgroundSprite; // Sprite for the menu background image
    bool helpTextureLoaded;
    bool menuBackgroundTextureLoaded;
    const std::vector<std::string> menuItems = {
        "Iniciar Novo Treinamento",
        "Visualizar Rede Treinada",
        "Ajuda",
        "Sair" // Certifique-se que esta opção corresponde ao case 3 em handleMenuKeyPress
    };
    const sf::Color menuNormalColor = sf::Color(200, 200, 200); // Normal menu item color
    const sf::Color menuSelectedColor = sf::Color::Yellow;      // Selected menu item color
    const sf::Color menuTitleColor = sf::Color::White;        // Menu title color
    const std::string VISUALIZE_BRAIN_FILENAME = "backups/bestBrain.dat"; // File to load for visualization
    bool loadSpecificBrainOnStart; // Flag to determine if loading specific brain

    // --- Simulation Object Members ---
    std::vector<std::unique_ptr<Car>> cars;              // Vector of car objects (managed by unique_ptr)
    std::vector<std::unique_ptr<Obstacle>> obstacles;    // Vector of obstacle objects (managed by unique_ptr)
    std::vector<Obstacle*> obstacleRawPtrs;            // Raw pointers for faster collision checks (non-owning)

    std::unique_ptr<NeuralNetwork> bestBrainOfGeneration; // Holds the best brain from the last generation
    std::vector<int> networkStructure;                  // Defines the NN structure (e.g., {5, 6, 4})

    Car* bestCarVisual; // Pointer to the car currently furthest ahead visually (non-owning)
    Car* focusedCar;    // Pointer to the car the camera/NN view follows (non-owning)

    // --- Simulation State Members ---
    int generationCount;         // Current generation number
    bool isPaused;               // Simulation pause state
    bool manualNavigationActive; // Is the user manually cycling focus?
    std::vector<Car*> navigableCars; // List of cars for manual navigation
    int currentNavIndex;         // Index in navigableCars for manual focus

    sf::Clock clock;           // Master clock for delta time
    sf::Clock generationClock; // Clock tracking time within a generation

    // +++ Membros para Controle de Velocidade e Renderização +++
    float simulationSpeedMultiplier; // Fator de velocidade atual
    bool renderingEnabled;           // Define se os gráficos devem ser desenhados
    const float MIN_SIMULATION_SPEED = 0.1f;  // Multiplicador mínimo de velocidade
    const float MAX_SIMULATION_SPEED = 10.0f; // Multiplicador máximo de velocidade
    const float SPEED_ADJUSTMENT_FACTOR = 1.2f; // Fator para aumentar/diminuir velocidade
    // +++ Fim dos Membros de Controle +++


    // --- NEW: Graphing Members ---
    std::deque<float> averageFitnessHistory; // Stores average fitness per generation
    std::deque<float> bestFitnessHistory;    // Stores best fitness per generation
    std::deque<float> mutationRateHistory;   // Stores mutation rate per generation
    float currentMutationRate;               // The mutation rate for the *next* generation
    const size_t MAX_HISTORY_POINTS = 200;   // Max data points to keep for graphs
    const float INITIAL_MUTATION_RATE = 0.15f;// Starting mutation rate
    const float MIN_MUTATION_RATE = 0.005f;   // Minimum mutation rate
    const float MUTATION_DECAY_FACTOR = 0.025f;// Controls how fast mutation decays (higher = faster decay)

    // --- Simulation Constants ---
    const std::string DEFAULT_BRAIN_FILENAME = "bestBrain.dat"; // Default file for saving/loading training brain
    const int NUM_AI_CARS = 1000;              // Number of AI cars per generation
    const int NUM_OBSTACLES = 30;              // Target number of obstacles on screen
    const float START_Y_POSITION = 100.0f;     // Initial Y position for cars
    const float OBSTACLE_REMOVAL_DISTANCE = 2000.0f; // Distance behind best car to remove obstacles
    const float GENERATION_ZONE_START_Y = -550.0f;  // Y offset ahead of best car where new obstacles start spawning
    const float GENERATION_ZONE_END_Y = -2000.0f; // Y offset ahead of best car where new obstacles stop spawning

    // --- Private Helper Methods ---
    // Setup
    void setupWindowAndViews();
    void loadAssets();
    void setupMenu();
    void initializeSimulation();

    // Event Handling
    void processEvents();
    void handleMenuKeyPress(const sf::Event::KeyPressed& keyEvent);
    void handleSimulationKeyPress(const sf::Event::KeyPressed& keyEvent);

    // Update Logic
    void updateMenu();
    void updateSimulation(sf::Time deltaTime);
    void updateFocus();         // Update which car the camera/NN focuses on
    void updateStatusPanel();   // Update the text in the status panel
    void updateMutationRate();  // <<< NEW: Update the mutation rate based on decay
    void updateGraphData(float avgFit, float bestFit, float mutRate); // <<< NEW: Store data for graphs

    // Rendering
    void renderMenu();
    void renderHelp();
    void renderSimulation();
    void renderGraphs();       // <<< NEW: Render the fitness/mutation graphs

    // Simulation Management
    void resetGeneration();     // Reset cars, obstacles, apply brains for a new generation
    void manageInfiniteObstacles(); // Remove far obstacles, generate new ones ahead
    void saveBestBrain();
    void discardSavedBrain();
    void togglePause();

    // Manual Focus Navigation
    void startManualNavigation();
    void navigateManual(sf::Keyboard::Key key);
    void stopManualNavigation();

    // Object Generation
    void populateCarVector(int N, float startY);
    void generateInitialObstacles(int N, float minY, float maxY, float minW, float maxW, float minH, float maxH);
    void applyBrainsToGeneration(int N); // Apply best brain + mutations to cars
    int getLaneIndex(float xPos);
    std::unique_ptr<Obstacle> generateSingleObstacle(
        float minY, float maxY,
        float minW, float maxW, float minH, float maxH,
        const sf::Color& color,
        float minVerticalGapAdjacentLane, float minVerticalGapSameLane,
        int maxPlacementRetries = 25); // Helper to place one obstacle safely
};

#endif // GAME_HPP