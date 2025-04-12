// main.cpp - COMPLETO COM CORREÇÕES, PAINEL DE STATUS E LOG NO CONSOLE REATIVADO

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
#include <cmath>     // Para std::abs

// --- Project Headers ---
#include "Road.hpp"
#include "Car.hpp"
#include "Visualizer.hpp" // Presume que Visualizer.hpp existe
#include "Utils.hpp"      // Presume que Utils.hpp existe
#include "Network.hpp"    // Presume que Network.hpp existe
#include "Sensor.hpp"     // Presume que Sensor.hpp existe

// --- Constants ---
const std::string BRAIN_FILENAME = "bestBrain.dat";
const int NUM_AI_CARS = 100;

// --- Function Declarations ---
std::vector<std::unique_ptr<Car>> generateCars(int N, const Road& road, float startY = 100.0f);
void resetTraffic(std::vector<std::unique_ptr<Car>>& traffic, const Road& road);
void applyBrainsToGeneration(std::vector<std::unique_ptr<Car>>& cars, const NeuralNetwork& bestBrain, int N);


// --- Main Application ---
int main() {
    // --- Setup (Window, Views, Font, Road) ---
    const unsigned int carCanvasWidth = 200;
    const unsigned int networkCanvasWidth = 400;
    const unsigned int windowWidth = carCanvasWidth + networkCanvasWidth;
    const unsigned int windowHeight = 700;
    sf::RenderWindow window(sf::VideoMode({windowWidth, windowHeight}), "Self-Driving Car - C++/SFML");
    window.setFramerateLimit(60);
    sf::View carView(sf::FloatRect({0.f, 0.f}, {(float)carCanvasWidth, (float)windowHeight}));
    carView.setViewport(sf::FloatRect({0.f, 0.f}, {(float)carCanvasWidth / windowWidth, 1.f}));
    sf::View networkView(sf::FloatRect({0.f, 0.f}, {(float)networkCanvasWidth, (float)windowHeight}));
    networkView.setViewport(sf::FloatRect({(float)carCanvasWidth / windowWidth, 0.f}, {(float)networkCanvasWidth / windowWidth, 1.f}));

    // Carrega a fonte PRIMEIRO
    sf::Font font;
    if (!font.openFromFile("assets/LiberationSans-Regular.ttf")) {
        std::cerr << "Error loading font: assets/LiberationSans-Regular.ttf" << std::endl;
        return 1; // Encerra se a fonte não carregar
    }
    Road road(carCanvasWidth / 2.0f, carCanvasWidth * 0.9f, 3);

    // --- Texto para o painel de status (Inicializado APÓS carregar a fonte) ---
    sf::Text statusText(font, "", 14); // Chama o construtor com a fonte
    statusText.setFillColor(sf::Color::White);
    statusText.setPosition({10.f, 10.f}); // Posição no canto superior esquerdo da janela
    // --- Fim Texto Status ---

    // --- End Setup ---

    // --- Simulation Variables ---
    auto cars = generateCars(NUM_AI_CARS, road); // Gera a primeira geração
    Car* bestCar = nullptr; // Ponteiro para o melhor carro *atual*
    int generationCount = 1; // Contador de gerações

    // --- Traffic Setup ---
    std::vector<std::unique_ptr<Car>> traffic;
    resetTraffic(traffic, road); // Posições iniciais do tráfego

    std::vector<Car*> trafficRawPtrs; // Vetor de ponteiros crus para checagem de colisão
    trafficRawPtrs.reserve(traffic.size());
    for (const auto& carPtr : traffic) { trafficRawPtrs.push_back(carPtr.get()); }
    std::vector<Car*> emptyTrafficList; // Usado para update do tráfego
    // --- End Traffic Setup ---


    // --- Brain Management ---
    std::vector<int> networkStructure = {5, 6, 5}; // Estrutura default (SensorInputs, Hidden, Outputs) - 5 saídas agora
    if (!cars.empty()) {
        int rayCount = cars[0]->getSensorRayCount(); // Usa getter
        if (rayCount > 0) {
             networkStructure[0] = rayCount; // Atualiza input layer size
        } else if (cars[0]->useBrain) { // Acessa useBrain (público)
             std::cerr << "Warning: First AI car has 0 sensor rays detected!" << std::endl;
        }
    }

    // Cérebro do melhor carro da *geração anterior* (para clonagem)
    NeuralNetwork bestBrainOfGeneration(networkStructure); // Começa randomizado

    // Tenta carregar o melhor cérebro salvo
    if (bestBrainOfGeneration.loadFromFile(BRAIN_FILENAME)) {
        std::cout << "Loaded best brain from previous session: " << BRAIN_FILENAME << std::endl;
        // Valida estrutura básica
        if (!bestBrainOfGeneration.levels.empty() &&
            bestBrainOfGeneration.levels.front().inputs.size() == networkStructure[0] &&
            bestBrainOfGeneration.levels.back().outputs.size() == networkStructure.back())
        {
            applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS);
        } else {
            std::cerr << "Warning: Loaded brain structure mismatch! Starting fresh." << std::endl;
            bestBrainOfGeneration = NeuralNetwork(networkStructure); // Recria se incompatível
        }
    } else {
        std::cout << "No saved brain found (" << BRAIN_FILENAME << "). Starting fresh generation 1." << std::endl;
    }
    // --- End Brain Management ---

    // Atualiza bestCar inicial após aplicar cérebros
    if (!cars.empty()) { bestCar = cars[0].get(); }


    // --- Game Loop Clock ---
    sf::Clock clock;

    // --- Main Loop ---
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();

        // --- Event Handling ---
        while (const auto eventOpt = window.pollEvent()) {
            const sf::Event& event = *eventOpt;
            if (event.is<sf::Event::Closed>()) { window.close(); }
            if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                 bool isCtrlOrCmd = keyPressed->control || keyPressed->system; // Verifica Ctrl (Linux/Win) ou Cmd (Mac)
                 if (keyPressed->code == sf::Keyboard::Key::S && isCtrlOrCmd) { // Salvar Melhor Cérebro
                      if (bestBrainOfGeneration.saveToFile(BRAIN_FILENAME)) { std::cout << "Best Generation Brain saved." << std::endl; }
                      else { std::cerr << "Error saving best brain." << std::endl; }
                  } else if (keyPressed->code == sf::Keyboard::Key::D && isCtrlOrCmd) { // Descartar Cérebro
                      if (std::remove(BRAIN_FILENAME.c_str()) == 0) { std::cout << "Brain discarded." << std::endl; }
                      else { std::cout << "No brain file to discard or error removing." << std::endl; }
                  } else if (keyPressed->code == sf::Keyboard::Key::Escape) { window.close(); } // Fechar
            }
        }
        // --- Fim Event Handling ---

        // --- Logic Update ---
        for (auto& carPtr : traffic) { if(carPtr) carPtr->update(road.borders, emptyTrafficList); }
        for (auto& carPtr : cars) { if(carPtr) carPtr->update(road.borders, trafficRawPtrs); }
        // --- Fim Logic Update ---

        // --- Seleção do Best Car ATUAL ---
        float minY = std::numeric_limits<float>::max();
        Car* currentFrameBestCar = nullptr;
        int nonDamagedCount = 0;
        for (const auto& carPtr : cars) {
             if (carPtr && !carPtr->isDamaged()) {
                 nonDamagedCount++;
                 if (carPtr->position.y < minY) {
                     minY = carPtr->position.y;
                     currentFrameBestCar = carPtr.get();
                 }
             }
        }

        if (currentFrameBestCar) {
             bestCar = currentFrameBestCar;
        } else {
             if (!bestCar || bestCar->isDamaged()) {
                minY = std::numeric_limits<float>::max();
                Car* fallbackCar = nullptr;
                for(const auto& carPtr : cars) { if (carPtr && carPtr->position.y < minY) { minY = carPtr->position.y; fallbackCar = carPtr.get(); }}
                bestCar = fallbackCar;
            }
        }
        // --- Fim Seleção Best Car ---


        // --- LOGGING e Atualização do Painel de Status ---
         std::ostringstream statusStream; // Stream para o painel sf::Text
         statusStream << "Gen: " << generationCount;

         // Variáveis para log (calculadas uma vez)
         float currentDist = std::numeric_limits<float>::max();
         float prevDist = -1.0f;
         float distDelta = 0.0f;
         int aggroFrames = 0;

         if (bestCar) {
             currentDist = bestCar->calculateMinDistanceAhead(trafficRawPtrs);
             prevDist = bestCar->getPreviousDistanceAhead();
             distDelta = (prevDist >= 0 && currentDist < std::numeric_limits<float>::max()) ? currentDist - prevDist : 0.0f;
             aggroFrames = bestCar->getAggressiveDeltaFrames();

             statusStream << std::fixed << std::setprecision(2);
             statusStream << " | BestCar Y: " << bestCar->position.y;
             statusStream << " | Dist Ahead: ";
             if (currentDist < std::numeric_limits<float>::max()) statusStream << currentDist; else statusStream << "INF";
             statusStream << " | Delta: " << distDelta;
             statusStream << " | AggroFrames: " << aggroFrames;
         } else {
             statusStream << " | BestCar Y: N/A";
             statusStream << " | Dist Ahead: N/A";
             statusStream << " | Delta: N/A";
             statusStream << " | AggroFrames: N/A";
         }
         statusStream << " | Alive: " << nonDamagedCount << "/" << cars.size();

         // Define o texto do painel sf::Text
         statusText.setString(statusStream.str());

         // --- LOG NO CONSOLE (Reativado) ---
         if (bestCar) {
             std::cout << std::fixed << std::setprecision(2); // Garante formatação para cout também
             std::cout << "Gen: " << generationCount
                       << " | BestCar Y: " << bestCar->position.y
                       << " | Dist Ahead: " << (currentDist < std::numeric_limits<float>::max() ? std::to_string(currentDist) : "INF")
                       << " | Delta: " << distDelta
                       << " | AggroFrames: " << aggroFrames
                       << " | Alive: " << nonDamagedCount << "/" << cars.size()
                       << std::endl;
         } else {
             // Log mínimo se não houver bestCar
             std::cout << "Gen: " << generationCount << " | No best car found. | Alive: " << nonDamagedCount << "/" << cars.size() << std::endl;
         }
         // --- FIM LOG NO CONSOLE ---

        // --- FIM LOGGING / Painel Status ---


        // --- VERIFICA FIM DA GERAÇÃO E REINICIA ---
        if (nonDamagedCount == 0 && !cars.empty()) {
            std::cout << "\n--- GENERATION " << generationCount << " ENDED ---" << std::endl;

            minY = std::numeric_limits<float>::max();
            Car* mostAdvancedCar = nullptr;
            for(const auto& carPtr : cars) { if (carPtr && carPtr->position.y < minY) { minY = carPtr->position.y; mostAdvancedCar = carPtr.get(); }}

            if(mostAdvancedCar && mostAdvancedCar->brain){
                 bestBrainOfGeneration = *(mostAdvancedCar->brain);
                 std::cout << "Saving brain of most advanced car (Y=" << mostAdvancedCar->position.y << ") for next generation." << std::endl;
                 bestBrainOfGeneration.saveToFile(BRAIN_FILENAME);
            } else {
                 std::cout << "Could not find a valid brain to save from the finished generation. Keeping previous best generation brain." << std::endl;
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
            clock.restart();
        }
        // --- FIM VERIFICAÇÃO E REINÍCIO ---


        // --- Drawing ---
        window.clear(sf::Color(100, 100, 100));

        // --- Draw Simulation View ---
        window.setView(carView);
        if (bestCar) {
             carView.setCenter({carCanvasWidth / 2.0f, bestCar->position.y - windowHeight * 0.3f});
        } else {
             carView.setCenter({carCanvasWidth / 2.0f, windowHeight / 2.0f});
        }
        window.setView(carView); // Reaplica a view após centralizar

        road.draw(window);
        for (const auto& carPtr : traffic) { if(carPtr) carPtr->draw(window); }
        for (const auto& carPtr : cars) { if (carPtr && carPtr.get() != bestCar) { carPtr->draw(window, false); } }
        if (bestCar) { bestCar->draw(window, true); }


        // --- Draw Network View ---
        window.setView(networkView); // Aplica a view da rede
        NeuralNetwork* brainToDraw = nullptr;
        if (bestCar && !bestCar->isDamaged() && bestCar->brain) {
             brainToDraw = &(*(bestCar->brain));
        } else {
              brainToDraw = &bestBrainOfGeneration;
        }
        if (brainToDraw) {
            Visualizer::drawNetwork(window, *brainToDraw, font, 0.f, 0.f, (float)networkCanvasWidth, (float)windowHeight);
        }

        // --- Draw Status Panel Text ---
        window.setView(window.getDefaultView()); // Usa a visão padrão para desenhar UI
        window.draw(statusText); // Desenha o texto de status

        // --- Display ---
        window.display();
    } // End main loop

    return 0;
}

// --- Function Definitions ---

// Gera carros AI
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

// Reseta as posições e estado dos carros de tráfego
void resetTraffic(std::vector<std::unique_ptr<Car>>& traffic, const Road& road) {
    std::vector<std::pair<int, float>> initialPositions = {
        {1, -100.0f}, {0, -300.0f}, {2, -300.0f},
        {0, -500.0f}, {1, -500.0f}, {1, -700.0f}, {2, -700.0f}
    };
    float dummySpeed = 2.0f;

    if (traffic.size() < initialPositions.size()) {
        traffic.resize(initialPositions.size());
    }
    for(size_t i=0; i<initialPositions.size(); ++i) {
        if (!traffic[i]) {
             traffic[i] = std::make_unique<Car>(
                 road.getLaneCenter(initialPositions[i].first), initialPositions[i].second,
                 30.f, 50.f, ControlType::DUMMY, dummySpeed, getRandomColor());
        }
    }

    for (size_t i = 0; i < traffic.size(); ++i) {
        if (traffic[i] && i < initialPositions.size()) {
             sf::Vector2f startPos = {road.getLaneCenter(initialPositions[i].first), initialPositions[i].second};
             traffic[i]->resetForTraffic(startPos, dummySpeed);
         } else if (traffic[i]) {
             traffic[i]->resetForTraffic(traffic[i]->position, 0.0f);
         }
    }
}


// Aplica o melhor cérebro e mutações à nova geração
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
             NeuralNetwork::mutate(*(cars[i]->brain), 0.15f);
         } else if (cars[i]->useBrain && !cars[i]->brain) {
              std::cerr << "Error: Car " << i << " is AI but has no brain object during applyBrains!" << std::endl;
         }
     }
}