#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <limits>
#include <cstdio>
#include <optional>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <map>
#include <random>
#include <ctime>

// --- Project Headers ---
#include "Road.hpp"
#include "Car.hpp"
#include "Obstacle.hpp"
#include "Visualizer.hpp"
#include "Utils.hpp"
#include "Network.hpp"
#include "Sensor.hpp"

// --- Constants ---
const std::string BRAIN_FILENAME = "bestBrain.dat";
const int NUM_AI_CARS = 300;
const int NUM_OBSTACLES = 20; // Target number of obstacles on screen
const float START_Y_POSITION = 100.0f;

// --- Function Declarations ---
void populateCarVector(std::vector<std::unique_ptr<Car>>& cars_vec, int N, const Road& road, float startY);
std::vector<std::unique_ptr<Obstacle>> generateObstacles(int N, const Road& road, float minY, float maxY, float minW, float maxW, float minH, float maxH);
void applyBrainsToGeneration(std::vector<std::unique_ptr<Car>>& cars, const NeuralNetwork& bestBrain, int N);

// --- NEW FUNCTION: generateSingleObstacle ---
// Gera UM obstáculo respeitando os existentes e uma faixa Y
std::unique_ptr<Obstacle> generateSingleObstacle(
    const Road& road,
    const std::vector<std::unique_ptr<Obstacle>>& existingObstacles,
    float minY, float maxY,
    float minW, float maxW, float minH, float maxH,
    const sf::Color& color,
    float minVerticalGapAdjacentLane, float minVerticalGapSameLane,
    int maxPlacementRetries = 25)
{
    // Seed only once if needed, or use global rng if defined in Utils.cpp
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> laneDistrib(0, road.laneCount - 1);
    std::uniform_real_distribution<> yDistrib(minY, maxY);
    std::uniform_real_distribution<> wDistrib(minW, maxW);
    std::uniform_real_distribution<> hDistrib(minH, maxH);

    int attempts = 0;
    while (attempts < maxPlacementRetries) {
        attempts++;
        // 1. Gera propriedades do obstáculo potencial
        int potentialLaneIndex = laneDistrib(gen);
        float potentialXPos = road.getLaneCenter(potentialLaneIndex);
        float potentialYPos = yDistrib(gen);
        float potentialWidth = wDistrib(gen);
        float potentialHeight = hDistrib(gen);
        float potentialTop = potentialYPos - potentialHeight / 2.0f;
        float potentialBottom = potentialYPos + potentialHeight / 2.0f;

        // 2. Verifica colisão/sobreposição com obstáculos existentes
        bool collisionFound = false;
        for (const auto& existingObsPtr : existingObstacles) { // Check against ALL existing
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
                 // Use a small tolerance for lane matching
                 if (dist < road.width / (road.laneCount * 2.0f) && dist < minLaneDist) {
                    minLaneDist = dist;
                    existingLaneIndex = l;
                 }
            }
             // If we couldn't determine the lane, we might skip complex spacing check
             // or assume it's in some lane for safety, depending on desired strictness.
             // For now, if no lane match, maybe only check direct overlap.
             // if (existingLaneIndex == -1) continue; // Option: Skip complex spacing if lane unclear

            // Determina a folga vertical necessária baseada na relação das pistas
            float requiredVerticalGap = 0.0f;
            if (potentialLaneIndex == existingLaneIndex && existingLaneIndex != -1) {
                requiredVerticalGap = minVerticalGapSameLane; // Pista igual -> folga maior
            } else if (existingLaneIndex != -1 && std::abs(potentialLaneIndex - existingLaneIndex) == 1) {
                requiredVerticalGap = minVerticalGapAdjacentLane; // Pista adjacente -> folga definida
            } // Else: Pistas não adjacentes ou lane indeterminada, só checa sobreposição direta

             // Checagem de sobreposição vertical INCLUINDO A FOLGA
            bool verticalOverlapWithGap = (potentialTop - requiredVerticalGap < existingBottom) &&
                                          (potentialBottom + requiredVerticalGap > existingTop);

             // Checagem adicional para sobreposição *direta* (mesmo sem folga)
            bool directVerticalOverlap = potentialTop < existingBottom && potentialBottom > existingTop;

            // Verifica colisão se:
            // 1. Há sobreposição vertical com folga E a folga é necessária (mesma pista ou adjacente)
            // OU
            // 2. Há sobreposição direta E estão na *mesma* pista (evitar empilhar exatamente)
            if ((verticalOverlapWithGap && requiredVerticalGap > 0.0f) ||
                (directVerticalOverlap && potentialLaneIndex == existingLaneIndex && existingLaneIndex != -1))
            {
                collisionFound = true;
                break; // Conflito encontrado, tenta gerar nova posição
            }
        } // Fim do loop pelos obstáculos existentes

        // 3. Se não houve colisão/conflito de folga, cria e retorna o novo obstáculo
        if (!collisionFound) {
            // Certifique-se de que Obstacle tem um construtor que atualiza o ID
            return std::make_unique<Obstacle>(potentialXPos, potentialYPos, potentialWidth, potentialHeight, color);
        }
        // Se houve colisão, o loop `while (attempts < maxPlacementRetries)` continuará

    } // Fim do loop de tentativas

    // Se mesmo após as tentativas não foi possível posicionar
    // std::cerr << "Warning: Could not place single obstacle after " << maxPlacementRetries << " attempts." << std::endl;
    return nullptr; // Indica falha
}


// --- Main Application ---
int main() {
    // --- Setup ---
    // Obter resolução do monitor
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    const unsigned int screenWidth = desktopMode.size.x;
    const unsigned int screenHeight = desktopMode.size.y;

    // Definir proporções baseadas nos valores originais (250, 200, 750 -> total 1200)
    const float totalOriginalWidth = 1200.0f;
    const float statusPanelProportion = 250.0f / totalOriginalWidth;
    const float carCanvasProportion = 200.0f / totalOriginalWidth;
    const float networkCanvasProportion = 750.0f / totalOriginalWidth;

    // Calcular larguras reais em pixels
    const float statusPanelWidthActual = screenWidth * statusPanelProportion;
    const float carCanvasWidthActual = screenWidth * carCanvasProportion;
    const float networkCanvasWidthActual = screenWidth * networkCanvasProportion;

    // Criar janela em tela cheia
    sf::RenderWindow window(desktopMode, "Self-Driving Car - Fullscreen Proportional");
    window.setFramerateLimit(60);

    // Views (Tamanho usa pixels calculados, Viewport usa proporções)
    sf::View carView(sf::FloatRect({0.f, 0.f}, {carCanvasWidthActual, (float)screenHeight}));
    carView.setViewport(sf::FloatRect({statusPanelProportion, 0.f}, {carCanvasProportion, 1.f}));

    sf::View networkView(sf::FloatRect({0.f, 0.f}, {networkCanvasWidthActual, (float)screenHeight}));
    networkView.setViewport(sf::FloatRect({statusPanelProportion + carCanvasProportion, 0.f}, {networkCanvasProportion, 1.f}));

    sf::View statusView(sf::FloatRect({0.f, 0.f}, {statusPanelWidthActual, (float)screenHeight}));
    statusView.setViewport(sf::FloatRect({0.f, 0.f}, {statusPanelProportion, 1.f}));

    // Font
    sf::Font font;
    if (!font.openFromFile("assets/LiberationSans-Regular.ttf")) {
        // Tenta um caminho alternativo se estiver rodando do diretório build
        if (!font.openFromFile("../assets/LiberationSans-Regular.ttf")) {
             std::cerr << "Error loading font from assets/ or ../assets/" << std::endl; return 1;
        }
    }


    // Road (Usa largura calculada do canvas do carro)
    Road road(carCanvasWidthActual / 2.0f, carCanvasWidthActual * 0.9f, 3);

    // Status Panel (Usa largura calculada)
    sf::RectangleShape statusPanelBackground(sf::Vector2f(statusPanelWidthActual, (float)screenHeight));
    statusPanelBackground.setFillColor(sf::Color(40, 40, 40));
    statusPanelBackground.setPosition({0.f, 0.f});
    sf::Text statusText(font, "", 16);
    statusText.setFillColor(sf::Color(200, 200, 200));
    statusText.setPosition({15.f, 15.f}); // Manter posição relativa

    // --- Simulation Variables ---
    std::vector<std::unique_ptr<Car>> cars;
    populateCarVector(cars, NUM_AI_CARS, road, START_Y_POSITION);

    Car* bestCarVisual = nullptr;
    Car* focusedCar = nullptr;
    int generationCount = 1;
    bool isPaused = false;
    bool manualNavigationActive = false;
    std::vector<Car*> navigableCars;
    int currentNavIndex = -1;

    // --- Obstacle Setup ---
    // Gera o lote inicial de obstáculos
    auto obstacles = generateObstacles(NUM_OBSTACLES, road, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);
    std::vector<Obstacle*> obstacleRawPtrs;
    obstacleRawPtrs.reserve(obstacles.size());
    for (const auto& obsPtr : obstacles) {
         if(obsPtr) obstacleRawPtrs.push_back(obsPtr.get());
    }

    // --- Brain Management ---
    std::vector<int> networkStructure = {5, 12, 4}; // Estrutura default
    if (!cars.empty() && cars[0]->useBrain) {
        int sensorRays = cars[0]->getSensorRayCount();
        if (sensorRays > 0) { networkStructure[0] = sensorRays; }
        else { std::cerr << "Warning: First AI car has 0 sensor rays! Using default inputs." << std::endl; }
    } else if (!cars.empty()){ std::cerr << "Warning: First car is not AI. Using default network inputs." << std::endl;}
      else { std::cerr << "Warning: No cars generated. Using default network inputs." << std::endl; }

    NeuralNetwork bestBrainOfGeneration(networkStructure);
    if (bestBrainOfGeneration.loadFromFile(BRAIN_FILENAME)) {
        std::cout << "Loaded best brain from: " << BRAIN_FILENAME << std::endl;
        if (bestBrainOfGeneration.levels.empty() || bestBrainOfGeneration.levels.front().inputs.size() != networkStructure[0] || bestBrainOfGeneration.levels.back().outputs.size() != networkStructure.back()) {
            std::cerr << "Warning: Loaded brain structure mismatch! Creating new default brain." << std::endl;
            bestBrainOfGeneration = NeuralNetwork(networkStructure);
        }
    } else { std::cout << "No saved brain found (" << BRAIN_FILENAME << "). Starting fresh." << std::endl; }

    applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS);
    if (!cars.empty()) {
        focusedCar = cars[0].get();
        bestCarVisual = focusedCar;
    }

    sf::Clock clock;
    sf::Clock generationClock;

    // --- Main Loop ---
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();

        // --- Event Handling ---
        while (const auto eventOpt = window.pollEvent()) {
             const sf::Event& event = *eventOpt;
             if (event.is<sf::Event::Closed>()) { window.close(); }
             if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                  bool isCtrlOrCmd = keyPressed->control || keyPressed->system; // Check for Ctrl (Linux/Win) or Cmd (Mac)
                  // Salvar Cérebro (Ctrl+S / Cmd+S)
                  if (keyPressed->code == sf::Keyboard::Key::S && isCtrlOrCmd) {
                       if (bestBrainOfGeneration.saveToFile(BRAIN_FILENAME)) { std::cout << "Saved Best Generation Brain (Ctrl/Cmd+S)" << std::endl; }
                       else { std::cerr << "Error saving brain." << std::endl; }
                  }
                  // Descartar Cérebro Salvo (D) - Removido Ctrl+D para simplificar
                  else if (keyPressed->code == sf::Keyboard::Key::D) {
                       if (std::remove(BRAIN_FILENAME.c_str()) == 0) { std::cout << "Discarded saved brain file." << std::endl; }
                       else { std::cout << "No brain file to discard or error removing." << std::endl; }
                  }
                  // Pausar (P)
                  else if (keyPressed->code == sf::Keyboard::Key::P) {
                      isPaused = !isPaused; std::cout << (isPaused ? "PAUSED" : "RESUMED") << std::endl;
                  }
                  // Reiniciar Geração (R)
                  else if (keyPressed->code == sf::Keyboard::Key::R) {
                      std::cout << "\n--- RESTARTING Generation " << generationCount << " (Manual Reset) ---" << std::endl;
                      for (auto& carPtr : cars) { if (carPtr) carPtr->resetForNewGeneration(START_Y_POSITION, road); }
                      applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS);
                      obstacles = generateObstacles(NUM_OBSTACLES, road, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);
                      obstacleRawPtrs.clear(); for (const auto& obsPtr : obstacles) { if(obsPtr) obstacleRawPtrs.push_back(obsPtr.get()); }
                      manualNavigationActive = false; currentNavIndex = -1; navigableCars.clear();
                      focusedCar = cars.empty() ? nullptr : cars[0].get(); bestCarVisual = focusedCar;
                      clock.restart(); generationClock.restart(); isPaused = false;
                      std::cout << "--- Generation " << generationCount << " Restarted ---\n" << std::endl;
                  }
                 // Navegação Manual (N/B)
                 else if (keyPressed->code == sf::Keyboard::Key::N || keyPressed->code == sf::Keyboard::Key::B) {
                      if (!manualNavigationActive) {
                          manualNavigationActive = true; navigableCars.clear();
                          for (const auto& carPtr : cars) { if (carPtr && !carPtr->isDamaged()) { navigableCars.push_back(carPtr.get()); } }
                          std::sort(navigableCars.begin(), navigableCars.end(), [](const Car* a, const Car* b) { return a->position.y < b->position.y; });
                          currentNavIndex = 0;
                          if (focusedCar) {
                              auto it = std::find(navigableCars.begin(), navigableCars.end(), focusedCar);
                              if (it != navigableCars.end()) { currentNavIndex = static_cast<int>(std::distance(navigableCars.begin(), it)); }
                          }
                          if (navigableCars.empty()) { manualNavigationActive = false; currentNavIndex = -1; std::cout << "No cars to navigate." << std::endl; }
                          else { std::cout << "Manual Nav ON. Cars: " << navigableCars.size() << ". Index: " << currentNavIndex << std::endl; focusedCar = navigableCars[currentNavIndex]; }
                      }
                      if (manualNavigationActive && !navigableCars.empty()) {
                          if (keyPressed->code == sf::Keyboard::Key::N) { currentNavIndex = (currentNavIndex + 1) % navigableCars.size(); }
                          else { currentNavIndex = (currentNavIndex - 1 + navigableCars.size()) % navigableCars.size(); }
                          focusedCar = navigableCars[currentNavIndex];
                          std::cout << "Navigating to Rank: " << (currentNavIndex + 1) << std::endl;
                      }
                  }
                 // Desativar Navegação Manual (Enter)
                 else if (keyPressed->code == sf::Keyboard::Key::Enter) {
                      if (manualNavigationActive) { manualNavigationActive = false; currentNavIndex = -1; navigableCars.clear(); std::cout << "Manual Nav OFF." << std::endl; }
                 }
                 // Fechar Janela (Esc)
                 else if (keyPressed->code == sf::Keyboard::Key::Escape) { window.close(); }
            } // Fim KeyPressed
       } // Fim PollEvent

        // --- Lógica Principal (se não pausado) ---
        int nonDamagedCount = 0;
        if (!isPaused) {
            // Atualiza Carros
            for (auto& carPtr : cars) {
                if(carPtr) {
                    carPtr->update(road, obstacleRawPtrs, deltaTime);
                    if (!carPtr->isDamaged()) {
                        nonDamagedCount++;
                    }
                }
            }

            // --- Lógica de Fim de Geração ---
            bool allCarsEffectivelyDone = (nonDamagedCount == 0);

            // Se a geração acabou
            if (allCarsEffectivelyDone && !cars.empty()) {
                std::cout << "\n--- GENERATION " << generationCount << " ENDED ---" << std::endl;
                 Car* carWithBestFitness = nullptr;
                 float maxFitness = -std::numeric_limits<float>::infinity();
                 for (const auto& carPtr : cars) {
                     if (carPtr) {
                         float currentCarFitness = carPtr->getFitness();
                         if (currentCarFitness > maxFitness) {
                             maxFitness = currentCarFitness;
                             carWithBestFitness = carPtr.get();
                         }
                     }
                 }
                if (carWithBestFitness && carWithBestFitness->brain) {
                    bestBrainOfGeneration = *(carWithBestFitness->brain);
                    std::cout << "Selected best brain (Max Fitness = " << maxFitness << ", Final Y = " << carWithBestFitness->position.y << ")" << std::endl;
                    if (!bestBrainOfGeneration.saveToFile(BRAIN_FILENAME)) { std::cerr << "Error saving brain to " << BRAIN_FILENAME << std::endl; }
                    else { std::cout << "Saved best brain to " << BRAIN_FILENAME << std::endl; }
                } else { std::cout << "No valid best car found by fitness or best car had no brain. Keeping previous generation's brain." << std::endl; }

                std::cout << "--- Preparing Generation " << generationCount + 1 << " ---" << std::endl;
                generationCount++;
                for (auto& carPtr : cars) { if (carPtr) carPtr->resetForNewGeneration(START_Y_POSITION, road); }
                applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS);
                obstacles = generateObstacles(NUM_OBSTACLES, road, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);
                obstacleRawPtrs.clear(); for (const auto& obsPtr : obstacles) { if(obsPtr) obstacleRawPtrs.push_back(obsPtr.get()); }
                manualNavigationActive = false; currentNavIndex = -1; navigableCars.clear();
                focusedCar = cars.empty() ? nullptr : cars[0].get(); bestCarVisual = focusedCar;
                std::cout << "--- GENERATION " << generationCount << " READY --- \n" << std::endl;
                clock.restart(); generationClock.restart();
            }

             // --- START: Infinite Obstacle Logic ---
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

             if (bestCarVisual) {
                 const float obstacleRemovalDistance = 200.0f; // Ajuste conforme necessário
                 const float generationZoneStartY = -250.0f;  // Posição Y relativa mais próxima à frente
                 const float generationZoneEndY = -600.0f;    // Posição Y relativa mais distante à frente
                 const float obstacleMinY = bestCarVisual->position.y + generationZoneEndY;
                 const float obstacleMaxY = bestCarVisual->position.y + generationZoneStartY;

                 // 1. Remover obstáculos muito atrás
                 for (int i = static_cast<int>(obstacles.size()) - 1; i >= 0; --i) {
                     if (!obstacles[i]) continue;
                     if (obstacles[i]->position.y > bestCarVisual->position.y + obstacleRemovalDistance) {
                         Obstacle* rawToRemove = obstacles[i].get();
                         obstacleRawPtrs.erase(std::remove(obstacleRawPtrs.begin(), obstacleRawPtrs.end(), rawToRemove), obstacleRawPtrs.end());
                         obstacles.erase(obstacles.begin() + i);
                     }
                 }

                 // 2. Gerar novos obstáculos se a contagem estiver baixa
                 while (obstacles.size() < NUM_OBSTACLES) {
                      const float minW = 20.0f, maxW = 40.0f, minH = 40.0f, maxH = 80.0f;
                      const sf::Color obsColor = sf::Color(128, 128, 128);
                      const float minGapAdj = 75.0f, minGapSame = 150.0f;
                      auto newObstacle = generateSingleObstacle(road, obstacles, obstacleMinY, obstacleMaxY, minW, maxW, minH, maxH, obsColor, minGapAdj, minGapSame);
                      if (newObstacle) {
                         obstacleRawPtrs.push_back(newObstacle.get());
                         obstacles.push_back(std::move(newObstacle));
                      } else {
                         break; // Para de tentar neste frame se falhar
                      }
                 }
             }
             // --- END: Infinite Obstacle Logic ---

        } // Fim if (!isPaused)

        // --- Seleção do Carro Focado para Visualização ---
        if (!manualNavigationActive) {
            focusedCar = bestCarVisual; // Foco automático segue o melhor visual
        } else {
            if (currentNavIndex >= 0 && currentNavIndex < navigableCars.size()) {
                Car* potentialFocus = navigableCars[currentNavIndex];
                bool stillExistsAndAlive = false;
                for(const auto& carPtr : cars) {
                    if(carPtr.get() == potentialFocus && !potentialFocus->isDamaged()) {
                        stillExistsAndAlive = true; break;
                    }
                }
                if (stillExistsAndAlive) { focusedCar = potentialFocus; }
                else {
                    navigableCars.erase(navigableCars.begin() + currentNavIndex);
                    if (navigableCars.empty()) { manualNavigationActive = false; currentNavIndex = -1; focusedCar = nullptr; }
                    else { currentNavIndex %= navigableCars.size(); focusedCar = navigableCars[currentNavIndex]; }
                }
            } else { manualNavigationActive = false; currentNavIndex = -1; focusedCar = bestCarVisual; }
        }
        if (!focusedCar && !cars.empty() && bestCarVisual) { focusedCar = bestCarVisual; }
        else if (!focusedCar && !cars.empty()) { focusedCar = cars[0].get(); }


        // --- ATUALIZAÇÃO DO TEXTO DO PAINEL ---
        std::ostringstream statusStream;
        statusStream << std::fixed << std::setprecision(1);
        statusStream << "Generation: " << generationCount << "\n";
        statusStream << "Time: " << generationClock.getElapsedTime().asSeconds() << "s\n";
        if (isPaused) { statusStream << "\n--- PAUSED ---\n\n"; } else { statusStream << "\n"; }
        statusStream << "Alive: " << nonDamagedCount << " / " << cars.size() << "\n";
        statusStream << "Obstacles: " << obstacles.size() << "\n\n"; // Mostra contagem atual
        if (manualNavigationActive && !navigableCars.empty() && focusedCar) {
             statusStream << "Manual Nav: ON (Rank " << (currentNavIndex + 1) << ")\n";
             statusStream << "Focus Fitness: " << focusedCar->getFitness() << "\n";
             statusStream << "Focus Y Pos: " << focusedCar->position.y << "\n";
             statusStream << "Focus Speed: " << focusedCar->getSpeed() << "\n";
        } else if (focusedCar){
             statusStream << "Manual Nav: OFF\n";
             statusStream << "Focus Fitness: " << focusedCar->getFitness() << "\n";
             statusStream << "Focus Y Pos: " << focusedCar->position.y << "\n";
             statusStream << "Focus Speed: " << focusedCar->getSpeed() << "\n";
        } else { statusStream << "Manual Nav: OFF\nFocus: N/A\n"; }
        statusText.setString(statusStream.str());

        // --- Drawing ---
        window.clear(sf::Color(100, 100, 100));

        // Desenha Painel de Status
        window.setView(statusView);
        window.draw(statusPanelBackground);
        window.draw(statusText);

        // Desenha Simulação
        if (focusedCar) { carView.setCenter({carCanvasWidthActual / 2.0f, focusedCar->position.y - screenHeight * 0.3f}); }
        else { carView.setCenter({carCanvasWidthActual / 2.0f, screenHeight / 2.0f}); }
        window.setView(carView);
        road.draw(window);
        for (const auto& obsPtr : obstacles) { if(obsPtr) obsPtr->draw(window); }
        for (const auto& carPtr : cars) { if (carPtr && carPtr.get() != focusedCar) carPtr->draw(window, false); }
        if(focusedCar) { focusedCar->draw(window, true); }

        // Desenha Rede Neural
        window.setView(networkView);
        sf::RectangleShape networkBackground(sf::Vector2f(networkCanvasWidthActual, (float)screenHeight));
        networkBackground.setFillColor(sf::Color(60, 60, 60));
        networkBackground.setPosition({0.f, 0.f});
        window.draw(networkBackground);
        NeuralNetwork* brainToDraw = nullptr;
        if (focusedCar && focusedCar->useBrain && focusedCar->brain) { brainToDraw = &(*(focusedCar->brain)); }
        if (brainToDraw && !brainToDraw->levels.empty()) {
            Visualizer::drawNetwork(window, *brainToDraw, font, 0.f, 0.f, networkCanvasWidthActual, (float)screenHeight);
        } else {
            sf::Text noBrainText(font, "No brain to display", 20);
            noBrainText.setFillColor(sf::Color::White);
            noBrainText.setPosition({networkCanvasWidthActual/2.0f - noBrainText.getLocalBounds().size.x/2.0f , screenHeight/2.0f - 10});
            window.draw(noBrainText);
        }

        window.display();
    } // Fim do loop principal

    return 0;
}

// --- Function Definitions ---

// Popula o vetor de carros
void populateCarVector(std::vector<std::unique_ptr<Car>>& cars_vec, int N, const Road& road, float startY) {
    cars_vec.clear();
    cars_vec.reserve(N);
    for (int i = 0; i < N; ++i) {
        cars_vec.push_back(std::make_unique<Car>( road.getLaneCenter(1), startY, 30.0f, 50.0f, ControlType::AI, 3.0f, getRandomColor() ));
    }
    std::cout << "Generated " << cars_vec.size() << " AI cars." << std::endl;
}

// Gera obstáculos iniciais
std::vector<std::unique_ptr<Obstacle>> generateObstacles(int N, const Road& road, float minY, float maxY, float minW, float maxW, float minH, float maxH) {
    std::vector<std::unique_ptr<Obstacle>> obstacles_gen;
    obstacles_gen.reserve(N);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> laneDistrib(0, road.laneCount - 1);
    std::uniform_real_distribution<> yDistrib(minY, maxY);
    std::uniform_real_distribution<> wDistrib(minW, maxW);
    std::uniform_real_distribution<> hDistrib(minH, maxH);
    sf::Color obstacleColor = sf::Color(128, 128, 128);
    const float minVerticalGapAdjacentLane = 75.0f;
    const float minVerticalGapSameLane = 150.0f;
    const int maxPlacementRetries = 25;
    int obstaclesPlaced = 0;
    int totalAttemptsOverall = 0;

    while (obstaclesPlaced < N && totalAttemptsOverall < N * maxPlacementRetries * 5) {
        totalAttemptsOverall++;
        int currentObstacleRetries = 0;
        bool successfullyPlaced = false;
        while (!successfullyPlaced && currentObstacleRetries < maxPlacementRetries) {
            currentObstacleRetries++;
            int potentialLaneIndex = laneDistrib(gen);
            float potentialXPos = road.getLaneCenter(potentialLaneIndex);
            float potentialYPos = yDistrib(gen);
            float potentialWidth = wDistrib(gen);
            float potentialHeight = hDistrib(gen);
            float potentialTop = potentialYPos - potentialHeight / 2.0f;
            float potentialBottom = potentialYPos + potentialHeight / 2.0f;
            bool collisionFound = false;
            for (const auto& existingObsPtr : obstacles_gen) {
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
                   if (dist < road.width / (road.laneCount * 2.0f) && dist < minLaneDist) {
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

               if ((verticalOverlapWithGap && requiredVerticalGap > 0.0f) ||
                   (directVerticalOverlap && potentialLaneIndex == existingLaneIndex && existingLaneIndex != -1))
                {
                    collisionFound = true;
                    break;
                }
            }
            if (!collisionFound) {
                obstacles_gen.push_back(std::make_unique<Obstacle>(potentialXPos, potentialYPos, potentialWidth, potentialHeight, obstacleColor));
                successfullyPlaced = true;
                obstaclesPlaced++;
            }
        }
        if (!successfullyPlaced && currentObstacleRetries >= maxPlacementRetries) {
            // std::cerr << "Warning: Could not place obstacle #" << obstaclesPlaced + 1 << " after " << maxPlacementRetries << " attempts." << std::endl;
        }
    }
    if (obstaclesPlaced < N) {
        std::cerr << "Warning: Only generated " << obstaclesPlaced << " out of " << N << " requested obstacles due to spacing constraints." << std::endl;
    }
    std::cout << "Generated " << obstacles_gen.size() << " initial obstacles." << std::endl;
    return obstacles_gen;
}

// Aplica cérebros
void applyBrainsToGeneration(std::vector<std::unique_ptr<Car>>& cars_gen, const NeuralNetwork& bestBrain, int N) {
     if (cars_gen.empty()) return;
     // Elitismo: Melhor cérebro no primeiro carro
     if (cars_gen[0]->useBrain) {
         if (!cars_gen[0]->brain) cars_gen[0]->brain.emplace(bestBrain); // Cria se não existe
         *(cars_gen[0]->brain) = bestBrain; // Copia pesos/bias
     }
     // Mutação para os outros
     for (int i = 1; i < cars_gen.size(); ++i) {
         if (cars_gen[i]->useBrain) {
             if (!cars_gen[i]->brain) cars_gen[i]->brain.emplace(bestBrain); // Cria se não existe
             *(cars_gen[i]->brain) = bestBrain; // Copia base
             NeuralNetwork::mutate(*(cars_gen[i]->brain), 0.15f); // Muta
         }
     }
}