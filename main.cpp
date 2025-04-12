// main.cpp - ATUALIZADO COM SCORE BASEADO EM DISTÂNCIA LÍQUIDA e REINÍCIO DE GERAÇÃO

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <limits>
#include <cstdio>    // For std::remove
#include <optional>
#include <iomanip>   // For std::fixed, std::setprecision
#include <cmath>     // Para std::abs

// --- Project Headers ---
#include "Road.hpp"
#include "Car.hpp"
#include "Visualizer.hpp"
#include "Utils.hpp"
#include "Network.hpp"

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
    sf::Font font;
    if (!font.openFromFile("assets/LiberationSans-Regular.ttf")) { // Certifique-se de ter a fonte
        std::cerr << "Error loading font." << std::endl;
        return 1; // Encerra se a fonte não carregar
    }
    Road road(carCanvasWidth / 2.0f, carCanvasWidth * 0.9f, 3);
    // --- End Setup ---

    // --- Simulation Variables ---
    auto cars = generateCars(NUM_AI_CARS, road);
    Car* bestCar = nullptr; // Ponteiro para o melhor carro *visualizado*
    int generationCount = 1;

    // --- Traffic Setup ---
    std::vector<std::unique_ptr<Car>> traffic;
    resetTraffic(traffic, road);
    std::vector<Car*> trafficRawPtrs; // Ponteiros crus para passar para Car::update
    trafficRawPtrs.reserve(traffic.size());
    for (const auto& carPtr : traffic) {
        trafficRawPtrs.push_back(carPtr.get());
    }
    std::vector<Car*> emptyTrafficList; // Para atualizar os carros de tráfego sem checar colisão entre eles
    // --- End Traffic Setup ---

    // --- Brain Management ---
    std::vector<int> networkStructure = {5, 6, 4}; // Estrutura padrão
    // Ajusta a camada de entrada com base na contagem de raios do sensor (se houver)
    if (!cars.empty() && cars[0]->sensor) {
        networkStructure[0] = static_cast<int>(cars[0]->sensor->rayCount);
    }
    NeuralNetwork bestBrainOfGeneration(networkStructure); // Cérebro para guardar o melhor da geração
    // Tenta carregar o melhor cérebro da sessão anterior
    if (bestBrainOfGeneration.loadFromFile(BRAIN_FILENAME)) {
        std::cout << "Loaded best brain from previous session from " << BRAIN_FILENAME << std::endl;
        applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS);
    } else {
        std::cout << "No saved brain found (" << BRAIN_FILENAME << "). Starting fresh generation 1." << std::endl;
    }
    // Define o carro inicial para visualização
    if (!cars.empty()) {
        bestCar = cars[0].get();
    }
    // --- End Brain Management ---

    sf::Clock clock; // Para calcular deltaTime

    // --- Main Loop ---
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart(); // Tempo desde o último frame

        // --- Event Handling ---
        while (const auto eventOpt = window.pollEvent()) {
            const sf::Event& event = *eventOpt; // Dereference the optional
            if (event.is<sf::Event::Closed>()) {
                window.close();
            }
            // Tratamento de teclas pressionadas
            if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
                 bool isCtrlOrCmd = keyPressed->control || keyPressed->system; // Verifica Ctrl (Linux/Win) ou Cmd (Mac)
                 // Salvar (Ctrl+S)
                 if (keyPressed->code == sf::Keyboard::Key::S && isCtrlOrCmd) {
                     if(bestBrainOfGeneration.saveToFile(BRAIN_FILENAME)) {
                         std::cout << "Best Generation Brain saved to " << BRAIN_FILENAME << std::endl;
                     } else {
                         std::cerr << "Error saving best brain." << std::endl;
                     }
                 }
                 // Descartar (Ctrl+D)
                 else if (keyPressed->code == sf::Keyboard::Key::D && isCtrlOrCmd) {
                     if (std::remove(BRAIN_FILENAME.c_str()) == 0) {
                         std::cout << "Saved brain file (" << BRAIN_FILENAME << ") discarded." << std::endl;
                     } else {
                         std::cout << "No saved brain file (" << BRAIN_FILENAME << ") to discard." << std::endl;
                     }
                 }
                 // Sair (ESC)
                 else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                     window.close();
                 }
            }
        } // Fim event loop

        // --- Logic Update ---
        // Atualiza carros do tráfego (sem checar colisão entre si)
        for (auto& carPtr : traffic) {
            if(carPtr) carPtr->update(road.borders, emptyTrafficList);
        }
        // Atualiza carros da IA (checando colisão com tráfego e bordas)
        for (auto& carPtr : cars) {
             if(carPtr) carPtr->update(road.borders, trafficRawPtrs);
        }

        // --- Seleção do Best Car ATUAL (para visualização, baseado no MAIOR SCORE entre os VIVOS) ---
        float maxScoreAlive = -std::numeric_limits<float>::max();
        Car* currentFrameBestCar = nullptr; // Melhor carro vivo neste frame
        int nonDamagedCount = 0;
        for (const auto& carPtr : cars) {
             if (carPtr && !carPtr->damaged) {
                 nonDamagedCount++;
                 float currentScore = carPtr->getNetDistanceTraveled();
                 if (currentScore > maxScoreAlive) {
                     maxScoreAlive = currentScore;
                     currentFrameBestCar = carPtr.get();
                 }
             }
        }

        // Atualiza bestCar (para visualização)
        if (currentFrameBestCar) {
             bestCar = currentFrameBestCar; // A câmera segue o melhor carro vivo
        } else { // Se todos danificados, procura um carro (qualquer um) para a câmera seguir (o mais avançado)
             if (cars.empty()){
                 bestCar = nullptr;
             } else if (!bestCar || bestCar->damaged) { // Se o bestCar anterior foi danificado ou não existe
                 float minY_fallback = std::numeric_limits<float>::max();
                 Car* fallbackCar = nullptr;
                 for(const auto& carPtr : cars) { // Procura o carro mais à frente (menor Y)
                      if (carPtr && carPtr->position.y < minY_fallback) {
                          minY_fallback = carPtr->position.y;
                          fallbackCar = carPtr.get();
                      }
                 }
                 bestCar = fallbackCar; // A câmera segue o carro mais avançado
             }
             // Se bestCar já existe e não está danificado, continua seguindo ele mesmo se não for o líder
        }

        // --- LOGGING (Opcional, mas útil para debug) ---
        if (bestCar) { // Loga informações do carro que a câmera está seguindo
             std::cout << std::fixed << std::setprecision(2);
             std::cout << "Gen: " << generationCount
                       << " | Following Car: " << bestCar << " (Y: " << bestCar->position.y << ", Score: " << bestCar->getNetDistanceTraveled() << ", Damaged: " << (bestCar->damaged?"Yes":"No") << ")"
                       << " | Leader Score: " << maxScoreAlive // Score do melhor vivo
                       << " | Alive: " << nonDamagedCount << "/" << cars.size()
                       << "\r" << std::flush; // \r para sobrescrever a linha anterior no console
        }
        // --- FIM LOGGING ---

        // --- VERIFICA FIM DA GERAÇÃO E REINICIA ---
        if (nonDamagedCount == 0 && !cars.empty()) {
            std::cout << std::endl; // Nova linha para não sobrescrever o log final

            // <<< INÍCIO DA NOVA LÓGICA DE SELEÇÃO DO MELHOR CÉREBRO >>>
            float absoluteMaxScore = -std::numeric_limits<float>::max();
            Car* absoluteBestCarOverall = nullptr;
            for (const auto& carPtr : cars) {
                if (carPtr) { // Garante que o ponteiro é válido
                    float currentScore = carPtr->getNetDistanceTraveled();
                    if (currentScore > absoluteMaxScore) {
                        absoluteMaxScore = currentScore;
                        absoluteBestCarOverall = carPtr.get();
                    }
                }
            }

            // Atualiza o cérebro que será usado como base para a próxima geração
            if (absoluteBestCarOverall && absoluteBestCarOverall->getBrain()) {
                bestBrainOfGeneration = *(absoluteBestCarOverall->getBrain());
                std::cout << "--- GENERATION " << generationCount << " ENDED ---" << std::endl;
                std::cout << "Selected best brain from Car " << absoluteBestCarOverall
                          << " (Score: " << absoluteMaxScore
                          << ", Damaged: " << (absoluteBestCarOverall->damaged ? "Yes" : "No") << ")" << std::endl;
            } else {
                // Fallback: Mantém o cérebro do último líder não danificado que estava em bestBrainOfGeneration
                std::cout << "--- GENERATION " << generationCount << " ENDED ---" << std::endl;
                std::cout << "Warning: Could not find an absolute best car with a brain. Using last known best brain." << std::endl;
                // Não precisa fazer nada, bestBrainOfGeneration já tem o cérebro do último líder vivo
            }
            // <<< FIM DA NOVA LÓGICA DE SELEÇÃO DO MELHOR CÉREBRO >>>


            // --- Lógica Original de Reinício Continua ---
            std::cout << "--- STARTING GENERATION " << generationCount + 1 << " ---" << std::endl;
            generationCount++;

            // Salva o melhor cérebro SELECIONADO
            if (bestBrainOfGeneration.saveToFile(BRAIN_FILENAME)) {
                 std::cout << "Best Overall Brain saved to " << BRAIN_FILENAME << std::endl;
             } else {
                 std::cerr << "Error saving best overall brain." << std::endl;
             }

            // Gera a nova população de carros
            cars = generateCars(NUM_AI_CARS, road);
            // Aplica o cérebro selecionado (com mutações) à nova geração
            applyBrainsToGeneration(cars, bestBrainOfGeneration, NUM_AI_CARS);

            // Reseta o 'bestCar' para visualização
            bestCar = cars.empty() ? nullptr : cars[0].get();

            // Reseta o tráfego
            resetTraffic(traffic, road);
            trafficRawPtrs.clear(); // Limpa a lista de ponteiros crus
            for (const auto& carPtr : traffic) { // Repopula com os novos ponteiros de tráfego
                trafficRawPtrs.push_back(carPtr.get());
            }

            std::cout << "--- GENERATION " << generationCount << " READY --- \n" << std::endl;
            // Pausa opcional para ler o log
            // sf::sleep(sf::seconds(1));
        }
        // --- FIM VERIFICAÇÃO E REINÍCIO ---


        // --- Drawing ---
        window.clear(sf::Color(100, 100, 100)); // Cor de fundo cinza

        // --- Desenha a Simulação ---
        window.setView(carView); // Define a view da simulação
        // Centraliza a câmera no 'bestCar' (o carro sendo seguido)
        if (bestCar) {
            carView.setCenter({carCanvasWidth / 2.0f, bestCar->position.y - windowHeight * 0.3f});
        } else { // Fallback se não houver carro para seguir
            carView.setCenter({carCanvasWidth / 2.0f, windowHeight / 2.0f});
        }
        window.setView(carView); // Aplica a view atualizada

        road.draw(window); // Desenha a estrada

        // Desenha o tráfego
        for (const auto& carPtr : traffic) {
            if(carPtr) carPtr->draw(window);
        }

        // Desenha os carros da IA (exceto o 'bestCar' que será desenhado por último com sensor)
        sf::ContextSettings settings; // Necessário para anti-aliasing das formas
        for (const auto& carPtr : cars) {
            if (carPtr && carPtr.get() != bestCar) { // Desenha sem sensor se não for o 'bestCar'
                carPtr->draw(window, false);
            }
        }
        // Desenha o 'bestCar' por último e com o sensor visível
        if (bestCar) {
            bestCar->draw(window, true);
        }

        // --- Desenha a Rede Neural ---
        window.setView(networkView); // Define a view da rede neural

        // Decide qual cérebro desenhar: o do carro seguido (se vivo) ou o melhor da geração
        NeuralNetwork* brainToDraw = nullptr;
        if (bestCar && !bestCar->damaged && bestCar->getBrain()) {
            brainToDraw = bestCar->getBrain();
        } else {
            // Se o carro seguido está danificado ou não tem cérebro, mostra o melhor da geração
            brainToDraw = &bestBrainOfGeneration;
        }

        // Desenha a rede neural se um cérebro válido foi determinado
        if (brainToDraw) {
            Visualizer::drawNetwork(window, *brainToDraw, font, 0.f, 0.f, (float)networkCanvasWidth, (float)windowHeight);
        }

        // --- Finaliza o Desenho ---
        window.setView(window.getDefaultView()); // Restaura a view padrão antes de exibir
        window.display(); // Mostra tudo na tela
    } // End main loop

    // --- Cleanup ---
    // unique_ptr faz a limpeza automaticamente quando sai do escopo.
    // Não é necessário 'delete' manual.

    return 0;
}

// --- Function Definitions ---

// Gera N carros de IA
std::vector<std::unique_ptr<Car>> generateCars(int N, const Road& road, float startY) {
    std::vector<std::unique_ptr<Car>> cars;
    cars.reserve(N);
    for (int i = 0; i < N; ++i) {
        // Cria o carro com um unique_ptr
        cars.push_back(std::make_unique<Car>(
            road.getLaneCenter(1), // Começa na faixa do meio
            startY,                // Posição Y inicial
            30.0f,                 // Largura
            50.0f,                 // Altura
            ControlType::AI,       // Tipo de controle
            3.0f,                  // Velocidade máxima
            getRandomColor()       // Cor aleatória
        ));
    }
    return cars; // Retorna o vetor de unique_ptr (movido)
}

// Reseta ou cria o tráfego
void resetTraffic(std::vector<std::unique_ptr<Car>>& traffic, const Road& road) {
     // Define as posições iniciais do tráfego
     float startPositions[] = {-100.f, -300.f, -300.f, -500.f, -500.f, -700.f, -700.f};
     int lanes[] = {1, 0, 2, 0, 1, 1, 2};
     int numTrafficCars = sizeof(startPositions) / sizeof(startPositions[0]);

     if (traffic.size() == numTrafficCars) { // Se o tráfego já existe, reseta posições e estado
         for(int i = 0; i < numTrafficCars; ++i) {
             if(traffic[i]) {
                 traffic[i]->position = {road.getLaneCenter(lanes[i]), startPositions[i]};
                 traffic[i]->damaged = false;
                 traffic[i]->speed = 2.0f; // Reseta velocidade padrão do tráfego
                 // Resetar outros estados se necessário (ângulo, etc.)
                 traffic[i]->angle = 0.0f;
             }
         }
     } else { // Se não existe ou o tamanho está errado, cria do zero
         traffic.clear(); // Limpa o vetor existente
         traffic.reserve(numTrafficCars);
         for(int i = 0; i < numTrafficCars; ++i) {
             traffic.push_back(std::make_unique<Car>(
                 road.getLaneCenter(lanes[i]),
                 startPositions[i],
                 30.f, 50.f,           // Largura/Altura
                 ControlType::DUMMY,   // Tipo DUMMY
                 2.0f,                 // Velocidade do tráfego
                 getRandomColor()      // Cor aleatória
             ));
         }
     }
}


// Aplica o melhor cérebro (com mutações) para a nova geração
void applyBrainsToGeneration(std::vector<std::unique_ptr<Car>>& cars, const NeuralNetwork& bestBrain, int N) {
     if (cars.empty()) return;

     // Aplica o melhor cérebro sem mutação ao primeiro carro
     NeuralNetwork* firstCarBrain = cars[0]->getBrain();
     if (firstCarBrain) {
         *firstCarBrain = bestBrain; // Copia o conteúdo do bestBrain
     }

     // Aplica o melhor cérebro COM mutação aos demais carros
     for (int i = 1; i < cars.size(); ++i) {
         NeuralNetwork* carBrain = cars[i]->getBrain();
         if (carBrain) {
             *carBrain = bestBrain; // Copia o conteúdo do bestBrain
             // Aplica uma mutação (ex: 15% de chance de alterar pesos/biases)
             NeuralNetwork::mutate(*carBrain, 0.15f);
         }
     }
}