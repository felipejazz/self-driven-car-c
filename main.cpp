// main.cpp (COMPLETO E MODIFICADO com painel de status, pausa, delete e restart)

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <limits>
#include <cstdio>    // Para std::remove
#include <optional>
#include <iomanip>   // Para std::fixed, std::setprecision
#include <sstream>   // Para std::ostringstream
#include <cmath>     // Para std::abs, M_PI (se Utils não definir)

// --- Project Headers ---
#include "Road.hpp"
#include "Car.hpp"
#include "Visualizer.hpp"
#include "Utils.hpp"
#include "Network.hpp"
#include "Sensor.hpp"

// --- Constants ---
const std::string BRAIN_FILENAME = "bestBrain.dat";
const int NUM_AI_CARS = 1000;

// --- Function Declarations ---
std::vector<std::unique_ptr<Car>> generateCars(int N, const Road& road, float startY = 100.0f);
void resetTraffic(std::vector<std::unique_ptr<Car>>& traffic, const Road& road);
void applyBrainsToGeneration(std::vector<std::unique_ptr<Car>>& cars, const NeuralNetwork& bestBrain, int N);

// --- Main Application ---
int main() {
    // --- Setup (Window, Views, Font, Road) ---
    const unsigned int statusPanelWidth = 250;
    const unsigned int carCanvasWidth = 200;
    const unsigned int networkCanvasWidth = 750;
    const unsigned int windowWidth = statusPanelWidth + carCanvasWidth + networkCanvasWidth;
    const unsigned int windowHeight = 800;

    sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), "Self-Driving Car - C++/SFML (P=Pause, D=Discard Brain, R=Restart Gen)"); // Atualiza título
    window.setFramerateLimit(60);

    // Views e Viewports (sem alterações)
    sf::View carView(sf::FloatRect({0.f, 0.f}, {(float)carCanvasWidth, (float)windowHeight}));
    carView.setViewport(sf::FloatRect({(float)statusPanelWidth / windowWidth, 0.f}, {(float)carCanvasWidth / windowWidth, 1.f}));
    sf::View networkView(sf::FloatRect({0.f, 0.f}, {(float)networkCanvasWidth, (float)windowHeight}));
    networkView.setViewport(sf::FloatRect({(float)(statusPanelWidth + carCanvasWidth) / windowWidth, 0.f}, {(float)networkCanvasWidth / windowWidth, 1.f}));

    // Font (sem alterações)
    sf::Font font;
    if (!font.openFromFile("assets/LiberationSans-Regular.ttf")) {
        std::cerr << "Error loading font: assets/LiberationSans-Regular.ttf" << std::endl;
        return 1;
    }

    // Road (sem alterações)
    Road road(carCanvasWidth / 2.0f, carCanvasWidth * 0.9f, 3);

    // Painel de Status (sem alterações)
    sf::RectangleShape statusPanelBackground(sf::Vector2f((float)statusPanelWidth, (float)windowHeight));
    statusPanelBackground.setFillColor(sf::Color(40, 40, 40));
    statusPanelBackground.setPosition({0.f, 0.f});
    sf::Text statusText(font, "", 16);
    statusText.setFillColor(sf::Color(200, 200, 200));
    statusText.setPosition({15.f, 15.f});
    // --- FIM PAINEL DE STATUS ---

    // --- End Setup ---

    // --- Simulation Variables ---
    auto cars = generateCars(NUM_AI_CARS, road);
    Car* bestCar = nullptr;
    int generationCount = 1;
    bool isPaused = false; // *** NOVO: Variável de estado de pausa ***

    // --- Traffic Setup (sem alterações) ---
    std::vector<std::unique_ptr<Car>> traffic;
    resetTraffic(traffic, road);
    std::vector<Car*> trafficRawPtrs;
    trafficRawPtrs.reserve(traffic.size());
    for (const auto& carPtr : traffic) { trafficRawPtrs.push_back(carPtr.get()); }
    // --- End Traffic Setup ---

    // --- Brain Management (sem alterações) ---
    std::vector<int> networkStructure = {5, 6, 5};
    if (!cars.empty() && cars[0]->getSensorRayCount() > 0) {
        networkStructure[0] = cars[0]->getSensorRayCount();
    } else if (!cars.empty() && cars[0]->useBrain) {
         std::cerr << "Warning: First AI car has 0 sensor rays detected! Using default input size." << std::endl;
    }
    NeuralNetwork bestBrainOfGeneration(networkStructure);
    if (bestBrainOfGeneration.loadFromFile(BRAIN_FILENAME)) {
        std::cout << "Loaded best brain from previous session: " << BRAIN_FILENAME << std::endl;
        if (bestBrainOfGeneration.levels.empty() || bestBrainOfGeneration.levels.front().inputs.size() != networkStructure[0] || bestBrainOfGeneration.levels.back().outputs.size() != networkStructure.back()) {
            std::cerr << "Warning: Loaded brain structure mismatch! Starting fresh." << std::endl;
            bestBrainOfGeneration = NeuralNetwork(networkStructure);
        }
    } else {
        std::cout << "No saved brain found (" << BRAIN_FILENAME << "). Starting fresh generation 1." << std::endl;
    }
    applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS); // Aplica cérebro inicial (carregado ou novo)
    if (!cars.empty()) { bestCar = cars[0].get(); }
    // --- End Brain Management ---

    sf::Clock clock;

    // --- Main Loop ---
    while (window.isOpen()) {
        // *** Reinicia o clock a cada iteração, mas deltaTime só é usado se não estiver pausado ***
        sf::Time deltaTime = clock.restart();

        // --- Event Handling ---
        while (const auto eventOpt = window.pollEvent()) {
            const sf::Event& event = *eventOpt;
            if (event.is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                 bool isCtrlOrCmd = keyPressed->control || keyPressed->system;

                 // Comandos com Ctrl/Cmd
                 if (keyPressed->code == sf::Keyboard::Key::S && isCtrlOrCmd) {
                      if (bestBrainOfGeneration.saveToFile(BRAIN_FILENAME)) {
                          std::cout << "Best Generation Brain saved to " << BRAIN_FILENAME << std::endl;
                      } else { std::cerr << "Error saving best generation brain." << std::endl; }
                 }
                 // *** MODIFICADO: Comando D com Ctrl/Cmd permanece como estava ***
                 else if (keyPressed->code == sf::Keyboard::Key::D && isCtrlOrCmd) {
                      if (std::remove(BRAIN_FILENAME.c_str()) == 0) {
                          std::cout << "Saved brain file (" << BRAIN_FILENAME << ") discarded (Ctrl+D)." << std::endl;
                      } else { std::cout << "No brain file to discard or error removing (Ctrl+D)." << std::endl; }
                 }
                 // Comandos sem Ctrl/Cmd
                 // *** NOVO: Pausa/Despausa com P ***
                 else if (keyPressed->code == sf::Keyboard::Key::P) {
                     isPaused = !isPaused;
                     std::cout << (isPaused ? "Simulation PAUSED" : "Simulation RESUMED") << std::endl;
                 }
                 // *** NOVO: Deleta arquivo com D (sem Ctrl/Cmd) ***
                 else if (keyPressed->code == sf::Keyboard::Key::D && !isCtrlOrCmd) {
                     if (std::remove(BRAIN_FILENAME.c_str()) == 0) {
                         std::cout << "Saved brain file (" << BRAIN_FILENAME << ") discarded (D)." << std::endl;
                     } else { std::cout << "No brain file to discard or error removing (D)." << std::endl; }
                 }
                 // *** NOVO: Reinicia geração com R ***
                 else if (keyPressed->code == sf::Keyboard::Key::R) {
                     std::cout << "--- RESTARTING Generation " << generationCount << " ---" << std::endl;
                     // Recria os carros
                     cars = generateCars(NUM_AI_CARS, road);
                     // Aplica o MELHOR CÉREBRO DA GERAÇÃO ANTERIOR (ou o inicial) com mutações
                     applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS);
                     // Reseta o melhor carro atual
                     bestCar = cars.empty() ? nullptr : cars[0].get();
                     // Reseta o tráfego
                     resetTraffic(traffic, road);
                     // Atualiza ponteiros crus do tráfego
                     trafficRawPtrs.clear();
                     for (const auto& carPtr : traffic) { trafficRawPtrs.push_back(carPtr.get()); }
                     // Reinicia o relógio para evitar salto no deltaTime
                     clock.restart();
                     isPaused = false; // Garante que a simulação não esteja pausada após reiniciar
                     std::cout << "--- Generation " << generationCount << " Restarted ---" << std::endl;
                 }
                 // Comando Esc (sem alterações)
                 else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                     window.close();
                 }
            }
        }
        // --- Fim Event Handling ---

        // --- Bloco de Lógica Principal (Executa apenas se não estiver pausado) ---
        int nonDamagedCount = 0; // Declarar fora do if para usar no painel

        if (!isPaused) { // *** ENVOLVE LÓGICA DE UPDATE E FIM DE GERAÇÃO ***

            // --- Logic Update ---
            for (auto& carPtr : traffic) {
                 if(carPtr) carPtr->update(road.borders, trafficRawPtrs, deltaTime);
            }
            for (auto& carPtr : cars) {
                 if(carPtr) carPtr->update(road.borders, trafficRawPtrs, deltaTime);
            }
            // --- Fim Logic Update ---

            // --- Seleção do Best Car ATUAL (Executada mesmo se pausado para visualização) ---
            // (Movido para fora do if(!isPaused) abaixo)

            // --- VERIFICA FIM DA GERAÇÃO E REINICIA ---
            // Conta carros não danificados *dentro* do if(!isPaused) para lógica de fim de geração
            float minY_gen_end = std::numeric_limits<float>::max();
            Car* currentFrameBestCar_gen_end = nullptr; // Variáveis locais para essa checagem
            nonDamagedCount = 0; // Reinicia a contagem aqui

            for (const auto& carPtr : cars) {
                if (carPtr && !carPtr->isDamaged()) {
                    nonDamagedCount++;
                    // A lógica de encontrar o melhor carro visualmente é feita fora do if(!isPaused)
                }
            }

            // Lógica de fim de geração:
            if (nonDamagedCount == 0 && !cars.empty()) {
                std::cout << "\n--- GENERATION " << generationCount << " ENDED ---" << std::endl;
                minY_gen_end = std::numeric_limits<float>::max();
                Car* mostAdvancedCar = nullptr;
                for(const auto& carPtr : cars) {
                    if (carPtr && carPtr->position.y < minY_gen_end) {
                        minY_gen_end = carPtr->position.y;
                        mostAdvancedCar = carPtr.get();
                    }
                }

                if(mostAdvancedCar && mostAdvancedCar->brain){
                     bestBrainOfGeneration = *(mostAdvancedCar->brain);
                     std::cout << "Saving brain of most advanced car (Y=" << mostAdvancedCar->position.y << ") for next generation." << std::endl;
                     bestBrainOfGeneration.saveToFile(BRAIN_FILENAME);
                } else {
                     std::cout << "Could not find a valid brain to save. Keeping previous best." << std::endl;
                }

                std::cout << "--- STARTING GENERATION " << generationCount + 1 << " ---" << std::endl;
                generationCount++;
                cars = generateCars(NUM_AI_CARS, road);
                applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS);
                bestCar = cars.empty() ? nullptr : cars[0].get();
                resetTraffic(traffic, road);
                trafficRawPtrs.clear();
                for (const auto& carPtr : traffic) { trafficRawPtrs.push_back(carPtr.get()); }
                std::cout << "--- GENERATION " << generationCount << " READY --- \n" << std::endl;
                clock.restart(); // Reinicia o clock para nova geração
            }
            // --- FIM VERIFICAÇÃO E REINÍCIO ---

        } // *** Fim do if (!isPaused) ***


        // --- Seleção do Best Car ATUAL (para visualização, feito mesmo se pausado) ---
        float minY_vis = std::numeric_limits<float>::max();
        Car* currentFrameBestCar_vis = nullptr;
        int currentNonDamagedCount = 0; // Recalcular aqui para o painel
        for (const auto& carPtr : cars) {
             if (carPtr && !carPtr->isDamaged()) {
                 currentNonDamagedCount++; // Conta para o painel
                 if (carPtr->position.y < minY_vis) {
                     minY_vis = carPtr->position.y;
                     currentFrameBestCar_vis = carPtr.get();
                 }
             }
        }
        if (currentFrameBestCar_vis) {
             bestCar = currentFrameBestCar_vis; // Atualiza o ponteiro principal
        } else {
            // Lógica de fallback (se todos danificados)
            if (!bestCar || bestCar->isDamaged()) {
                minY_vis = std::numeric_limits<float>::max();
                Car* fallbackCar = nullptr;
                for(const auto& carPtr : cars) {
                    if (carPtr && carPtr->position.y < minY_vis) {
                         minY_vis = carPtr->position.y;
                         fallbackCar = carPtr.get();
                    }
                }
                bestCar = fallbackCar;
            }
             // Se havia um bestCar não danificado antes e agora todos estão, mantém o último válido
             // (bestCar já aponta para ele)
        }
        // --- Fim Seleção Best Car ---


        // --- ATUALIZAÇÃO DO TEXTO DO PAINEL (Feita sempre) ---
         std::ostringstream statusStream;
         statusStream << std::fixed << std::setprecision(1);

         statusStream << "Generation: " << generationCount << "\n";
         if (isPaused) { // *** Adiciona indicador de PAUSA ***
             statusStream << "\n--- PAUSED ---\n\n";
         } else {
             statusStream << "\n";
         }

         if (bestCar) {
             float currentDist = bestCar->calculateMinDistanceAhead(trafficRawPtrs);
             float prevDist = bestCar->getPreviousDistanceAhead();
             float distDelta = (prevDist >= 0.0f && currentDist < std::numeric_limits<float>::max())
                           ? currentDist - prevDist : 0.0f;
             int aggroFrames = bestCar->getAggressiveDeltaFrames();

             statusStream << "Best Score: " << bestCar->getScore() << "\n";
             statusStream << "Best Y Pos: " << bestCar->position.y << "\n";
             statusStream << "Dist Ahead: ";
             if (currentDist < std::numeric_limits<float>::max()) statusStream << currentDist; else statusStream << "INF";
             statusStream << "\n";
             statusStream << "Dist Delta: " << distDelta << "\n";
             statusStream << "Aggro Frames: " << aggroFrames << "\n\n";
         } else {
             statusStream << "Best Score: N/A\n";
             statusStream << "Best Y Pos: N/A\n";
             statusStream << "Dist Ahead: N/A\n";
             statusStream << "Dist Delta: N/A\n";
             statusStream << "Aggro Frames: N/A\n\n";
         }
         // Usa a contagem recalculada fora do if(!isPaused) para mostrar o estado atual
         statusStream << "Alive: " << currentNonDamagedCount << " / " << cars.size();

         statusText.setString(statusStream.str());
         // --- FIM ATUALIZAÇÃO TEXTO PAINEL ---


        // --- Drawing (Feito sempre) ---
        window.clear(sf::Color(100, 100, 100));

        // Draw Status Panel
        window.setView(window.getDefaultView());
        window.draw(statusPanelBackground);
        window.draw(statusText);

        // Draw Simulation View
        if (bestCar) {
             carView.setCenter({carCanvasWidth / 2.0f, bestCar->position.y - windowHeight * 0.3f});
        } else {
             carView.setCenter({carCanvasWidth / 2.0f, windowHeight / 2.0f});
        }
        window.setView(carView);
        road.draw(window);
        for (const auto& carPtr : traffic) { if(carPtr) carPtr->draw(window); }
        for (const auto& carPtr : cars) { if (carPtr && carPtr.get() != bestCar) { carPtr->draw(window, false); } }
        if (bestCar) { bestCar->draw(window, true); }

        // Draw Network View
        window.setView(networkView);
        NeuralNetwork* brainToDraw = nullptr;
        if (bestCar && !bestCar->isDamaged() && bestCar->brain) { brainToDraw = &(*(bestCar->brain)); }
        else { brainToDraw = &bestBrainOfGeneration; }
        sf::RectangleShape networkBackground(sf::Vector2f((float)networkCanvasWidth, (float)windowHeight));
        networkBackground.setFillColor(sf::Color(60, 60, 60));
        networkBackground.setPosition({0.f, 0.f});
        window.draw(networkBackground);
        if (brainToDraw) { Visualizer::drawNetwork(window, *brainToDraw, font, 0.f, 0.f, (float)networkCanvasWidth, (float)windowHeight); }

        // Display
        window.display();
    } // End main loop

    return 0;
}

// --- Function Definitions (generateCars, resetTraffic, applyBrainsToGeneration - sem alterações) ---
// ... (cole as definições das funções aqui, elas não precisam mudar) ...

std::vector<std::unique_ptr<Car>> generateCars(int N, const Road& road, float startY) {
    std::vector<std::unique_ptr<Car>> cars;
    cars.reserve(N);
    for (int i = 0; i < N; ++i) {
        cars.push_back(std::make_unique<Car>(
            road.getLaneCenter(1), startY, 30.0f, 50.0f,
            ControlType::AI, 3.0f, getRandomColor()
        ));
    }
    return cars;
}

void resetTraffic(std::vector<std::unique_ptr<Car>>& traffic, const Road& road) {
    std::vector<std::pair<int, float>> initialPositions = {
        {1, -100.0f}, {0, -300.0f}, {2, -300.0f},
        {0, -500.0f}, {1, -500.0f}, {1, -700.0f}, {2, -700.0f}
    };
    float dummySpeed = 2.0f;

    if (traffic.size() < initialPositions.size()) {
        traffic.resize(initialPositions.size());
    }
    for(size_t i = 0; i < initialPositions.size(); ++i) {
        if (!traffic[i]) {
             traffic[i] = std::make_unique<Car>(
                 road.getLaneCenter(initialPositions[i].first), initialPositions[i].second,
                 30.f, 50.f,
                 ControlType::DUMMY, dummySpeed, getRandomColor());
        } else {
             sf::Vector2f startPos = {road.getLaneCenter(initialPositions[i].first), initialPositions[i].second};
             traffic[i]->resetForTraffic(startPos, dummySpeed);
        }
    }
     for (size_t i = initialPositions.size(); i < traffic.size(); ++i) {
         if (traffic[i]) {
             traffic[i]->resetForTraffic(traffic[i]->position, 0.0f);
         }
     }
}

void applyBrainsToGeneration(std::vector<std::unique_ptr<Car>>& cars, const NeuralNetwork& bestBrain, int N) {
     if (cars.empty()) return;

     if (cars[0]->useBrain && cars[0]->brain) {
         *(cars[0]->brain) = bestBrain;
     } else if (cars[0]->useBrain && !cars[0]->brain) {
         std::cerr << "Error: Car 0 is AI but has no brain object during applyBrains!" << std::endl;
     }

     for (int i = 1; i < cars.size(); ++i) {
         if (cars[i]->useBrain && cars[i]->brain) {
             *(cars[i]->brain) = bestBrain;
             NeuralNetwork::mutate(*(cars[i]->brain), 0.15f); // Aplicando mutação
         } else if (cars[i]->useBrain && !cars[i]->brain) {
              std::cerr << "Error: Car " << i << " is AI but has no brain object during applyBrains!" << std::endl;
         }
     }
}