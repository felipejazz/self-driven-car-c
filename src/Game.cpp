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


Game::Game()
    : road(0, 0),
    font(),
    helpTexture(),
    menuBackgroundTexture(),
    statusText(font, "", 16),
    helpSprite(helpTexture),
    menuBackgroundSprite(menuBackgroundTexture),
    currentState(GameState::MENU),
    selectedMenuItemIndex(0),
    helpTextureLoaded(false),
    menuBackgroundTextureLoaded(false),
    loadSpecificBrainOnStart(false),
    bestCarVisual(nullptr),
    focusedCar(nullptr),
    generationCount(1),
    isPaused(false),
    manualNavigationActive(false),
    currentNavIndex(-1)

{
    std::cout << "Game constructor called." << std::endl;
    loadAssets();

    setupWindowAndViews();
    setupMenu();

    std::cout << "Game setup complete. Starting in MENU state." << std::endl;
}


Game::~Game() {


    std::cout << "Game destructor called." << std::endl;
}


void Game::setupWindowAndViews() {
    std::cout << "Setting up window and views (using old proportions)..." << std::endl;
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


    window.create(sf::VideoMode({screenWidth, screenHeight}), "Self-Driving Car Simulation - V2");
    window.setFramerateLimit(60);
    std::cout << "Window created: " << screenWidth << "x" << screenHeight << std::endl;


    road = Road(carCanvasWidthActual / 2.0f, carCanvasWidthActual * 0.9f, 3);
    std::cout << "Road configured with old width proportions." << std::endl;


    const float statusViewHeightRatio = 0.65f;
    statusView.setSize({statusPanelWidthActual, screenHeight * statusViewHeightRatio});
    statusView.setCenter({statusPanelWidthActual / 2.0f, screenHeight * statusViewHeightRatio / 2.0f});
    statusView.setViewport(sf::FloatRect({0.f, 0.f}, {statusPanelProportion, statusViewHeightRatio}));


    const float graphViewHeightRatio = 1.0f - statusViewHeightRatio;
    graphView.setSize({statusPanelWidthActual, screenHeight * graphViewHeightRatio});
    graphView.setCenter({statusPanelWidthActual / 2.0f, screenHeight * graphViewHeightRatio / 2.0f});
    graphView.setViewport(sf::FloatRect({0.f, statusViewHeightRatio}, {statusPanelProportion, graphViewHeightRatio}));


    carView.setSize({carCanvasWidthActual, (float)screenHeight});
    carView.setCenter({carCanvasWidthActual / 2.0f, (float)screenHeight / 2.0f});
    carView.setViewport(sf::FloatRect({statusPanelProportion, 0.f}, {carCanvasProportion, 1.f}));


    networkView.setSize({networkCanvasWidthActual, (float)screenHeight});
    networkView.setCenter({networkCanvasWidthActual / 2.0f, (float)screenHeight / 2.0f});
    networkView.setViewport(sf::FloatRect({statusPanelProportion + carCanvasProportion, 0.f}, {networkCanvasProportion, 1.f}));

    std::cout << "Views configured using old proportions (Status, Graph, Car, Network)." << std::endl;


    statusPanelBackground.setSize({statusPanelWidthActual, screenHeight * statusViewHeightRatio});
    statusPanelBackground.setFillColor(sf::Color(40, 40, 40));
    statusPanelBackground.setPosition({0.f, 0.f});

    graphPanelBackground.setSize({statusPanelWidthActual, screenHeight * graphViewHeightRatio});
    graphPanelBackground.setFillColor(sf::Color(45, 45, 45));
    graphPanelBackground.setPosition({0.f, screenHeight * statusViewHeightRatio});

    std::cout << "Panel backgrounds configured with old widths." << std::endl;
}


void Game::loadAssets() {
    std::cout << "Loading assets..." << std::endl;


    if (!font.openFromFile("assets/Roboto_Condensed-SemiBold.ttf")) {
        if (!font.openFromFile("../assets/Roboto_Condensed-SemiBold.ttf")) {
            std::cerr << "FATAL ERROR: Could not load font. Exiting." << std::endl;
            throw std::runtime_error("Failed to load font");
        }
    }
    std::cout << "Font loaded successfully." << std::endl;


    std::cout << "Loading menu background texture..." << std::endl;
    menuBackgroundTextureLoaded = false;
    std::cout << "Attempting to load from: assets/home.png" << std::endl;
    if (menuBackgroundTexture.loadFromFile("assets/home.png")) {
        std::cout << ">>> Successfully loaded menu background from: assets/home.png" << std::endl;
        menuBackgroundTextureLoaded = true;
    } else {

        std::cout << "Failed. Attempting to load from: ../assets/home.png" << std::endl;
        if (menuBackgroundTexture.loadFromFile("../assets/home.png")) {
            std::cout << ">>> Successfully loaded menu background from: ../assets/home.png" << std::endl;
            menuBackgroundTextureLoaded = true;
        } else {
            std::cerr << ">>> Warning: Could not load menu background texture from 'assets/home.png' or '../assets/home.png'. Menu background disabled." << std::endl;

        }
    }

    if(menuBackgroundTextureLoaded) {

        sf::Vector2u texSize = menuBackgroundTexture.getSize();
        if (texSize.x > 0 && texSize.y > 0) {
            menuBackgroundSprite.setTexture(menuBackgroundTexture, true);
            menuBackgroundSprite.setOrigin({static_cast<float>(texSize.x) / 2.f, static_cast<float>(texSize.y) / 2.f});
            std::cout << "Menu background sprite configured." << std::endl;
        } else {
            std::cerr << ">>> Warning: Menu background texture loaded but has zero dimensions. Disabling background." << std::endl;
            menuBackgroundTextureLoaded = false;
        }
    }



    std::cout << "Loading help texture..." << std::endl;
    helpTextureLoaded = false;
    if (!helpTexture.loadFromFile("assets/help.png")) {
        if (!helpTexture.loadFromFile("../assets/help.png")) {
            std::cerr << ">>> Warning: Could not load help texture 'assets/help.png' or '../assets/help.png'. Help screen disabled." << std::endl;
        } else {
            helpTextureLoaded = true;
            std::cout << ">>> Successfully loaded help texture from: ../assets/help.png" << std::endl;
        }
    } else {
        helpTextureLoaded = true;
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


void Game::setupMenu() {
    std::cout << "Setting up menu..." << std::endl;
    if (font.getInfo().family.empty()) {
        std::cerr << "Error in setupMenu: Font not loaded. Cannot create menu text." << std::endl;
        throw std::runtime_error("Font not loaded for menu setup");
    }


    menuTexts.clear();


    float startY = window.getSize().y * 0.4f;
    float spacing = 70.0f;
    float centerX = window.getSize().x / 2.0f;

    for (size_t i = 0; i < menuItems.size(); ++i) {


        menuTexts.emplace_back(font, menuItems[i], 40);


        sf::Text& currentText = menuTexts.back();

        currentText.setFillColor(menuNormalColor);


        sf::FloatRect textBounds = currentText.getLocalBounds();
        currentText.setOrigin({textBounds.position.x + textBounds.size.x / 2.0f,
            textBounds.position.y + textBounds.size.y / 2.0f});
        currentText.setPosition({centerX, startY + i * spacing});
    }



    if (!menuTexts.empty()) {

        if (selectedMenuItemIndex < 0 || selectedMenuItemIndex >= menuTexts.size()) {
            selectedMenuItemIndex = 0;
        }
        menuTexts[selectedMenuItemIndex].setFillColor(menuSelectedColor);
        menuTexts[selectedMenuItemIndex].setStyle(sf::Text::Bold);
    } else {
        selectedMenuItemIndex = 0;
    }



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


    averageFitnessHistory.clear();
    bestFitnessHistory.clear();
    mutationRateHistory.clear();
    currentMutationRate = INITIAL_MUTATION_RATE;

    updateGraphData(0.0f, 0.0f, currentMutationRate);


    Car tempCar(0, 0, 30, 50, ControlType::AI);
    int sensorRays = tempCar.getSensorRayCount();
    if (sensorRays <= 0) {
        std::cerr << "Warning: Default car has 0 sensor rays! Using fallback (5)." << std::endl;
        sensorRays = 5;
    }
    networkStructure = {sensorRays, 12, 4};
    std::cout << "Network structure defined: " << sensorRays << "-12-4" << std::endl;


    populateCarVector(NUM_AI_CARS, START_Y_POSITION);


    bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);
    std::string brainFileToLoad;

    if (loadSpecificBrainOnStart) {

        brainFileToLoad = VISUALIZE_BRAIN_FILENAME;
        std::cout << "Attempting to load brain for visualization: " << brainFileToLoad << std::endl;
        std::filesystem::path backupPath(brainFileToLoad);

        if (!std::filesystem::exists(backupPath.parent_path()) && backupPath.has_parent_path()) {
            std::cerr << "Warning: Directory '" << backupPath.parent_path().string() << "' does not exist. Using random brain for visualization." << std::endl;
        } else if (!bestBrainOfGeneration->loadFromFile(brainFileToLoad)) {
            std::cerr << "Warning: Could not load brain file '" << brainFileToLoad << "'. Using random brain for visualization." << std::endl;
        } else {
            std::cout << "Successfully loaded brain for visualization: " << brainFileToLoad << std::endl;

            if (bestBrainOfGeneration->levels.empty() ||
                bestBrainOfGeneration->levels.front().inputs.size() != networkStructure[0] ||
                bestBrainOfGeneration->levels.back().outputs.size() != networkStructure.back()) {
                std::cerr << "Warning: Loaded visualization brain structure mismatch! Reverting to random brain." << std::endl;
                bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);
            }
        }

        applyBrainsToGeneration(NUM_AI_CARS);

    } else {

        brainFileToLoad = DEFAULT_BRAIN_FILENAME;
        std::cout << "Attempting to load default brain for training: " << brainFileToLoad << std::endl;
        std::filesystem::path defaultBrainPath(brainFileToLoad);

        if (!std::filesystem::exists(defaultBrainPath.parent_path()) && defaultBrainPath.has_parent_path()) {
            std::cout << "Default brain directory '" << defaultBrainPath.parent_path().string() << "' not found. Starting training with random brain." << std::endl;
        }
        else if (bestBrainOfGeneration->loadFromFile(brainFileToLoad)) {
            std::cout << "Loaded default brain: " << brainFileToLoad << ". Resuming training." << std::endl;

            if (bestBrainOfGeneration->levels.empty() ||
                bestBrainOfGeneration->levels.front().inputs.size() != networkStructure[0] ||
                bestBrainOfGeneration->levels.back().outputs.size() != networkStructure.back()) {
                std::cerr << "Warning: Loaded default brain structure mismatch! Starting training with random brain." << std::endl;
                bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);
            }
        } else {
            std::cout << "No default brain found (" << brainFileToLoad << ") or directory missing. Starting new training with random brain." << std::endl;
        }

        applyBrainsToGeneration(NUM_AI_CARS);
    }


    generateInitialObstacles(NUM_OBSTACLES, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);


    focusedCar = cars.empty() ? nullptr : cars[0].get();
    bestCarVisual = focusedCar;
    std::cout << "Initial focus set." << std::endl;

    std::cout << "Simulation initialized successfully." << std::endl;
}


void Game::run() {
    std::cout << "Starting game loop..." << std::endl;
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();

        processEvents();


        window.clear(sf::Color::Black);


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

                renderHelp();
                break;
        }


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
            return;
        }


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

                break;

            case GameState::HELP:
                if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {

                    if (keyPressed->code == sf::Keyboard::Key::Escape || keyPressed->code == sf::Keyboard::Key::Enter) {
                        std::cout << "Exiting help screen." << std::endl;
                        currentState = GameState::MENU;
                    }
                }

                if (event.is<sf::Event::Resized>()) {

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
        selectedMenuItemIndex = (selectedMenuItemIndex - 1 + numItems) % numItems;
    } else if (keyEvent.code == sf::Keyboard::Key::Down) {
        selectedMenuItemIndex = (selectedMenuItemIndex + 1) % numItems;
    } else if (keyEvent.code == sf::Keyboard::Key::Enter) {
        std::cout << "Menu selection: " << menuItems[selectedMenuItemIndex] << std::endl;
        switch (selectedMenuItemIndex) {
            case 0:
                loadSpecificBrainOnStart = false;
                initializeSimulation();
                if (window.isOpen()) currentState = GameState::SIMULATION;
                break;
            case 1:
                loadSpecificBrainOnStart = true;
                initializeSimulation();
                if (window.isOpen()) currentState = GameState::SIMULATION;
                break;
            case 2:
                if (helpTextureLoaded) {
                    currentState = GameState::HELP;
                } else {
                    std::cerr << "Cannot show help: Help texture failed to load." << std::endl;

                }
                break;
        }
        return;

    } else if (keyEvent.code == sf::Keyboard::Key::Escape) {
        std::cout << "Escape pressed in menu. Exiting application." << std::endl;
        window.close();
        return;
    }


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
    bool isCtrlOrCmd = keyEvent.control || keyEvent.system;

    switch (keyEvent.code) {
        case sf::Keyboard::Key::S:
            if (isCtrlOrCmd) {
                std::cout << "Save key combination pressed." << std::endl;
                saveBestBrain();
            }
            break;
        case sf::Keyboard::Key::D:
            std::cout << "'Discard Brain' key pressed." << std::endl;
            discardSavedBrain();
            break;
        case sf::Keyboard::Key::P:
            std::cout << "'Pause' key pressed." << std::endl;
            togglePause();
            break;
        case sf::Keyboard::Key::R:
            std::cout << "'Reset Generation' key pressed." << std::endl;
            resetGeneration();
            break;
        case sf::Keyboard::Key::N:
        case sf::Keyboard::Key::B:
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
            isPaused = false;
            stopManualNavigation();


            break;
        default:

            break;
    }
}



void Game::updateMenu() {


}


void Game::updateSimulation(sf::Time deltaTime) {



    int nonDamagedCount = 0;
    bool anyCarMoved = false;
    float totalFitness = 0.0f;
    float maxFitness = -std::numeric_limits<float>::infinity();

    for (auto& carPtr : cars) {
        if (carPtr) {
            float yBefore = carPtr->position.y;
            carPtr->update(road, obstacleRawPtrs, deltaTime);
            if (!carPtr->isDamaged()) {
                nonDamagedCount++;
                if (carPtr->position.y < yBefore) {
                    anyCarMoved = true;
                }

                float currentCarFitness = carPtr->getFitness();
                totalFitness += currentCarFitness;
                if (currentCarFitness > maxFitness) {
                    maxFitness = currentCarFitness;

                }
            } else {
                totalFitness += carPtr->getFitness();
                if (carPtr->getFitness() > maxFitness) {
                    maxFitness = carPtr->getFitness();
                }
            }
        }
    }


    bool allCarsDamaged = (nonDamagedCount == 0);
    bool generationStalled = (!allCarsDamaged && !anyCarMoved && generationClock.getElapsedTime().asSeconds() > 5.0f);
    bool timeLimitExceeded = generationClock.getElapsedTime().asSeconds() > 60.0f;

    if ((allCarsDamaged || generationStalled || timeLimitExceeded) && !cars.empty()) {
        std::cout << "\n--- GENERATION " << generationCount << " ENDED ";
        if (timeLimitExceeded) std::cout << "(Time Limit Exceeded: >60s) ---" << std::endl;
        else if (generationStalled) std::cout << "(Stalled) ---" << std::endl;
        else std::cout << "(All Damaged) ---" << std::endl;


        float averageFitness = cars.empty() ? 0.0f : totalFitness / static_cast<float>(cars.size());

        updateMutationRate();
        updateGraphData(averageFitness, maxFitness, currentMutationRate);
        std::cout << "Stats: Avg Fitness=" << averageFitness
                << ", Best Fitness=" << maxFitness
                << ", Next Mut Rate=" << currentMutationRate << std::endl;


        Car* carWithBestFitness = nullptr;

        for (const auto& carPtr : cars) {
            if (carPtr && carPtr->getFitness() >= maxFitness) {
                carWithBestFitness = carPtr.get();

            }
        }


        if (carWithBestFitness && carWithBestFitness->brain) {
            *bestBrainOfGeneration = *(carWithBestFitness->brain);
            std::cout << "Selected best brain (Fitness: " << maxFitness
                    << ", Y: " << carWithBestFitness->position.y << ")" << std::endl;
            if (!loadSpecificBrainOnStart) {
                saveBestBrain();
            } else {
                std::cout << "(Visualization mode: Not saving brain)" << std::endl;
            }
        } else {
            std::cout << "No valid best car/brain found for this generation. Keeping previous best brain." << std::endl;
        }


        if (!loadSpecificBrainOnStart) {
            std::cout << "--- Preparing Next Training Generation " << generationCount + 1 << " ---" << std::endl;
        } else {
            std::cout << "--- Resetting Visualization ---" << std::endl;
        }
        resetGeneration();

    } else if (!allCarsDamaged) {
        manageInfiniteObstacles();
    }

    updateFocus();
    updateStatusPanel();
}

void Game::updateMutationRate() {
    if (loadSpecificBrainOnStart) {
        currentMutationRate = 0.0f;
        return;
    }

    currentMutationRate = MIN_MUTATION_RATE +
                        (INITIAL_MUTATION_RATE - MIN_MUTATION_RATE) *
                        std::exp(-MUTATION_DECAY_FACTOR * static_cast<float>(generationCount));


    currentMutationRate = std::max(MIN_MUTATION_RATE, currentMutationRate);
}
void Game::updateGraphData(float avgFit, float bestFit, float mutRate) {
    averageFitnessHistory.push_back(avgFit);
    bestFitnessHistory.push_back(bestFit);
    mutationRateHistory.push_back(mutRate);


    while (averageFitnessHistory.size() > MAX_HISTORY_POINTS) {
        averageFitnessHistory.pop_front();
    }
    while (bestFitnessHistory.size() > MAX_HISTORY_POINTS) {
        bestFitnessHistory.pop_front();
    }
    while (mutationRateHistory.size() > MAX_HISTORY_POINTS) {
        mutationRateHistory.pop_front();
    }
}


void Game::renderMenu() {
    window.setView(window.getDefaultView());


    if (menuBackgroundTextureLoaded) {
        sf::Vector2u winSize = window.getSize();
        sf::Vector2u texSize = menuBackgroundTexture.getSize();

        if (texSize.x > 0 && texSize.y > 0) {
            float scaleX = static_cast<float>(winSize.x) / texSize.x;
            float scaleY = static_cast<float>(winSize.y) / texSize.y;
            float scale = std::min(scaleX, scaleY);

            menuBackgroundSprite.setScale({scaleX, scaleY});
            menuBackgroundSprite.setPosition({static_cast<float>(winSize.x) / 2.f, static_cast<float>(winSize.y) / 2.f});

            std::cout << "--- DEBUG renderMenu ---" << std::endl;
            std::cout << "Window Size: (" << winSize.x << ", " << winSize.y << ")" << std::endl;
            std::cout << "Texture Size: (" << texSize.x << ", " << texSize.y << ")" << std::endl;
            std::cout << "Calculated Scale: " << scale << std::endl;
            std::cout << "Drawing BG Sprite - Pos: (" << menuBackgroundSprite.getPosition().x << ", " << menuBackgroundSprite.getPosition().y << ")"
                    << " Scale: (" << menuBackgroundSprite.getScale().x << ", " << menuBackgroundSprite.getScale().y << ")" << std::endl;
            const sf::Color& spriteColor = menuBackgroundSprite.getColor();
            std::cout << "Drawing BG Sprite - Color: (" << (int)spriteColor.r << "," << (int)spriteColor.g << "," << (int)spriteColor.b << "," << (int)spriteColor.a << ")" << std::endl;
            const sf::IntRect& texRect = menuBackgroundSprite.getTextureRect();

            std::cout << "Drawing BG Sprite - TexRect: (L:" << texRect.position.x << ", T:" << texRect.size.x << ", W:" << texRect.position.y << ", H:" << texRect.size.y << ")" << std::endl;


            const sf::Texture* pTextureFromSprite = &(menuBackgroundSprite.getTexture());


            std::cout << "Drawing BG Sprite - Actual Texture Pointer Addr: " << static_cast<const void*>(pTextureFromSprite) << std::endl;
            std::cout << "Drawing BG Sprite - Expected menuBackgroundTexture Addr: " << static_cast<const void*>(&menuBackgroundTexture) << std::endl;


            if (pTextureFromSprite) {
                if (pTextureFromSprite == &menuBackgroundTexture) {
                    std::cout << "Drawing BG Sprite - Texture Pointer Check: OK (matches menuBackgroundTexture)" << std::endl;
                } else {
                    std::cout << "Drawing BG Sprite - Texture Pointer Check: WARNING (Pointer exists, but doesn't match menuBackgroundTexture!)" << std::endl;
                }
            } else {
                std::cout << "Drawing BG Sprite - Texture Pointer Check: ERROR (nullptr!)" << std::endl;
            }
            std::cout << "--- END DEBUG renderMenu ---" << std::endl;


            window.draw(menuBackgroundSprite);

        } else {
            std::cout << "--- DEBUG renderMenu: Texture size is zero, skipping background draw. ---" << std::endl;
        }
    } else {
        std::cout << "--- DEBUG renderMenu: menuBackgroundTextureLoaded is false, skipping background draw. ---" << std::endl;
    }



    sf::Text titleText(font, "", 60);
    titleText.setFillColor(menuTitleColor);
    titleText.setStyle(sf::Text::Bold | sf::Text::Underlined);
    sf::FloatRect titleBounds = titleText.getLocalBounds();
    titleText.setOrigin({titleBounds.position.x + titleBounds.size.x / 2.0f, titleBounds.position.y + titleBounds.size.y / 2.0f});
    titleText.setPosition({window.getSize().x / 2.0f, window.getSize().y * 0.2f});
    window.draw(titleText);


    for (const auto& text : menuTexts) {
        window.draw(text);
    }


    sf::Text instructionText(font, "Use UP/DOWN arrows and ENTER to select. ESC to exit.", 18);
    instructionText.setFillColor(sf::Color(180, 180, 180));
    sf::FloatRect instBounds = instructionText.getLocalBounds();

    instructionText.setOrigin({instBounds.position.x + instBounds.size.x / 2.0f, instBounds.position.y + instBounds.size.y / 2.0f});
    instructionText.setPosition({window.getSize().x / 2.0f, window.getSize().y * 0.9f});
    window.draw(instructionText);
}

void Game::renderHelp() {
    window.setView(window.getDefaultView());

    if (helpTextureLoaded) {

        sf::Vector2u texSize = helpTexture.getSize();
        sf::Vector2u winSize = window.getSize();
        helpSprite.setPosition({winSize.x / 2.f, winSize.y / 2.f});
        float scaleX = (float)winSize.x / texSize.x * 0.9f;
        float scaleY = (float)winSize.y / texSize.y * 0.9f;
        float scale = std::min({scaleX, scaleY, 1.0f});
        helpSprite.setScale({scale, scale});

        window.draw(helpSprite);


        sf::Text returnText(font, "Press ESC or Enter to return to menu", 20);
        returnText.setFillColor(sf::Color(200, 200, 200, 220));
        sf::FloatRect bounds = returnText.getLocalBounds();
        returnText.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
        returnText.setPosition({window.getSize().x / 2.f, window.getSize().y * 0.95f});
        window.draw(returnText);

    } else {

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

    window.setView(statusView);
    window.draw(statusPanelBackground);
    window.draw(statusText);


    window.setView(graphView);
    window.draw(graphPanelBackground);
    renderGraphs();


    if (focusedCar) {
        float viewCenterX = carView.getSize().x / 2.0f;
        float targetY = focusedCar->position.y - window.getSize().y * 0.3f;
        float currentCenterY = carView.getCenter().y;
        float newCenterY = lerp(currentCenterY, targetY, 0.05f);
        carView.setCenter({viewCenterX, newCenterY});
    } else {
        carView.setCenter({carView.getSize().x / 2.0f, START_Y_POSITION - window.getSize().y * 0.3f});
    }
    window.setView(carView);

    road.draw(window);
    for (const auto& obsPtr : obstacles) { if (obsPtr) obsPtr->draw(window); }
    for (const auto& carPtr : cars) {
        if (carPtr && carPtr.get() != focusedCar) {
            carPtr->draw(window, false);
        }
    }
    if (focusedCar) {
        focusedCar->draw(window, true);
    }



    window.setView(networkView);

    sf::RectangleShape networkBackground(networkView.getSize());
    networkBackground.setFillColor(sf::Color(60, 60, 60));
    networkBackground.setPosition({0.f, 0.f});
    window.draw(networkBackground);

    NeuralNetwork* brainToDraw = nullptr;
    if (focusedCar && focusedCar->useBrain && focusedCar->brain) {
        brainToDraw = &(*(focusedCar->brain));
    }

    if (brainToDraw && !brainToDraw->levels.empty()) {
        Visualizer::drawNetwork(window, *brainToDraw, font,
                                0.f, 0.f,
                                networkView.getSize().x, networkView.getSize().y);
    } else {
        sf::Text noBrainText(font, "No brain to display", 20);
        noBrainText.setFillColor(sf::Color::White);
        sf::FloatRect textBounds = noBrainText.getLocalBounds();
        noBrainText.setOrigin({textBounds.position.x + textBounds.size.y / 2.0f, textBounds.position.y + textBounds.size.y / 2.0f});
        noBrainText.setPosition({networkView.getSize().x / 2.0f, networkView.getSize().y / 2.0f});
        window.draw(noBrainText);
    }
}


void Game::resetGeneration() {
    if (!loadSpecificBrainOnStart) {
        std::cout << "--- RESETTING Training Generation " << generationCount << " ---" << std::endl;
        generationCount++;
    } else {
        std::cout << "--- Resetting Visualization State ---" << std::endl;

    }


    for (auto& carPtr : cars) {
        if (carPtr) carPtr->resetForNewGeneration(START_Y_POSITION, road);
    }


    applyBrainsToGeneration(NUM_AI_CARS);


    obstacles.clear();
    obstacleRawPtrs.clear();
    generateInitialObstacles(NUM_OBSTACLES, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);


    manualNavigationActive = false;
    currentNavIndex = -1;
    navigableCars.clear();

    focusedCar = cars.empty() ? nullptr : cars[0].get();
    bestCarVisual = focusedCar;


    generationClock.restart();
    isPaused = false;

    std::cout << "--- Generation " << generationCount << (loadSpecificBrainOnStart ? " (Visualization)" : "") << " Ready --- \n" << std::endl;
}


void Game::renderGraphs() {
    sf::Vector2f viewSize = graphView.getSize();
    float width = viewSize.x;
    float height = viewSize.y;
    float margin = 20.0f;
    float graphWidth = width - 2 * margin;
    float graphHeight = height - 2 * margin;
    float graphLeft = margin;
    float graphTop = margin;
    float graphBottom = graphTop + graphHeight;
    float graphRight = graphLeft + graphWidth;

    size_t dataCount = averageFitnessHistory.size();
    if (dataCount < 2) return;


    float minFit = std::numeric_limits<float>::max();
    float maxFit = -std::numeric_limits<float>::max();
    float minMut = std::numeric_limits<float>::max();
    float maxMut = -std::numeric_limits<float>::max();


    auto avgFitBegin = averageFitnessHistory.begin();
    auto avgFitEnd = averageFitnessHistory.end();
    auto bestFitBegin = bestFitnessHistory.begin();
    auto bestFitEnd = bestFitnessHistory.end();
    auto mutRateBegin = mutationRateHistory.begin();
    auto mutRateEnd = mutationRateHistory.end();

    for (auto it = bestFitBegin; it != bestFitEnd; ++it) {
        minFit = std::min(minFit, *it);
        maxFit = std::max(maxFit, *it);
    }
     for (auto it = avgFitBegin; it != avgFitEnd; ++it) {
        minFit = std::min(minFit, *it);

    }
     for (auto it = mutRateBegin; it != mutRateEnd; ++it) {
        minMut = std::min(minMut, *it);
        maxMut = std::max(maxMut, *it);
    }



    if (maxFit <= minFit) maxFit = minFit + 1.0f;
    if (maxMut <= minMut) maxMut = minMut + 0.01f;


    auto getY = [&](float value, float minValue, float maxValue) {
        if (maxValue == minValue) return graphBottom;
        float normalized = (value - minValue) / (maxValue - minValue);
        return graphBottom - normalized * graphHeight;
    };


    sf::VertexArray axes(sf::PrimitiveType::Lines);
    axes.append({{graphLeft, graphTop}, sf::Color(100, 100, 100)});
    axes.append({{graphRight, graphTop}, sf::Color(100, 100, 100)});
    axes.append({{graphLeft, graphBottom}, sf::Color(100, 100, 100)});
    axes.append({{graphRight, graphBottom}, sf::Color(100, 100, 100)});
    axes.append({{graphLeft, graphTop}, sf::Color(100, 100, 100)});
    axes.append({{graphLeft, graphBottom}, sf::Color(100, 100, 100)});
    window.draw(axes);


    sf::VertexArray avgFitLine(sf::PrimitiveType::LineStrip);
    sf::VertexArray bestFitLine(sf::PrimitiveType::LineStrip);
    sf::VertexArray mutRateLine(sf::PrimitiveType::LineStrip);

    for (size_t i = 0; i < dataCount; ++i) {
        float xPos = graphLeft + (static_cast<float>(i) / static_cast<float>(dataCount - 1)) * graphWidth;

        if (i < averageFitnessHistory.size()) {
            float yPosAvg = getY(averageFitnessHistory[i], minFit, maxFit);
            avgFitLine.append({{xPos, yPosAvg}, sf::Color::Yellow});
        }
        if (i < bestFitnessHistory.size()) {
            float yPosBest = getY(bestFitnessHistory[i], minFit, maxFit);
            bestFitLine.append({{xPos, yPosBest}, sf::Color::Cyan});
        }
        if (i < mutationRateHistory.size()) {
            float yPosMut = getY(mutationRateHistory[i], minMut, maxMut);
            mutRateLine.append({{xPos, yPosMut}, sf::Color::Magenta});
        }
    }

    if (avgFitLine.getVertexCount() > 0) window.draw(avgFitLine);
    if (bestFitLine.getVertexCount() > 0) window.draw(bestFitLine);
    if (mutRateLine.getVertexCount() > 0) window.draw(mutRateLine);


    float legendX = graphRight + 5;
    float legendYStart = graphTop + 5;
    float legendSpacing = 15.0f;
    unsigned int legendCharSize = 10;

    sf::Text avgFitLegend(font, "Avg Fitness", legendCharSize);
    avgFitLegend.setFillColor(sf::Color::Yellow);
    avgFitLegend.setPosition({graphLeft + 5, graphTop + 5});

    sf::Text bestFitLegend(font, "Best Fitness", legendCharSize);
    bestFitLegend.setFillColor(sf::Color::Cyan);
    bestFitLegend.setPosition({graphLeft + 5, graphTop + 5 + legendSpacing});

    sf::Text mutRateLegend(font, "Mutation Rate", legendCharSize);
    mutRateLegend.setFillColor(sf::Color::Magenta);
    mutRateLegend.setPosition({graphLeft + 5, graphTop + 5 + 2 * legendSpacing});

    window.draw(avgFitLegend);
    window.draw(bestFitLegend);
    window.draw(mutRateLegend);
}
void Game::discardSavedBrain() {
    std::cout << "Attempting to discard saved brain file(s)..." << std::endl;
    bool discardedDefault = false;
    bool discardedVis = false;


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

        if (currentState == GameState::SIMULATION && bestBrainOfGeneration) {
            std::cout << "Resetting current in-memory brain to random." << std::endl;
            bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);

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


void Game::startManualNavigation() {
    if (currentState != GameState::SIMULATION) return;

    manualNavigationActive = true;
    navigableCars.clear();


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


    std::sort(navigableCars.begin(), navigableCars.end(), [](const Car* a, const Car* b) {
        return a->position.y < b->position.y;
    });


    currentNavIndex = 0;
    if (focusedCar) {
        auto it = std::find(navigableCars.begin(), navigableCars.end(), focusedCar);
        if (it != navigableCars.end()) {
            currentNavIndex = static_cast<int>(std::distance(navigableCars.begin(), it));
        } else {
            focusedCar = navigableCars[currentNavIndex];
        }
    } else {
        focusedCar = navigableCars[currentNavIndex];
    }

    std::cout << "Manual Navigation ON. Cars available: " << navigableCars.size()
            << ". Focused Rank: " << (currentNavIndex + 1) << std::endl;
}

void Game::navigateManual(sf::Keyboard::Key key) {
    if (!manualNavigationActive || navigableCars.empty() || currentState != GameState::SIMULATION) return;

    int numNavigable = static_cast<int>(navigableCars.size());
    if (key == sf::Keyboard::Key::N) {
        currentNavIndex = (currentNavIndex + 1) % numNavigable;
    } else if (key == sf::Keyboard::Key::B) {
        currentNavIndex = (currentNavIndex - 1 + numNavigable) % numNavigable;
    } else {
        return;
    }

    focusedCar = navigableCars[currentNavIndex];
    std::cout << "Navigated focus to Rank: " << (currentNavIndex + 1) << std::endl;

}

void Game::stopManualNavigation() {
    if (currentState != GameState::SIMULATION) return;

    if (manualNavigationActive) {
        manualNavigationActive = false;
        currentNavIndex = -1;
        navigableCars.clear();
        std::cout << "Manual Navigation OFF." << std::endl;

    }
}


void Game::populateCarVector(int N, float startY) {
    cars.clear();
    cars.reserve(N);
    std::cout << "Generating " << N << " AI cars..." << std::endl;
    for (int i = 0; i < N; ++i) {
        cars.push_back(std::make_unique<Car>(
            road.getLaneCenter(1),
            startY,
            30.0f, 50.0f,
            ControlType::AI,
            3.0f,
            getRandomColor()
        ));
    }
    std::cout << cars.size() << " AI cars generated." << std::endl;
}

void Game::generateInitialObstacles(int N, float minY, float maxY, float minW, float maxW, float minH, float maxH) {
    obstacles.clear();
    obstacleRawPtrs.clear();
    obstacles.reserve(N);
    std::cout << "Generating " << N << " initial obstacles between Y=" << minY << " and Y=" << maxY << "..." << std::endl;

    const sf::Color obstacleColor = sf::Color(128, 128, 128);
    const float minVerticalGapAdjacentLane = 60.0f;
    const float minVerticalGapSameLane = 100.0f;
    int obstaclesPlaced = 0;
    int totalAttemptsOverall = 0;
    const int maxTotalAttempts = N * 50;

    while (obstaclesPlaced < N && totalAttemptsOverall < maxTotalAttempts) {
        totalAttemptsOverall++;
        auto newObstacle = generateSingleObstacle(
            minY, maxY, minW, maxW, minH, maxH, obstacleColor,
            minVerticalGapAdjacentLane, minVerticalGapSameLane, 50
        );

        if (newObstacle) {
            obstacleRawPtrs.push_back(newObstacle.get());
            obstacles.push_back(std::move(newObstacle));
            obstaclesPlaced++;
        }
    }

    if (obstaclesPlaced < N) {
        std::cerr << "Warning: Only generated " << obstaclesPlaced << "/" << N
                << " initial obstacles due to spacing constraints or max attempts." << std::endl;
    }
    std::cout << obstacles.size() << " initial obstacles placed." << std::endl;
}
void Game::saveBestBrain() {
    if (loadSpecificBrainOnStart) {
        std::cout << "(Save disabled in Visualization mode)" << std::endl;
        return;
    }

    if (bestBrainOfGeneration) {
        std::filesystem::path filePath(DEFAULT_BRAIN_FILENAME);
        std::filesystem::path dirPath = filePath.parent_path();


        if (!dirPath.empty() && !std::filesystem::exists(dirPath)) {
            try {
                if (std::filesystem::create_directories(dirPath)) {
                    std::cout << "Created directory: " << dirPath.string() << std::endl;
                } else {
                     std::cerr << "Warning: Failed to create directory (unknown reason): " << dirPath.string() << std::endl;

                }
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << "Error creating directory '" << dirPath.string() << "': " << e.what() << std::endl;
                return;
            }
        }


        if (bestBrainOfGeneration->saveToFile(DEFAULT_BRAIN_FILENAME)) {
            std::cout << "Saved Best Generation Brain to " << DEFAULT_BRAIN_FILENAME << std::endl;
        } else {
            std::cerr << "Error saving brain to " << DEFAULT_BRAIN_FILENAME << std::endl;
        }
    } else {
        std::cerr << "No best brain available to save." << std::endl;
    }
}


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

        int potentialLaneIndex = getRandomInt(0, road.laneCount - 1);
        float potentialYPos = getRandomFloat(minY, maxY);
        float potentialWidth = getRandomFloat(minW, maxW);
        float potentialHeight = getRandomFloat(minH, maxH);


        float laneWidth = road.width / static_cast<float>(road.laneCount);
        float laneLeft = road.left + potentialLaneIndex * laneWidth;
        float laneRight = laneLeft + laneWidth;


        float minCenterX = laneLeft + potentialWidth / 2.0f;
        float maxCenterX = laneRight - potentialWidth / 2.0f;

        float potentialXPos;
        if (maxCenterX <= minCenterX) {

            potentialXPos = road.getLaneCenter(potentialLaneIndex);
        } else {

            potentialXPos = getRandomFloat(minCenterX, maxCenterX);
        }



        float potentialTop = potentialYPos - potentialHeight / 2.0f;
        float potentialBottom = potentialYPos + potentialHeight / 2.0f;


        bool collisionFound = false;
        for (const auto& existingObsPtr : obstacles) {
            if (!existingObsPtr) continue;


            float existingXPos = existingObsPtr->position.x;
            float existingYPos = existingObsPtr->position.y;
            float existingHeight = existingObsPtr->height;
            float existingTop = existingYPos - existingHeight / 2.0f;
            float existingBottom = existingYPos + existingHeight / 2.0f;


            int existingLaneIndex = -1;
            float minLaneDist = std::numeric_limits<float>::max();
            for (int l = 0; l < road.laneCount; ++l) {
                float dist = std::abs(existingXPos - road.getLaneCenter(l));
                if (dist < (laneWidth / 2.0f) && dist < minLaneDist) {
                    minLaneDist = dist;
                    existingLaneIndex = l;
                }
            }


            float requiredVerticalGap = 0.0f;
            if (potentialLaneIndex == existingLaneIndex && existingLaneIndex != -1) {
                requiredVerticalGap = minVerticalGapSameLane;
            } else if (existingLaneIndex != -1 && std::abs(potentialLaneIndex - existingLaneIndex) == 1) {
                requiredVerticalGap = minVerticalGapAdjacentLane;
            }


            bool verticalOverlapWithGap = (potentialTop - requiredVerticalGap < existingBottom) &&
                                        (potentialBottom + requiredVerticalGap > existingTop);


            bool directVerticalOverlap = potentialTop < existingBottom && potentialBottom > existingTop;


            if ((directVerticalOverlap && potentialLaneIndex == existingLaneIndex && existingLaneIndex != -1) ||
                (verticalOverlapWithGap && requiredVerticalGap > 0.0f))
            {
                collisionFound = true;
                break;
            }
        }


        if (!collisionFound) {
            return std::make_unique<Obstacle>(potentialXPos, potentialYPos, potentialWidth, potentialHeight, color);
        }


    }


    return nullptr;
}

void Game::applyBrainsToGeneration(int N) {
    if (cars.empty() || !bestBrainOfGeneration) {
        std::cerr << "Error in applyBrainsToGeneration: No cars or no base brain available." << std::endl;
        return;
    }

    if (loadSpecificBrainOnStart) {

        std::cout << "Applying visualized brain to all cars (no mutation)." << std::endl;
        for (int i = 0; i < cars.size(); ++i) {
            if (cars[i] && cars[i]->useBrain) {
                if (!cars[i]->brain) {
                    cars[i]->brain.emplace(*bestBrainOfGeneration);
                } else {
                    *(cars[i]->brain) = *bestBrainOfGeneration;
                }

                NeuralNetwork::mutate(*(cars[i]->brain), 0.0f);
            }
        }
    } else {

        std::cout << "Applying training brain (elite + mutations using rate " << currentMutationRate << ") to gen " << generationCount << "." << std::endl;

        if (cars[0] && cars[0]->useBrain) {
            if (!cars[0]->brain) {
                cars[0]->brain.emplace(*bestBrainOfGeneration);
            } else {
                *(cars[0]->brain) = *bestBrainOfGeneration;
            }


        }


        for (int i = 1; i < cars.size(); ++i) {
            if (cars[i] && cars[i]->useBrain) {
                if (!cars[i]->brain) {
                    cars[i]->brain.emplace(*bestBrainOfGeneration);
                } else {
                    *(cars[i]->brain) = *bestBrainOfGeneration;
                }

                NeuralNetwork::mutate(*(cars[i]->brain), currentMutationRate);
            }
        }
    }
}



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
        focusedCar = bestCarVisual;
    } else {

        if (navigableCars.empty() || currentNavIndex < 0 || currentNavIndex >= navigableCars.size()) {
            stopManualNavigation();
            focusedCar = bestCarVisual;
        } else {
            Car* potentialFocus = navigableCars[currentNavIndex];
            bool stillValid = false;

            for(const auto& carPtr : cars) {
                if(carPtr.get() == potentialFocus && !potentialFocus->isDamaged()) {
                    stillValid = true;
                    break;
                }
            }

            if (stillValid) {
                focusedCar = potentialFocus;
            } else {

                std::cout << "Manually focused car (Rank " << currentNavIndex + 1 << ") no longer valid. Updating list." << std::endl;
                navigableCars.erase(navigableCars.begin() + currentNavIndex);

                if (navigableCars.empty()) {
                    stopManualNavigation();
                    focusedCar = bestCarVisual;
                } else {

                    currentNavIndex %= navigableCars.size();
                    focusedCar = navigableCars[currentNavIndex];
                    std::cout << "Auto-switched manual focus to Rank: " << (currentNavIndex + 1) << std::endl;
                }
            }
        }
    }


    if (!focusedCar && !cars.empty() && cars[0]) {
        focusedCar = cars[0].get();
    }
}


void Game::updateStatusPanel() {
    std::ostringstream statusStream;
    statusStream << std::fixed << std::setprecision(1);


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
            {
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

                } else if (focusedCar){
                    statusStream << "Manual Nav: OFF\n";
                    statusStream << "Focus Y Pos: " << focusedCar->position.y << "\n";
                    statusStream << "Focus Speed: " << focusedCar->getSpeed() << "\n";

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
            break;
    }

    statusText.setString(statusStream.str());
}