#include "Game.hpp"
#include "Car.hpp"
#include "Obstacle.hpp"
#include "Road.hpp"
#include "Network.hpp"
#include "Visualizer.hpp"
#include "Utils.hpp"

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/Text.hpp>
#include <iostream>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <random>
#include <filesystem>

// --- Construtor Corrigido ---
// In src/Game.cpp

Game::Game()
    : road(0, 0),
      // Inicializa font e textures PRIMEIRO
      font(),
      helpTexture(),
      menuBackgroundTexture(), // <<< ADD: Initialize menu background texture
      // AGORA inicializa statusText, sprites usando font/textures
      statusText(font, "", 16),
      helpSprite(helpTexture),
      menuBackgroundSprite(menuBackgroundTexture), // <<< ADD: Initialize menu background sprite
      // --- Inicializações restantes ---
      currentState(GameState::MENU),
      selectedMenuItemIndex(0),
      helpTextureLoaded(false),
      menuBackgroundTextureLoaded(false), // <<< ADD: Initialize menu background flag
      loadSpecificBrainOnStart(false),
      bestCarVisual(nullptr),
      focusedCar(nullptr),
      generationCount(1),
      isPaused(false),
      manualNavigationActive(false),
      currentNavIndex(-1)
      // window, views, background, clocks são inicializados por padrão
      // vetores/unique_ptr são inicializados vazios/nullptr por padrão
{
    std::cout << "Game constructor called." << std::endl;
    // Carrega Assets DEPOIS da lista de inicialização, mas ANTES de usá-los para configurar
    loadAssets();          // Carrega font, helpTexture E menuBackgroundTexture

    // Configura Janela/Views e Menu usando os assets carregados
    setupWindowAndViews(); // Configura janela e views
    setupMenu();           // Configura menuTexts e statusText

    std::cout << "Game setup complete. Starting in MENU state." << std::endl;
}


// --- Destructor ---
Game::~Game() {
    // The compiler implicitly generates member destruction code here.
    // Because Car.hpp, Obstacle.hpp, Network.hpp are included in this .cpp file,
    // the unique_ptr destructors will have the complete type information they need.
    std::cout << "Game destructor called." << std::endl; // Optional: add for verification
}

// --- Configuration & Initialization ---

void Game::setupWindowAndViews() {
    std::cout << "Setting up window and views..." << std::endl;
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    const unsigned int screenWidth = std::max(1200u, static_cast<unsigned int>(desktopMode.size.x));
    const unsigned int screenHeight = std::max(800u, static_cast<unsigned int>(desktopMode.size.y));

    const float totalOriginalWidth = 1200.0f;
    const float statusPanelProportion = 250.0f / totalOriginalWidth;
    const float carCanvasProportion = 200.0f / totalOriginalWidth;
    const float networkCanvasProportion = 1.0f - statusPanelProportion - carCanvasProportion;

    const float statusPanelWidthActual = screenWidth * statusPanelProportion;
    const float carCanvasWidthActual = screenWidth * carCanvasProportion;
    const float networkCanvasWidthActual = screenWidth * networkCanvasProportion;

    window.create(sf::VideoMode({screenWidth, screenHeight}), "Self-Driving Car Simulation");
    window.setFramerateLimit(60);
    std::cout << "Window created: " << screenWidth << "x" << screenHeight << std::endl;

    // Configure road based on actual car canvas size
    road = Road(carCanvasWidthActual / 2.0f, carCanvasWidthActual * 0.9f, 3);
    std::cout << "Road configured." << std::endl;

    // Configure Views
    carView.setSize({carCanvasWidthActual, (float)screenHeight});
    carView.setCenter({carCanvasWidthActual / 2.0f, (float)screenHeight / 2.0f});
    carView.setViewport(sf::FloatRect({statusPanelProportion, 0.f}, {carCanvasProportion, 1.f}));

    networkView.setSize({networkCanvasWidthActual, (float)screenHeight});
    networkView.setCenter({networkCanvasWidthActual / 2.0f, (float)screenHeight / 2.0f});
    networkView.setViewport(sf::FloatRect({statusPanelProportion + carCanvasProportion, 0.f}, {networkCanvasProportion, 1.f}));

    statusView.setSize({statusPanelWidthActual, (float)screenHeight});
    statusView.setCenter({statusPanelWidthActual / 2.0f, (float)screenHeight / 2.0f});
    statusView.setViewport(sf::FloatRect({0.f, 0.f}, {statusPanelProportion, 1.f}));
    std::cout << "Views configured." << std::endl;

    // Configure Status Panel Background
    statusPanelBackground.setSize({statusPanelWidthActual, (float)screenHeight});
    statusPanelBackground.setFillColor(sf::Color(40, 40, 40));
    statusPanelBackground.setPosition({0.f, 0.f});
    std::cout << "Status panel background configured." << std::endl;
}

// In src/Game.cpp

// In src/Game.cpp

void Game::loadAssets() {
    std::cout << "Loading assets..." << std::endl;

    // Carrega a Fonte no membro 'font'
    if (!font.openFromFile("assets/Roboto_Condensed-SemiBold.ttf")) {
        if (!font.openFromFile("../assets/Roboto_Condensed-SemiBold.ttf")) {
            std::cerr << "FATAL ERROR: Could not load font. Exiting." << std::endl;
            throw std::runtime_error("Failed to load font");
        }
    }
    std::cout << "Font loaded successfully." << std::endl;

    // --- Load Menu Background Texture ---
    std::cout << "Loading menu background texture..." << std::endl;
    menuBackgroundTextureLoaded = false; // Assume failure initially
    std::cout << "Attempting to load from: assets/home.png" << std::endl;
    if (menuBackgroundTexture.loadFromFile("assets/home.png")) {
        std::cout << ">>> Successfully loaded menu background from: assets/home.png" << std::endl;
        menuBackgroundTextureLoaded = true; // Loaded from primary path
    } else {
        // Try alternative path only if first failed
        std::cout << "Failed. Attempting to load from: ../assets/home.png" << std::endl;
        if (menuBackgroundTexture.loadFromFile("../assets/home.png")) {
            std::cout << ">>> Successfully loaded menu background from: ../assets/home.png" << std::endl;
            menuBackgroundTextureLoaded = true; // Loaded from alternative path
        } else {
            std::cerr << ">>> Warning: Could not load menu background texture from 'assets/home.png' or '../assets/home.png'. Menu background disabled." << std::endl;
            // menuBackgroundTextureLoaded remains false
        }
    }

    if(menuBackgroundTextureLoaded) {
        // Check dimensions *after* confirming load success
        sf::Vector2u texSize = menuBackgroundTexture.getSize();
        if (texSize.x > 0 && texSize.y > 0) { // Ensure texture is valid
             menuBackgroundSprite.setTexture(menuBackgroundTexture, true); // Link texture
             menuBackgroundSprite.setOrigin({static_cast<float>(texSize.x) / 2.f, static_cast<float>(texSize.y) / 2.f});
             std::cout << "Menu background sprite configured." << std::endl;
        } else {
             std::cerr << ">>> Warning: Menu background texture loaded but has zero dimensions. Disabling background." << std::endl;
             menuBackgroundTextureLoaded = false; // Treat as not loaded if dimensions are invalid
        }
    }
    // --- END: Load Menu Background Texture ---


    // Carrega a Textura de Ajuda no membro 'helpTexture'
    // (Lógica de carregamento da textura de ajuda permanece a mesma)
     std::cout << "Loading help texture..." << std::endl;
     helpTextureLoaded = false; // Assume failure
     if (!helpTexture.loadFromFile("assets/help.png")) {
          if (!helpTexture.loadFromFile("../assets/help.png")) {
             std::cerr << ">>> Warning: Could not load help texture 'assets/help.png' or '../assets/help.png'. Help screen disabled." << std::endl;
          } else {
               helpTextureLoaded = true; // Loaded from alternative path
               std::cout << ">>> Successfully loaded help texture from: ../assets/help.png" << std::endl;
          }
     } else {
          helpTextureLoaded = true; // Loaded from primary path
          std::cout << ">>> Successfully loaded help texture from: assets/help.png" << std::endl;
     }

     if(helpTextureLoaded) {
         sf::Vector2u texSize = helpTexture.getSize();
         if (texSize.x > 0 && texSize.y > 0) {
             helpSprite.setTexture(helpTexture, true);
             helpSprite.setOrigin({static_cast<float>(texSize.x) / 2.f, static_cast<float>(texSize.y) / 2.f});
             std::cout << "Help sprite configured." << std::endl;
         } else {
             std::cerr << ">>> Warning: Help texture loaded but has zero dimensions. Disabling help screen." << std::endl;
             helpTextureLoaded = false;
         }
     }
}

// In src/Game.cpp
void Game::setupMenu() {
    std::cout << "Setting up menu..." << std::endl;
    if (font.getInfo().family.empty()) { // Font check remains
        std::cerr << "Error in setupMenu: Font not loaded. Cannot create menu text." << std::endl;
         throw std::runtime_error("Font not loaded for menu setup");
    }

    // --- MODIFICATION START ---
    menuTexts.clear(); // Ensure the vector is empty before adding new items
    // menuTexts.reserve(menuItems.size()); // Optional: Pre-allocate memory

    float startY = window.getSize().y * 0.4f;
    float spacing = 70.0f;
    float centerX = window.getSize().x / 2.0f;

    for (size_t i = 0; i < menuItems.size(); ++i) {
        // Construct the sf::Text object directly with the font and add it
        // Using emplace_back is slightly more efficient as it constructs in place
        menuTexts.emplace_back(font, menuItems[i], 40); // Use the constructor sf::Text(font, string, size)

        // Get a reference to the newly added text object to configure it
        sf::Text& currentText = menuTexts.back();

        currentText.setFillColor(menuNormalColor);

        // Center the text origin (Corrected calculation)
        sf::FloatRect textBounds = currentText.getLocalBounds();
        currentText.setOrigin({textBounds.position.x + textBounds.size.x / 2.0f,
            textBounds.position.y + textBounds.size.y / 2.0f});
        currentText.setPosition({centerX, startY + i * spacing});
    }
    // --- MODIFICATION END ---

    // Highlight the selected item (This logic should work fine now)
    if (!menuTexts.empty()) {
         // Make sure selectedMenuItemIndex is valid (it's initialized to 0)
         if (selectedMenuItemIndex < 0 || selectedMenuItemIndex >= menuTexts.size()) {
             selectedMenuItemIndex = 0;
         }
         menuTexts[selectedMenuItemIndex].setFillColor(menuSelectedColor);
         menuTexts[selectedMenuItemIndex].setStyle(sf::Text::Bold);
    } else {
         selectedMenuItemIndex = 0; // Reset if vector happens to be empty
    }


    // Configura statusText (no changes needed here)
    statusText.setCharacterSize(16);
    statusText.setFillColor(sf::Color(200, 200, 200));
    statusText.setPosition({15.f, 15.f});

    std::cout << "Menu setup complete." << std::endl;
}
void Game::initializeSimulation() {
    std::cout << "Initializing Simulation..." << std::endl;
    generationCount = 1;
    generationClock.restart();
    isPaused = false;
    manualNavigationActive = false;
    currentNavIndex = -1;
    navigableCars.clear();
    cars.clear();
    obstacles.clear();
    obstacleRawPtrs.clear();

    // 1. Define Network Structure
    Car tempCar(0, 0, 30, 50, ControlType::AI); // Temp car to get sensor count
    int sensorRays = tempCar.getSensorRayCount();
    if (sensorRays <= 0) {
        std::cerr << "Warning: Default car has 0 sensor rays! Using fallback (5)." << std::endl;
        sensorRays = 5;
    }
    networkStructure = {sensorRays, 12, 4}; // Input(Sensors), Hidden(12), Output(4)
    std::cout << "Network structure defined: " << sensorRays << "-12-4" << std::endl;

    // 2. Populate Car Vector
    populateCarVector(NUM_AI_CARS, START_Y_POSITION);

    // 3. Prepare Base Brain (Load or Create New)
    bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure); // Always create a new one initially
    std::string brainFileToLoad;

    if (loadSpecificBrainOnStart) {
        // --- Visualization Mode ---
        brainFileToLoad = VISUALIZE_BRAIN_FILENAME;
        std::cout << "Attempting to load brain for visualization: " << brainFileToLoad << std::endl;
        std::filesystem::path backupPath(brainFileToLoad);

        if (!std::filesystem::exists(backupPath.parent_path()) && backupPath.has_parent_path()) {
            std::cerr << "Warning: Directory '" << backupPath.parent_path().string() << "' does not exist. Using random brain for visualization." << std::endl;
        } else if (!bestBrainOfGeneration->loadFromFile(brainFileToLoad)) {
            std::cerr << "Warning: Could not load brain file '" << brainFileToLoad << "'. Using random brain for visualization." << std::endl;
        } else {
            std::cout << "Successfully loaded brain for visualization: " << brainFileToLoad << std::endl;
            // Basic structure validation
            if (bestBrainOfGeneration->levels.empty() ||
                bestBrainOfGeneration->levels.front().inputs.size() != networkStructure[0] ||
                bestBrainOfGeneration->levels.back().outputs.size() != networkStructure.back()) {
                std::cerr << "Warning: Loaded visualization brain structure mismatch! Reverting to random brain." << std::endl;
                bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);
            }
        }
        // Apply the loaded/random brain WITHOUT mutation to all cars
        applyBrainsToGeneration(NUM_AI_CARS); // loadSpecificBrainOnStart=true handles no mutation

    } else {
        // --- Training Mode ---
        brainFileToLoad = DEFAULT_BRAIN_FILENAME;
        std::cout << "Attempting to load default brain for training: " << brainFileToLoad << std::endl;
        std::filesystem::path defaultBrainPath(brainFileToLoad);

         if (!std::filesystem::exists(defaultBrainPath.parent_path()) && defaultBrainPath.has_parent_path()) {
              std::cout << "Default brain directory '" << defaultBrainPath.parent_path().string() << "' not found. Starting training with random brain." << std::endl;
         }
        else if (bestBrainOfGeneration->loadFromFile(brainFileToLoad)) {
            std::cout << "Loaded default brain: " << brainFileToLoad << ". Resuming training." << std::endl;
            // Validation
            if (bestBrainOfGeneration->levels.empty() ||
                bestBrainOfGeneration->levels.front().inputs.size() != networkStructure[0] ||
                bestBrainOfGeneration->levels.back().outputs.size() != networkStructure.back()) {
                std::cerr << "Warning: Loaded default brain structure mismatch! Starting training with random brain." << std::endl;
                bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);
            }
        } else {
            std::cout << "No default brain found (" << brainFileToLoad << ") or directory missing. Starting new training with random brain." << std::endl;
        }
        // Apply the loaded/new brain WITH mutations (handled by applyBrainsToGeneration)
        applyBrainsToGeneration(NUM_AI_CARS);
    }

    // 4. Generate Initial Obstacles
    generateInitialObstacles(NUM_OBSTACLES, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);

    // 5. Set Initial Focus
    focusedCar = cars.empty() ? nullptr : cars[0].get();
    bestCarVisual = focusedCar;
    std::cout << "Initial focus set." << std::endl;

    // 6. Reset Master Clock (if needed, maybe not)
    // clock.restart();

    std::cout << "Simulation initialized successfully." << std::endl;
}

// --- Main Game Loop & Event Handling ---

void Game::run() {
    std::cout << "Starting game loop..." << std::endl;
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();

        processEvents(); // Handle all input and window events

        // Clear the window once per frame
        window.clear(sf::Color::Black); // Default background, states will draw over

        // Update and Render based on the current game state
        switch (currentState) {
            case GameState::MENU:
                updateMenu();
                renderMenu();
                break;

            case GameState::SIMULATION:
                if (!isPaused) {
                    updateSimulation(deltaTime);
                }
                renderSimulation();
                break;

            case GameState::HELP:
                // No update needed for static help screen yet
                renderHelp();
                break;
        }

        // Display the rendered frame
        window.display();
    }
    std::cout << "Exiting game loop." << std::endl;
}

void Game::processEvents() {
    while (const auto eventOpt = window.pollEvent()) {
        const sf::Event& event = *eventOpt;

        if (event.is<sf::Event::Closed>()) {
            std::cout << "Window close event received." << std::endl;
            window.close();
            return; // Exit event loop immediately
        }

        // Delegate event handling based on current state
        switch (currentState) {
            case GameState::MENU:
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    handleMenuKeyPress(*keyPressed);
                }
                break;

            case GameState::SIMULATION:
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    handleSimulationKeyPress(*keyPressed);
                }
                // Handle other simulation events (mouse clicks, joystick?) if needed
                break;

            case GameState::HELP:
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                    // Allow returning to menu with Escape or Enter
                    if (keyPressed->code == sf::Keyboard::Key::Escape || keyPressed->code == sf::Keyboard::Key::Enter) {
                        std::cout << "Exiting help screen." << std::endl;
                        currentState = GameState::MENU;
                    }
                }
                 // Handle window resize for help screen? (Might need to recalculate sprite scale/pos)
                 if (event.is<sf::Event::Resized>()) {
                     // Example: Recalculate help sprite scale/position if needed
                      if(helpTextureLoaded) {
                           sf::Vector2u texSize = helpTexture.getSize();
                           sf::Vector2u winSize = window.getSize();
                           float scaleX = (float)winSize.x / texSize.x * 0.9f;
                           float scaleY = (float)winSize.y / texSize.y * 0.9f;
                           float scale = std::min({scaleX, scaleY, 1.0f});
                           helpSprite.setScale({scale, scale});
                           helpSprite.setPosition({winSize.x / 2.f, winSize.y / 2.f});
                      }
                 }
                break;
        }
    }
}

void Game::handleMenuKeyPress(const sf::Event::KeyPressed& keyEvent) {
    int previousIndex = selectedMenuItemIndex;
    int numItems = static_cast<int>(menuItems.size());

    if (keyEvent.code == sf::Keyboard::Key::Up) {
        selectedMenuItemIndex = (selectedMenuItemIndex - 1 + numItems) % numItems; // Modulo arithmetic for wrap-around
    } else if (keyEvent.code == sf::Keyboard::Key::Down) {
        selectedMenuItemIndex = (selectedMenuItemIndex + 1) % numItems; // Modulo arithmetic
    } else if (keyEvent.code == sf::Keyboard::Key::Enter) {
        std::cout << "Menu selection: " << menuItems[selectedMenuItemIndex] << std::endl;
        switch (selectedMenuItemIndex) {
            case 0: // Iniciar Novo Treinamento
                loadSpecificBrainOnStart = false;
                initializeSimulation();
                if (window.isOpen()) currentState = GameState::SIMULATION;
                break;
            case 1: // Visualizar Rede Treinada
                loadSpecificBrainOnStart = true;
                initializeSimulation();
                if (window.isOpen()) currentState = GameState::SIMULATION;
                break;
            case 2: // Ajuda
                if (helpTextureLoaded) {
                    currentState = GameState::HELP;
                } else {
                    std::cerr << "Cannot show help: Help texture failed to load." << std::endl;
                    // Optional: Display temporary error message on menu screen
                }
                break;
        }
        return; // Exit after handling Enter

    } else if (keyEvent.code == sf::Keyboard::Key::Escape) {
        std::cout << "Escape pressed in menu. Exiting application." << std::endl;
        window.close();
        return;
    }

    // Update visual selection highlight if index changed
    if (previousIndex != selectedMenuItemIndex && !menuTexts.empty()) {
        if(previousIndex >= 0 && previousIndex < menuTexts.size()) {
            menuTexts[previousIndex].setFillColor(menuNormalColor);
            menuTexts[previousIndex].setStyle(sf::Text::Regular);
        }
        if(selectedMenuItemIndex >= 0 && selectedMenuItemIndex < menuTexts.size()) {
            menuTexts[selectedMenuItemIndex].setFillColor(menuSelectedColor);
            menuTexts[selectedMenuItemIndex].setStyle(sf::Text::Bold);
        }
    }
}

void Game::handleSimulationKeyPress(const sf::Event::KeyPressed& keyEvent) {
    bool isCtrlOrCmd = keyEvent.control || keyEvent.system; // Cmd on macOS

    switch (keyEvent.code) {
        case sf::Keyboard::Key::S:
            if (isCtrlOrCmd) { // Ctrl+S or Cmd+S to save
                std::cout << "Save key combination pressed." << std::endl;
                saveBestBrain();
            }
            break;
        case sf::Keyboard::Key::D:
            std::cout << "'Discard Brain' key pressed." << std::endl;
            discardSavedBrain(); // D key (no modifier) to discard
            break;
        case sf::Keyboard::Key::P:
            std::cout << "'Pause' key pressed." << std::endl;
            togglePause();
            break;
        case sf::Keyboard::Key::R:
            std::cout << "'Reset Generation' key pressed." << std::endl;
            resetGeneration();
            break;
        case sf::Keyboard::Key::N: // Next focused car
        case sf::Keyboard::Key::B: // Back/Previous focused car
            std::cout << "'Navigate Focus' key pressed (N/B)." << std::endl;
            if (!manualNavigationActive) startManualNavigation();
            navigateManual(keyEvent.code);
            break;
        case sf::Keyboard::Key::Enter:
            if (manualNavigationActive) {
                std::cout << "'Stop Navigate' key pressed." << std::endl;
                stopManualNavigation();
            }
            break;
        case sf::Keyboard::Key::Escape:
            std::cout << "Escape pressed in simulation. Returning to menu." << std::endl;
            currentState = GameState::MENU;
            isPaused = false; // Ensure simulation is not paused when returning
            stopManualNavigation();
            // Optional: Clear simulation state? (cars, obstacles etc.) or leave it for next start?
            // For now, leave it; initializeSimulation will clear them.
            break;
        default:
            // Key not mapped to a global simulation action
            break;
    }
}


// --- Update Methods ---

void Game::updateMenu() {
    // Currently no per-frame update logic needed for the menu itself.
    // Selection changes are handled directly in handleMenuKeyPress.
}

// In src/Game.cpp

void Game::updateSimulation(sf::Time deltaTime) {
    // --- Main Simulation Update Logic ---

    int nonDamagedCount = 0;
    bool anyCarMoved = false; // Track if any car is still moving forward

    // 1. Update Cars
    for (auto& carPtr : cars) {
        if (carPtr) {
            float yBefore = carPtr->position.y;
            carPtr->update(road, obstacleRawPtrs, deltaTime);
            if (!carPtr->isDamaged()) {
                nonDamagedCount++;
                if (carPtr->position.y < yBefore) { // Check if moved forward
                    anyCarMoved = true;
                }
            }
        }
    }

    // 2. Check for End of Generation
    bool allCarsDamaged = (nonDamagedCount == 0);
    // Stall check: No cars damaged, but none moved forward, and some time passed
    bool generationStalled = (!allCarsDamaged && !anyCarMoved && generationClock.getElapsedTime().asSeconds() > 5.0f);
    // *** NEW: Time limit check ***
    bool timeLimitExceeded = generationClock.getElapsedTime().asSeconds() > 60.0f;

    // *** MODIFIED: Add timeLimitExceeded to the condition ***
    if ((allCarsDamaged || generationStalled || timeLimitExceeded) && !cars.empty()) {
        std::cout << "\n--- GENERATION " << generationCount << " ENDED ";
        // *** MODIFIED: Add specific message for time limit ***
        if (timeLimitExceeded) std::cout << "(Time Limit Exceeded: >60s) ---" << std::endl;
        else if (generationStalled) std::cout << "(Stalled) ---" << std::endl;
        else std::cout << "(All Damaged) ---" << std::endl; // Default reason if others aren't true

        Car* carWithBestFitness = nullptr;
        float maxFitness = -std::numeric_limits<float>::infinity();

        // Find the best car of the generation (regardless of why generation ended)
        // Prioritize non-damaged cars if any exist, otherwise take the best fitness overall
        // (This part could be refined, but current logic finds absolute best fitness)
        for (const auto& carPtr : cars) {
            if (carPtr) {
                float currentCarFitness = carPtr->getFitness();
                if (currentCarFitness > maxFitness) {
                    maxFitness = currentCarFitness;
                    carWithBestFitness = carPtr.get();
                }
            }
        }

        // Process the best brain (same logic as before)
        if (carWithBestFitness && carWithBestFitness->brain) {
            *bestBrainOfGeneration = *(carWithBestFitness->brain); // Copy best brain
            std::cout << "Selected best brain (Fitness: " << maxFitness
                      << ", Y: " << carWithBestFitness->position.y << ")" << std::endl;
            if (!loadSpecificBrainOnStart) { // Save only if in training mode
                saveBestBrain();
            } else {
                std::cout << "(Visualization mode: Not saving brain)" << std::endl;
            }
        } else {
            // If no best car found (e.g., all cars invalid immediately) or it has no brain,
            // keep the brain from the previous generation (bestBrainOfGeneration remains unchanged)
            std::cout << "No valid best car/brain found for this generation. Keeping previous best brain." << std::endl;
        }

        // Prepare for the next round (same logic as before)
        if (!loadSpecificBrainOnStart) {
             std::cout << "--- Preparing Next Training Generation " << generationCount + 1 << " ---" << std::endl;
         } else {
              std::cout << "--- Resetting Visualization ---" << std::endl;
         }
        // Reset always uses the potentially updated bestBrainOfGeneration
        resetGeneration(); // Reset cars, obstacles, apply brains

    } else if (!allCarsDamaged) { // Only manage obstacles if generation is still running
        // 3. Manage Obstacles (Remove old, add new)
        manageInfiniteObstacles();
    }

    // 4. Update Camera/NN Focus (same logic as before)
    updateFocus();

    // 5. Update Status Panel Text (same logic as before)
    updateStatusPanel();
}
// --- Render Methods ---

// In src/Game.cpp

// In src/Game.cpp

void Game::renderMenu() {
    window.setView(window.getDefaultView()); // Use the main window view

    // --- Draw Background Image (if loaded) ---
    if (menuBackgroundTextureLoaded) {
        sf::Vector2u winSize = window.getSize();
        sf::Vector2u texSize = menuBackgroundTexture.getSize();

        if (texSize.x > 0 && texSize.y > 0) {
            float scaleX = static_cast<float>(winSize.x) / texSize.x;
            float scaleY = static_cast<float>(winSize.y) / texSize.y;
            float scale = std::min(scaleX, scaleY);

            menuBackgroundSprite.setScale({scaleX, scaleY}); // <<< CERTIFIQUE-SE QUE ESTÁ USANDO scaleX e scaleY AQUI            menuBackgroundSprite.setPosition({static_cast<float>(winSize.x) / 2.f, static_cast<float>(winSize.y) / 2.f});
            menuBackgroundSprite.setPosition({static_cast<float>(winSize.x) / 2.f, static_cast<float>(winSize.y) / 2.f});
            // ***** DEBUGGING DETALHADO (Versão Corrigida) *****
            std::cout << "--- DEBUG renderMenu ---" << std::endl;
            std::cout << "Window Size: (" << winSize.x << ", " << winSize.y << ")" << std::endl;
            std::cout << "Texture Size: (" << texSize.x << ", " << texSize.y << ")" << std::endl;
            std::cout << "Calculated Scale: " << scale << std::endl;
            std::cout << "Drawing BG Sprite - Pos: (" << menuBackgroundSprite.getPosition().x << ", " << menuBackgroundSprite.getPosition().y << ")"
                      << " Scale: (" << menuBackgroundSprite.getScale().x << ", " << menuBackgroundSprite.getScale().y << ")" << std::endl;
            const sf::Color& spriteColor = menuBackgroundSprite.getColor();
            std::cout << "Drawing BG Sprite - Color: (" << (int)spriteColor.r << "," << (int)spriteColor.g << "," << (int)spriteColor.b << "," << (int)spriteColor.a << ")" << std::endl;
            const sf::IntRect& texRect = menuBackgroundSprite.getTextureRect();
            // Corrigido para acessar membros de sf::IntRect corretamente
            std::cout << "Drawing BG Sprite - TexRect: (L:" << texRect.position.x << ", T:" << texRect.size.x << ", W:" << texRect.position.y << ", H:" << texRect.size.y << ")" << std::endl;

            // Armazena o ponteiro retornado e verifica
            const sf::Texture* pTextureFromSprite = &(menuBackgroundSprite.getTexture());

            // Imprime os endereços para comparação visual (útil se a comparação direta falhar)
             std::cout << "Drawing BG Sprite - Actual Texture Pointer Addr: " << static_cast<const void*>(pTextureFromSprite) << std::endl;
             std::cout << "Drawing BG Sprite - Expected menuBackgroundTexture Addr: " << static_cast<const void*>(&menuBackgroundTexture) << std::endl;

            // Verifica o ponteiro de forma idiomática
            if (pTextureFromSprite) { // Checa se não é nullptr
                 if (pTextureFromSprite == &menuBackgroundTexture) { // Compara os ponteiros
                     std::cout << "Drawing BG Sprite - Texture Pointer Check: OK (matches menuBackgroundTexture)" << std::endl;
                 } else {
                     std::cout << "Drawing BG Sprite - Texture Pointer Check: WARNING (Pointer exists, but doesn't match menuBackgroundTexture!)" << std::endl;
                 }
             } else {
                 std::cout << "Drawing BG Sprite - Texture Pointer Check: ERROR (nullptr!)" << std::endl;
             }
            std::cout << "--- END DEBUG renderMenu ---" << std::endl;
            // ***** FIM DEBUGGING *****

            window.draw(menuBackgroundSprite); // A chamada de desenho

        } else {
             std::cout << "--- DEBUG renderMenu: Texture size is zero, skipping background draw. ---" << std::endl;
        }
    } else {
        std::cout << "--- DEBUG renderMenu: menuBackgroundTextureLoaded is false, skipping background draw. ---" << std::endl;
    }
    // --- End Background Image ---


    // Draw Title (Existing code)
    sf::Text titleText(font, "", 60);
    titleText.setFillColor(menuTitleColor);
    titleText.setStyle(sf::Text::Bold | sf::Text::Underlined);
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.0f, titleBounds.position.y + titleBounds.size.y / 2.0f});
    titleText.setPosition({window.getSize().x / 2.0f, window.getSize().y * 0.2f});
    window.draw(titleText);

    // Draw Menu Items (Existing code)
    for (const auto& text : menuTexts) {
        window.draw(text);
    }

     // Draw Basic Instructions (Existing code)
     sf::Text instructionText(font, "Use UP/DOWN arrows and ENTER to select. ESC to exit.", 18);
     instructionText.setFillColor(sf::Color(180, 180, 180));
     sf::FloatRect instBounds = instructionText.getLocalBounds();
      // Corrigido para usar left/top/width/height que são mais robustos que position/size para bounds
     instructionText.setOrigin({instBounds.position.x + instBounds.size.x / 2.0f, instBounds.position.y + instBounds.size.y / 2.0f});
     instructionText.setPosition({window.getSize().x / 2.0f, window.getSize().y * 0.9f});
     window.draw(instructionText);
}

void Game::renderHelp() {
    window.setView(window.getDefaultView()); // Use the main window view

    if (helpTextureLoaded) {
        // Ensure sprite is centered and scaled correctly for current window size
        sf::Vector2u texSize = helpTexture.getSize();
        sf::Vector2u winSize = window.getSize();
        helpSprite.setPosition({winSize.x / 2.f, winSize.y / 2.f});
        float scaleX = (float)winSize.x / texSize.x * 0.9f; // 90% window fit
        float scaleY = (float)winSize.y / texSize.y * 0.9f;
        float scale = std::min({scaleX, scaleY, 1.0f}); // Only scale down
        helpSprite.setScale({scale, scale});

        window.draw(helpSprite);

        // Draw return instruction text
        sf::Text returnText(font, "Press ESC or Enter to return to menu", 20);
        returnText.setFillColor(sf::Color(200, 200, 200, 220)); // Slightly transparent white
        sf::FloatRect bounds = returnText.getLocalBounds();
        returnText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        returnText.setPosition({window.getSize().x / 2.f, window.getSize().y * 0.95f}); // Near bottom
        window.draw(returnText);

    } else {
        // Display error message if texture failed to load
        sf::Text errorText(font, "Error: Could not load help image (assets/help.png)", 24);
        errorText.setFillColor(sf::Color::Red);
        sf::FloatRect bounds = errorText.getLocalBounds();
        errorText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        errorText.setPosition({window.getSize().x / 2.f, window.getSize().y / 2.f});
        window.draw(errorText);

        sf::Text returnText(font, "Press ESC or Enter to return", 20);
        returnText.setFillColor(sf::Color(200, 200, 200));
        bounds = returnText.getLocalBounds();
        returnText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.x + bounds.size.y / 2.f});
        returnText.setPosition({window.getSize().x / 2.f, window.getSize().y * 0.6f});
        window.draw(returnText);
    }
}

void Game::renderSimulation() {
    // --- Render the Simulation State ---

    // 1. Draw Status Panel
    window.setView(statusView);
    window.draw(statusPanelBackground);
    window.draw(statusText); // Updated in updateSimulation

    // 2. Draw Car Simulation Area
    // Update car view center (smoothly)
    if (focusedCar) {
        float viewCenterX = carView.getSize().x / 2.0f;
        float targetY = focusedCar->position.y - window.getSize().y * 0.3f; // Look ahead slightly
        float currentCenterY = carView.getCenter().y;
        // Lerp for smooth camera movement (adjust factor 0.05 for speed)
        float newCenterY = lerp(currentCenterY, targetY, 0.05f);
        carView.setCenter({viewCenterX, newCenterY});
    } else {
        // Fallback if no car is focused (e.g., all cars crashed in first frame)
        carView.setCenter({carView.getSize().x / 2.0f, START_Y_POSITION - window.getSize().y * 0.3f});
    }
    window.setView(carView); // Apply the car view

    // Draw road, obstacles, and cars
    road.draw(window);
    for (const auto& obsPtr : obstacles) { if (obsPtr) obsPtr->draw(window); }
    // Draw non-focused cars first (no sensors)
    for (const auto& carPtr : cars) {
        if (carPtr && carPtr.get() != focusedCar) {
            carPtr->draw(window, false); // false = don't draw sensor
        }
    }
    // Draw focused car last (with sensor)
    if (focusedCar) {
        focusedCar->draw(window, true); // true = draw sensor
    }

    // 3. Draw Neural Network Visualization
    window.setView(networkView);
    sf::RectangleShape networkBackground(networkView.getSize());
    networkBackground.setFillColor(sf::Color(60, 60, 60));
    networkBackground.setPosition({0.f, 0.f});
    window.draw(networkBackground);

    // Find the brain to draw (from the focused car)
    NeuralNetwork* brainToDraw = nullptr;
    if (focusedCar && focusedCar->useBrain && focusedCar->brain) {
        brainToDraw = &(*(focusedCar->brain));
    }

    // Draw the network using the Visualizer class
    if (brainToDraw && !brainToDraw->levels.empty()) {
        Visualizer::drawNetwork(window, *brainToDraw, font,
                                0.f, 0.f, // Draw relative to the network view's origin
                                networkView.getSize().x, networkView.getSize().y);
    } else {
        // Display message if no brain is available for the focused car
        sf::Text noBrainText(font, "No brain to display", 20);
        noBrainText.setFillColor(sf::Color::White);
        sf::FloatRect textBounds = noBrainText.getLocalBounds();
        noBrainText.setOrigin({textBounds.position.x + textBounds.size.y / 2.0f, textBounds.position.y + textBounds.size.y / 2.0f});
        noBrainText.setPosition({networkView.getSize().x / 2.0f, networkView.getSize().y / 2.0f}); // Center in network view
        window.draw(noBrainText);
    }
}

// --- Simulation Management Methods ---

void Game::resetGeneration() {
    if (!loadSpecificBrainOnStart) {
        std::cout << "--- RESETTING Training Generation " << generationCount << " ---" << std::endl;
        generationCount++;
    } else {
        std::cout << "--- Resetting Visualization State ---" << std::endl;
        // Do not increment generationCount in visualization mode
    }

    // 1. Reset Car States
    for (auto& carPtr : cars) {
        if (carPtr) carPtr->resetForNewGeneration(START_Y_POSITION, road);
    }

    // 2. Reapply Brains (Handles training mutation vs visualization copy internally)
    applyBrainsToGeneration(NUM_AI_CARS);

    // 3. Reset Obstacles
    obstacles.clear();
    obstacleRawPtrs.clear();
    generateInitialObstacles(NUM_OBSTACLES, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);

    // 4. Reset Control/Focus States
    manualNavigationActive = false;
    currentNavIndex = -1;
    navigableCars.clear();
    // Reset focus to the first car (or null if cars is empty)
    focusedCar = cars.empty() ? nullptr : cars[0].get();
    bestCarVisual = focusedCar; // Assume first car is initially best visually

    // 5. Reset Generation Clock and Pause State
    generationClock.restart();
    isPaused = false;

    std::cout << "--- Generation " << generationCount << (loadSpecificBrainOnStart ? " (Visualization)" : "") << " Ready --- \n" << std::endl;
}

void Game::saveBestBrain() {
    if (loadSpecificBrainOnStart) {
        std::cout << "(Save disabled in Visualization mode)" << std::endl;
        return; // Don't save when just visualizing
    }

    if (bestBrainOfGeneration) {
        std::filesystem::path filePath(DEFAULT_BRAIN_FILENAME);
        std::filesystem::path dirPath = filePath.parent_path();

        // Create directory if it doesn't exist
        if (!dirPath.empty() && !std::filesystem::exists(dirPath)) {
            try {
                if (std::filesystem::create_directories(dirPath)) {
                    std::cout << "Created directory: " << dirPath.string() << std::endl;
                } else {
                     std::cerr << "Warning: Failed to create directory (unknown reason): " << dirPath.string() << std::endl;
                     // Continue trying to save anyway, might work if path is relative
                }
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error creating directory '" << dirPath.string() << "': " << e.what() << std::endl;
                return; // Stop if directory creation fails definitively
            }
        }

        // Attempt to save the file
        if (bestBrainOfGeneration->saveToFile(DEFAULT_BRAIN_FILENAME)) {
            std::cout << "Saved Best Generation Brain to " << DEFAULT_BRAIN_FILENAME << std::endl;
        } else {
            std::cerr << "Error saving brain to " << DEFAULT_BRAIN_FILENAME << std::endl;
        }
    } else {
        std::cerr << "No best brain available to save." << std::endl;
    }
}

void Game::discardSavedBrain() {
    std::cout << "Attempting to discard saved brain file(s)..." << std::endl;
    bool discardedDefault = false;
    bool discardedVis = false;

    // Discard Default Training Brain
    if (std::filesystem::exists(DEFAULT_BRAIN_FILENAME)) {
        if (std::remove(DEFAULT_BRAIN_FILENAME.c_str()) == 0) {
            std::cout << "Discarded training brain file: " << DEFAULT_BRAIN_FILENAME << std::endl;
            discardedDefault = true;
        } else {
            perror(("Error removing file: " + DEFAULT_BRAIN_FILENAME).c_str());
        }
    } else {
        std::cout << "Training brain file not found: " << DEFAULT_BRAIN_FILENAME << std::endl;
    }

    // Discard Visualization Brain
     if (std::filesystem::exists(VISUALIZE_BRAIN_FILENAME)) {
        if (std::remove(VISUALIZE_BRAIN_FILENAME.c_str()) == 0) {
            std::cout << "Discarded visualization brain file: " << VISUALIZE_BRAIN_FILENAME << std::endl;
            discardedVis = true;
        } else {
             perror(("Error removing file: " + VISUALIZE_BRAIN_FILENAME).c_str());
        }
    } else {
        std::cout << "Visualization brain file not found: " << VISUALIZE_BRAIN_FILENAME << std::endl;
    }

    if (discardedDefault || discardedVis) {
        // Optional: Reset the in-memory brain if needed, e.g., if currently running sim
         if (currentState == GameState::SIMULATION && bestBrainOfGeneration) {
            std::cout << "Resetting current in-memory brain to random." << std::endl;
            bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);
             // Re-apply this new random brain to the current generation
            applyBrainsToGeneration(NUM_AI_CARS);
         }
    }
}


void Game::togglePause() {
    if (currentState == GameState::SIMULATION) {
        isPaused = !isPaused;
        std::cout << "Simulation " << (isPaused ? "PAUSED" : "RESUMED") << std::endl;
    } else {
        std::cout << "(Pause command ignored outside simulation)" << std::endl;
    }
}

// --- Manual Navigation Methods ---

void Game::startManualNavigation() {
    if (currentState != GameState::SIMULATION) return;

    manualNavigationActive = true;
    navigableCars.clear();

    // Populate with non-damaged AI cars
    for (const auto& carPtr : cars) {
        if (carPtr && !carPtr->isDamaged() && carPtr->useBrain) {
            navigableCars.push_back(carPtr.get());
        }
    }

    if (navigableCars.empty()) {
        manualNavigationActive = false;
        currentNavIndex = -1;
        std::cout << "Manual Navigation Failed: No non-damaged AI cars available." << std::endl;
        return;
    }

    // Sort by Y position (ascending, lower Y is further ahead)
    std::sort(navigableCars.begin(), navigableCars.end(), [](const Car* a, const Car* b) {
        return a->position.y < b->position.y;
    });

    // Try to find current focus, otherwise default to best (index 0)
    currentNavIndex = 0; // Default to rank 1
    if (focusedCar) {
        auto it = std::find(navigableCars.begin(), navigableCars.end(), focusedCar);
        if (it != navigableCars.end()) {
            currentNavIndex = static_cast<int>(std::distance(navigableCars.begin(), it));
        } else {
             focusedCar = navigableCars[currentNavIndex]; // Focus wasn't in list, set to rank 1
        }
    } else {
        focusedCar = navigableCars[currentNavIndex]; // No focus before, set to rank 1
    }

    std::cout << "Manual Navigation ON. Cars available: " << navigableCars.size()
              << ". Focused Rank: " << (currentNavIndex + 1) << std::endl;
}

void Game::navigateManual(sf::Keyboard::Key key) {
    if (!manualNavigationActive || navigableCars.empty() || currentState != GameState::SIMULATION) return;

    int numNavigable = static_cast<int>(navigableCars.size());
    if (key == sf::Keyboard::Key::N) { // Next
        currentNavIndex = (currentNavIndex + 1) % numNavigable;
    } else if (key == sf::Keyboard::Key::B) { // Back/Previous
        currentNavIndex = (currentNavIndex - 1 + numNavigable) % numNavigable;
    } else {
        return; // Ignore other keys
    }

    focusedCar = navigableCars[currentNavIndex]; // Update the focused car pointer
    std::cout << "Navigated focus to Rank: " << (currentNavIndex + 1) << std::endl;
    // updateFocus will handle if this car gets damaged later
}

void Game::stopManualNavigation() {
    if (currentState != GameState::SIMULATION) return;

    if (manualNavigationActive) {
        manualNavigationActive = false;
        currentNavIndex = -1;
        navigableCars.clear(); // Clear the list
        std::cout << "Manual Navigation OFF." << std::endl;
        // Focus will revert to automatic (bestCarVisual) in the next updateFocus call
    }
}

// --- Object Generation Helpers ---

void Game::populateCarVector(int N, float startY) {
    cars.clear(); // Ensure vector is empty before populating
    cars.reserve(N);
    std::cout << "Generating " << N << " AI cars..." << std::endl;
    for (int i = 0; i < N; ++i) {
        cars.push_back(std::make_unique<Car>(
            road.getLaneCenter(1), // Start in the middle lane
            startY,                // Initial Y position
            30.0f, 50.0f,          // Width, Height
            ControlType::AI,       // Always AI type for simulation cars
            3.0f,                  // Max Speed
            getRandomColor()       // Assign a random color
        ));
    }
    std::cout << cars.size() << " AI cars generated." << std::endl;
}

void Game::generateInitialObstacles(int N, float minY, float maxY, float minW, float maxW, float minH, float maxH) {
    obstacles.clear();
    obstacleRawPtrs.clear();
    obstacles.reserve(N);
    std::cout << "Generating " << N << " initial obstacles between Y=" << minY << " and Y=" << maxY << "..." << std::endl;

    const sf::Color obstacleColor = sf::Color(128, 128, 128); // Gray
    const float minVerticalGapAdjacentLane = 60.0f; // Increased gap slightly
    const float minVerticalGapSameLane = 100.0f;    // Increased gap slightly
    int obstaclesPlaced = 0;
    int totalAttemptsOverall = 0;
    const int maxTotalAttempts = N * 50; // More attempts per obstacle

    while (obstaclesPlaced < N && totalAttemptsOverall < maxTotalAttempts) {
        totalAttemptsOverall++;
        auto newObstacle = generateSingleObstacle(
            minY, maxY, minW, maxW, minH, maxH, obstacleColor,
            minVerticalGapAdjacentLane, minVerticalGapSameLane, 50 // Max retries per single obstacle
        );

        if (newObstacle) {
            obstacleRawPtrs.push_back(newObstacle.get()); // Add raw pointer
            obstacles.push_back(std::move(newObstacle)); // Transfer ownership
            obstaclesPlaced++;
        }
    }

    if (obstaclesPlaced < N) {
        std::cerr << "Warning: Only generated " << obstaclesPlaced << "/" << N
                  << " initial obstacles due to spacing constraints or max attempts." << std::endl;
    }
    std::cout << obstacles.size() << " initial obstacles placed." << std::endl;
}

// Replace the existing Game::generateSingleObstacle function in src/Game.cpp with this:

// Coloque esta função completa no seu arquivo src/Game.cpp
// Certifique-se de que <limits>, <random> e "Utils.hpp" estão incluídos no topo do Game.cpp

#include <limits>   // Necessário para std::numeric_limits
#include <random>   // Necessário para std::mt19937 etc. (se usar localmente)
#include "Utils.hpp" // Necessário para getRandomInt e getRandomFloat

std::unique_ptr<Obstacle> Game::generateSingleObstacle(
    float minY, float maxY,
    float minW, float maxW, float minH, float maxH,
    const sf::Color& color,
    float minVerticalGapAdjacentLane, float minVerticalGapSameLane,
    int maxPlacementRetries)
{

    int attempts = 0;
    while (attempts < maxPlacementRetries) {
        attempts++;
        // 1. Gera propriedades do obstáculo potencial
        int potentialLaneIndex = getRandomInt(0, road.laneCount - 1); // Usando helper
        float potentialYPos = getRandomFloat(minY, maxY);         // Usando helper
        float potentialWidth = getRandomFloat(minW, maxW);        // Usando helper
        float potentialHeight = getRandomFloat(minH, maxH);       // Usando helper

        // --- Calcula a posição X horizontal com desvio aleatório dentro da faixa ---
        float laneWidth = road.width / static_cast<float>(road.laneCount);
        float laneLeft = road.left + potentialLaneIndex * laneWidth;
        float laneRight = laneLeft + laneWidth;

        // Calcula a faixa válida para o *centro* do obstáculo
        float minCenterX = laneLeft + potentialWidth / 2.0f;
        float maxCenterX = laneRight - potentialWidth / 2.0f;

        float potentialXPos;
        if (maxCenterX <= minCenterX) {
            // Obstáculo é tão largo quanto ou mais largo que a faixa, centraliza
            potentialXPos = road.getLaneCenter(potentialLaneIndex);
        } else {
            // Obstáculo cabe, gera posição X aleatória para o centro dentro da faixa válida
            potentialXPos = getRandomFloat(minCenterX, maxCenterX);
        }
        // --- Fim do cálculo da posição X ---

        // Calcula limites verticais
        float potentialTop = potentialYPos - potentialHeight / 2.0f;
        float potentialBottom = potentialYPos + potentialHeight / 2.0f;

        // 2. Verifica colisão/sobreposição com obstáculos existentes
        bool collisionFound = false;
        for (const auto& existingObsPtr : obstacles) {
            if (!existingObsPtr) continue;

            // Propriedades do obstáculo existente
            float existingXPos = existingObsPtr->position.x;
            float existingYPos = existingObsPtr->position.y;
            float existingHeight = existingObsPtr->height;
            float existingTop = existingYPos - existingHeight / 2.0f;
            float existingBottom = existingYPos + existingHeight / 2.0f;

            // Determina a pista do obstáculo existente (aproximado)
            int existingLaneIndex = -1;
            float minLaneDist = std::numeric_limits<float>::max();
            for (int l = 0; l < road.laneCount; ++l) {
                float dist = std::abs(existingXPos - road.getLaneCenter(l));
                 if (dist < (laneWidth / 2.0f) && dist < minLaneDist) { // Verifica se está mais perto que metade da largura da faixa
                    minLaneDist = dist;
                    existingLaneIndex = l;
                 }
            }

            // Determina a folga vertical necessária
            float requiredVerticalGap = 0.0f;
            if (potentialLaneIndex == existingLaneIndex && existingLaneIndex != -1) {
                requiredVerticalGap = minVerticalGapSameLane;
            } else if (existingLaneIndex != -1 && std::abs(potentialLaneIndex - existingLaneIndex) == 1) {
                requiredVerticalGap = minVerticalGapAdjacentLane;
            }

            // Verifica sobreposição vertical (considerando a folga)
            bool verticalOverlapWithGap = (potentialTop - requiredVerticalGap < existingBottom) &&
                                          (potentialBottom + requiredVerticalGap > existingTop);

            // Verifica sobreposição vertical direta (sem folga)
            bool directVerticalOverlap = potentialTop < existingBottom && potentialBottom > existingTop;

            // Condição de colisão:
            // (Sobreposição direta na *mesma* faixa) OU (Sobreposição com folga quando a folga é necessária)
            if ((directVerticalOverlap && potentialLaneIndex == existingLaneIndex && existingLaneIndex != -1) ||
                (verticalOverlapWithGap && requiredVerticalGap > 0.0f))
            {
                collisionFound = true;
                break; // Conflito encontrado, tenta nova posição
            }
        } // Fim do loop pelos obstáculos existentes

        // 3. Se não houve colisão, cria e retorna o obstáculo
        if (!collisionFound) {
            return std::make_unique<Obstacle>(potentialXPos, potentialYPos, potentialWidth, potentialHeight, color);
        }
        // Se houve colisão, tenta novamente

    } // Fim das tentativas

    // Falhou em posicionar após todas as tentativas
    // std::cerr << "Warning: Could not place single obstacle after " << maxPlacementRetries << " attempts." << std::endl; // Opcional
    return nullptr;
}

void Game::applyBrainsToGeneration(int N) {
    if (cars.empty() || !bestBrainOfGeneration) {
        std::cerr << "Error in applyBrainsToGeneration: No cars or no base brain available." << std::endl;
        return;
    }

    if (loadSpecificBrainOnStart) {
        // --- Visualization Mode: Apply the same brain to all ---
        std::cout << "Applying visualized brain to all cars (no mutation)." << std::endl;
        for (int i = 0; i < cars.size(); ++i) {
            if (cars[i] && cars[i]->useBrain) {
                // Ensure car has a brain instance, then copy contents
                if (!cars[i]->brain) {
                    cars[i]->brain.emplace(*bestBrainOfGeneration); // Create copy
                } else {
                    *(cars[i]->brain) = *bestBrainOfGeneration; // Overwrite with copy
                }
                // Explicitly set mutation amount to 0 for visualization
                NeuralNetwork::mutate(*(cars[i]->brain), 0.0f);
            }
        }
    } else {
        // --- Training Mode: Elitism + Mutation ---
        std::cout << "Applying training brain (elite + mutations) to generation " << generationCount << "." << std::endl;
        // Elite car (index 0) gets the best brain directly
        if (cars[0] && cars[0]->useBrain) {
            if (!cars[0]->brain) {
                cars[0]->brain.emplace(*bestBrainOfGeneration);
            } else {
                *(cars[0]->brain) = *bestBrainOfGeneration;
            }
            // No mutation for the elite
            // NeuralNetwork::mutate(*(cars[0]->brain), 0.0f); // Already handled by copying
        }

        // Other cars get a copy of the best brain + mutation
        float mutationRate = 0.05f; // Adjust mutation rate as needed
        for (int i = 1; i < cars.size(); ++i) {
            if (cars[i] && cars[i]->useBrain) {
                if (!cars[i]->brain) {
                    cars[i]->brain.emplace(*bestBrainOfGeneration);
                } else {
                    *(cars[i]->brain) = *bestBrainOfGeneration; // Start with best brain copy
                }
                // Apply mutation
                NeuralNetwork::mutate(*(cars[i]->brain), mutationRate);
            }
        }
    }
}


// --- Obstacle/Focus Management --- (Rest of the functions from previous answer)
void Game::manageInfiniteObstacles() {
     bestCarVisual = nullptr;
     float minY_visual = std::numeric_limits<float>::max();
     for (const auto& carPtr : cars) {
         if (carPtr && !carPtr->isDamaged()) {
             if (carPtr->position.y < minY_visual) {
                 minY_visual = carPtr->position.y;
                 bestCarVisual = carPtr.get();
             }
         }
     }

     if (!bestCarVisual) return;

     const float generationMinY = bestCarVisual->position.y + GENERATION_ZONE_END_Y;
     const float generationMaxY = bestCarVisual->position.y + GENERATION_ZONE_START_Y;
     const float removalY = bestCarVisual->position.y + OBSTACLE_REMOVAL_DISTANCE;

      auto new_end = std::remove_if(obstacles.begin(), obstacles.end(),
          [&](const std::unique_ptr<Obstacle>& obsPtr) {
              if (!obsPtr) return true;
              return obsPtr->position.y > removalY;
          });
     obstacles.erase(new_end, obstacles.end());

     obstacleRawPtrs.clear();
     obstacleRawPtrs.reserve(obstacles.size());
     for (const auto& obsPtr : obstacles) {
          if(obsPtr) obstacleRawPtrs.push_back(obsPtr.get());
     }

     while (obstacles.size() < NUM_OBSTACLES) {
          const float minW = 20.0f, maxW = 40.0f, minH = 40.0f, maxH = 80.0f;
          const sf::Color obsColor = sf::Color(128, 128, 128);
          const float minGapAdj = 75.0f, minGapSame = 150.0f;

          auto newObstacle = generateSingleObstacle(generationMinY, generationMaxY, minW, maxW, minH, maxH, obsColor, minGapAdj, minGapSame);

          if (newObstacle) {
             obstacleRawPtrs.push_back(newObstacle.get());
             obstacles.push_back(std::move(newObstacle));
          } else {
             break;
          }
     }
}

void Game::updateFocus() {
    if (currentState != GameState::SIMULATION) {
        focusedCar = nullptr;
        return;
    }

    if (!manualNavigationActive) {
        focusedCar = bestCarVisual; // Automatic focus follows best visual car
    } else {
        // Manual Navigation: Validate current selection
        if (navigableCars.empty() || currentNavIndex < 0 || currentNavIndex >= navigableCars.size()) {
            stopManualNavigation();
            focusedCar = bestCarVisual;
        } else {
            Car* potentialFocus = navigableCars[currentNavIndex];
            bool stillValid = false;
            // Check if the car still exists in the main 'cars' list and is not damaged
            for(const auto& carPtr : cars) {
                if(carPtr.get() == potentialFocus && !potentialFocus->isDamaged()) {
                    stillValid = true;
                    break;
                }
            }

            if (stillValid) {
                focusedCar = potentialFocus; // Keep manual focus
            } else {
                // Focused car is no longer valid, remove from navigable list
                std::cout << "Manually focused car (Rank " << currentNavIndex + 1 << ") no longer valid. Updating list." << std::endl;
                navigableCars.erase(navigableCars.begin() + currentNavIndex);

                if (navigableCars.empty()) {
                    stopManualNavigation();
                    focusedCar = bestCarVisual;
                } else {
                    // Select next available car (adjust index)
                    currentNavIndex %= navigableCars.size();
                    focusedCar = navigableCars[currentNavIndex];
                    std::cout << "Auto-switched manual focus to Rank: " << (currentNavIndex + 1) << std::endl;
                }
            }
        }
    }

    // Final fallback: if no focused car determined, try the first car
    if (!focusedCar && !cars.empty() && cars[0]) {
        focusedCar = cars[0].get();
    }
}


void Game::updateStatusPanel() {
     std::ostringstream statusStream;
     statusStream << std::fixed << std::setprecision(1);

    // Display content based on current state
    switch (currentState) {
        case GameState::MENU:
             statusStream << "State: MENU\n";
             statusStream << "-------------\n";
             statusStream << "Select an option using\n";
             statusStream << "UP/DOWN arrows and ENTER.\n\n";
             statusStream << "ESC to exit.";
            break;

        case GameState::HELP:
             statusStream << "State: HELP\n";
             statusStream << "-------------\n";
             statusStream << "Displaying help image.\n\n";
             statusStream << "Press ESC or ENTER\n";
             statusStream << "to return to the menu.";
            break;

        case GameState::SIMULATION:
            { // Scope for simulation variables
                 statusStream << "State: SIMULATION\n";
                 statusStream << "Mode: " << (loadSpecificBrainOnStart ? "Visualization" : "Training") << "\n";
                 statusStream << "-------------\n";

                 int nonDamagedCount = 0;
                 for (const auto& carPtr : cars) {
                     if (carPtr && !carPtr->isDamaged()) {
                         nonDamagedCount++;
                     }
                 }

                statusStream << "Generation: " << generationCount << "\n";
                statusStream << "Time: " << generationClock.getElapsedTime().asSeconds() << "s\n";
                if (isPaused) { statusStream << "\n--- PAUSED ---\n\n"; } else { statusStream << "\n"; }
                statusStream << "Alive: " << nonDamagedCount << " / " << cars.size() << "\n";
                statusStream << "Obstacles: " << obstacles.size() << "\n\n";

                if (manualNavigationActive && !navigableCars.empty() && focusedCar) {
                     int currentRank = -1;
                      // Re-sort temporarily to get correct rank (can be optimized if list order is stable)
                      std::sort(navigableCars.begin(), navigableCars.end(), [](const Car* a, const Car* b) {
                          return a->position.y < b->position.y;
                      });
                      auto it = std::find(navigableCars.begin(), navigableCars.end(), focusedCar);
                      if (it != navigableCars.end()) {
                          currentRank = static_cast<int>(std::distance(navigableCars.begin(), it)) + 1;
                      }
                     statusStream << "Manual Nav: ON (Rank "
                                  << (currentRank != -1 ? std::to_string(currentRank) : "?")
                                  << "/" << navigableCars.size() << ")\n";
                     statusStream << "Focus Y Pos: " << focusedCar->position.y << "\n";
                     statusStream << "Focus Speed: " << focusedCar->getSpeed() << "\n";
                      // statusStream << "Focus Fitness: " << focusedCar->getFitness() << "\n";
                } else if (focusedCar){
                     statusStream << "Manual Nav: OFF\n";
                     statusStream << "Focus Y Pos: " << focusedCar->position.y << "\n";
                     statusStream << "Focus Speed: " << focusedCar->getSpeed() << "\n";
                      // statusStream << "Focus Fitness: " << focusedCar->getFitness() << "\n";
                } else {
                     statusStream << "Manual Nav: OFF\nFocus: N/A\n";
                }

                 statusStream << "\n--- Controls ---\n";
                 statusStream << " P: Pause | R: Reset Gen\n";
                 statusStream << " N/B: Navigate Focus\n";
                 statusStream << " Enter: Stop Navigate\n";
                 statusStream << " Ctrl+S: Save Brain\n";
                 statusStream << " D: Discard Brain(s)\n";
                 statusStream << " ESC: Back to Menu";
            }
            break; // End of SIMULATION case
    }

    statusText.setString(statusStream.str());
}