#include "Game.hpp"
#include "Car.hpp"
#include "Obstacle.hpp"
#include "Road.hpp"
#include "Network.hpp"
#include "Visualizer.hpp"
#include "Utils.hpp" // Para getRandomColor, lerp, etc.

#include <SFML/Window/Event.hpp>
#include <iostream>
#include <limits>
#include <algorithm>
#include <sstream> // Para statusText
#include <iomanip> // Para std::setprecision
#include <cstdio> // Para std::remove (discard brain)
#include <random> // Para generateSingleObstacle

// --- Construtor ---
Game::Game()
    : window(),
      carView(), networkView(), statusView(),
      font(), // Default construct font (will be loaded in loadAssets)
      road(0, 0),
      statusPanelBackground(),
      statusText(font, sf::String(""), 16)
{
    // Constructor Body
    setupWindowAndViews();
    loadAssets(); // Font is loaded here
    initializeSimulation();

    // Set properties after construction and font loading
    // statusText.setFont(font); // Already called in loadAssets
    // statusText.setCharacterSize(16); // Already set in constructor
    statusText.setFillColor(sf::Color(200, 200, 200));
    statusText.setPosition({15.f, 15.f});
}



Game::~Game() {
    // unique_ptr cuidará da limpeza dos carros, obstáculos e cérebro
}

// --- Configuração Inicial ---

void Game::setupWindowAndViews() {
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    const unsigned int screenWidth = desktopMode.size.x;
    const unsigned int screenHeight = desktopMode.size.y;

    const float totalOriginalWidth = 1200.0f;
    const float statusPanelProportion = 250.0f / totalOriginalWidth;
    const float carCanvasProportion = 200.0f / totalOriginalWidth;
    const float networkCanvasProportion = 750.0f / totalOriginalWidth;

    const float statusPanelWidthActual = screenWidth * statusPanelProportion;
    const float carCanvasWidthActual = screenWidth * carCanvasProportion;
    const float networkCanvasWidthActual = screenWidth * networkCanvasProportion;

    window.create(desktopMode, "Self-Driving Car");
    window.setFramerateLimit(60);

    // Reconfigura a estrada com a largura correta do canvas
    road = Road(carCanvasWidthActual / 2.0f, carCanvasWidthActual * 0.9f, 3);

    // Configura Views
    carView.setSize({carCanvasWidthActual, (float)screenHeight});
    carView.setCenter({carCanvasWidthActual / 2.0f, (float)screenHeight / 2.0f}); // Centro inicial
    carView.setViewport(sf::FloatRect({statusPanelProportion, 0.f}, {carCanvasProportion, 1.f}));

    networkView.setSize({networkCanvasWidthActual, (float)screenHeight});
    networkView.setCenter({networkCanvasWidthActual / 2.0f, (float)screenHeight / 2.0f}); // Centro inicial
    networkView.setViewport(sf::FloatRect({statusPanelProportion + carCanvasProportion, 0.f}, {networkCanvasProportion, 1.f}));

    statusView.setSize({statusPanelWidthActual, (float)screenHeight});
    statusView.setCenter({statusPanelWidthActual / 2.0f, (float)screenHeight / 2.0f}); // Centro inicial
    statusView.setViewport(sf::FloatRect({0.f, 0.f}, {statusPanelProportion, 1.f}));

    // Configura Painel de Status
    statusPanelBackground.setSize({statusPanelWidthActual, (float)screenHeight});
    statusPanelBackground.setFillColor(sf::Color(40, 40, 40));
    statusPanelBackground.setPosition({0.f, 0.f}); // A posição é relativa à view de status
}

void Game::loadAssets() {
    // Tenta carregar a fonte de dois locais comuns
    if (!font.openFromFile("assets/LiberationSans-Regular.ttf")) {
        if (!font.openFromFile("../assets/LiberationSans-Regular.ttf")) {
             std::cerr << "Error loading font from assets/ or ../assets/" << std::endl;
             // Considerar lançar uma exceção ou fechar o jogo se a fonte for essencial
             window.close();
             return;
        }
    }
    statusText.setFont(font);
    statusText.setCharacterSize(16);
    statusText.setFillColor(sf::Color(200, 200, 200));
    statusText.setPosition({15.f, 15.f}); // Posição relativa à view de status
}

void Game::initializeSimulation() {
    // 1. Define estrutura da rede (default ou baseada no primeiro carro AI)
    //    Esta parte é um pouco circular, pois precisamos de um carro para saber os raios,
    //    mas precisamos da estrutura para criar o cérebro do carro.
    //    Vamos criar um carro temporário *sem* cérebro apenas para pegar os raios.
    Car tempCar(0, 0, 30, 50, ControlType::AI); // Carro temporário
    int sensorRays = tempCar.getSensorRayCount();
    if (sensorRays <= 0) {
        std::cerr << "Warning: Default car has 0 sensor rays! Using default inputs (5)." << std::endl;
        sensorRays = 5;
    }
    networkStructure = {sensorRays, 12, 4}; // Estrutura (Entradas, Oculta, Saídas)

    // 2. Cria o vetor de carros AI
    populateCarVector(NUM_AI_CARS, START_Y_POSITION);

    // 3. Tenta carregar o melhor cérebro ou cria um novo
    bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);
    if (bestBrainOfGeneration->loadFromFile(BRAIN_FILENAME)) {
        std::cout << "Loaded best brain from: " << BRAIN_FILENAME << std::endl;
        // Validação básica da estrutura carregada (pode ser mais robusta)
        if (bestBrainOfGeneration->levels.empty() ||
            bestBrainOfGeneration->levels.front().inputs.size() != networkStructure[0] ||
            bestBrainOfGeneration->levels.back().outputs.size() != networkStructure.back()) {
            std::cerr << "Warning: Loaded brain structure mismatch! Creating new default brain." << std::endl;
            bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure); // Cria novo
        }
    } else {
        std::cout << "No saved brain found (" << BRAIN_FILENAME << "). Starting fresh." << std::endl;
        // bestBrainOfGeneration já foi criado com a estrutura correta
    }

    // 4. Aplica o cérebro (carregado ou novo) à geração atual
    applyBrainsToGeneration(NUM_AI_CARS);

    // 5. Gera obstáculos iniciais
    generateInitialObstacles(NUM_OBSTACLES, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);

    // 6. Define carro focado inicial
    if (!cars.empty()) {
        focusedCar = cars[0].get();
        bestCarVisual = focusedCar;
    }
}

// --- Game Loop e Lógica Principal ---

void Game::run() {
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();

        processEvents();

        // Atualiza apenas se não estiver pausado e a janela estiver aberta
        if (!isPaused && window.isOpen()) {
            update(deltaTime);
        }

        // Renderiza sempre (mesmo pausado)
        if (window.isOpen()) {
            render();
        }
    }
}

void Game::processEvents() {
    while (const auto eventOpt = window.pollEvent()) {
        const sf::Event& event = *eventOpt;

        if (event.is<sf::Event::Closed>()) {
            window.close();
            return; // Sai do loop de eventos se a janela fechar
        }

        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            handleKeyPress(*keyPressed);
        }
        // Adicionar outros eventos se necessário (mouse, resize, etc.)
    }
}

void Game::handleKeyPress(const sf::Event::KeyPressed& keyEvent) {
     bool isCtrlOrCmd = keyEvent.control || keyEvent.system; // Ctrl (Linux/Win) ou Cmd (Mac)

    switch (keyEvent.code) {
        case sf::Keyboard::Key::S:
            if (isCtrlOrCmd) saveBestBrain();
            break;
        case sf::Keyboard::Key::D:
             // if (isCtrlOrCmd) // Remover Ctrl se não for mais necessário
             discardSavedBrain();
            break;
        case sf::Keyboard::Key::P:
            togglePause();
            break;
        case sf::Keyboard::Key::R:
            resetGeneration();
            break;
        case sf::Keyboard::Key::N:
        case sf::Keyboard::Key::B:
            if (!manualNavigationActive) startManualNavigation();
            navigateManual(keyEvent.code);
            break;
        case sf::Keyboard::Key::Enter:
            stopManualNavigation();
            break;
        case sf::Keyboard::Key::Escape:
            window.close();
            break;
        default:
            // Tecla não mapeada para ações do jogo
            break;
    }
}


void Game::update(sf::Time deltaTime) {
    int nonDamagedCount = 0;

    // 1. Atualiza Carros
    for (auto& carPtr : cars) {
        if(carPtr) {
            carPtr->update(road, obstacleRawPtrs, deltaTime); // Passa ponteiros crus
            if (!carPtr->isDamaged()) {
                nonDamagedCount++;
            }
        }
    }

    // 2. Lógica de Fim de Geração
    bool allCarsEffectivelyDone = (nonDamagedCount == 0);
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
            // Cria uma cópia do melhor cérebro *antes* de resetar
             // Usamos o operador de cópia da NeuralNetwork (se existir e for profundo)
             // ou criamos um novo e copiamos os dados. Assumindo que o operador= faz cópia profunda.
             *bestBrainOfGeneration = *(carWithBestFitness->brain);
            std::cout << "Selected best brain (Max Fitness = " << maxFitness
                      << ", Final Y = " << carWithBestFitness->position.y << ")" << std::endl;
            saveBestBrain(); // Tenta salvar
        } else {
            std::cout << "No valid best car found or best car had no brain. Keeping previous generation's brain." << std::endl;
            // bestBrainOfGeneration mantém o cérebro anterior
        }

        // Prepara a próxima geração
        std::cout << "--- Preparing Generation " << generationCount + 1 << " ---" << std::endl;
        resetGeneration(); // Chama o método de reset
        std::cout << "--- GENERATION " << generationCount << " READY --- \n" << std::endl;

    } else {
         // 3. Gerenciamento de Obstáculos (se a geração não acabou)
         manageInfiniteObstacles();
    }

    // 4. Atualiza qual carro está focado para visualização
    updateFocus();

    // 5. Atualiza o texto do painel de status
    updateStatusPanel();
}

void Game::manageInfiniteObstacles() {
     // Encontra o carro não danificado mais avançado (menor Y)
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

     if (!bestCarVisual) return; // Sem carros vivos, não faz nada

     const float generationMinY = bestCarVisual->position.y + GENERATION_ZONE_END_Y;
     const float generationMaxY = bestCarVisual->position.y + GENERATION_ZONE_START_Y;
     const float removalY = bestCarVisual->position.y + OBSTACLE_REMOVAL_DISTANCE;

     // 1. Remover obstáculos muito atrás
      auto new_end = std::remove_if(obstacles.begin(), obstacles.end(),
          [&](const std::unique_ptr<Obstacle>& obsPtr) {
              if (!obsPtr) return true; // Remove ponteiros nulos
              return obsPtr->position.y > removalY;
          });
     obstacles.erase(new_end, obstacles.end());

     // **IMPORTANTE:** Sincronizar obstacleRawPtrs APÓS remover de obstacles
     obstacleRawPtrs.clear();
     obstacleRawPtrs.reserve(obstacles.size());
     for (const auto& obsPtr : obstacles) {
          if(obsPtr) obstacleRawPtrs.push_back(obsPtr.get());
     }


     // 2. Gerar novos obstáculos se a contagem estiver baixa
     while (obstacles.size() < NUM_OBSTACLES) {
          const float minW = 20.0f, maxW = 40.0f, minH = 40.0f, maxH = 80.0f;
          const sf::Color obsColor = sf::Color(128, 128, 128); // Cinza
          const float minGapAdj = 75.0f, minGapSame = 150.0f;

          // Usa o método da classe para gerar o obstáculo
          auto newObstacle = generateSingleObstacle(generationMinY, generationMaxY, minW, maxW, minH, maxH, obsColor, minGapAdj, minGapSame);

          if (newObstacle) {
             obstacleRawPtrs.push_back(newObstacle.get()); // Adiciona o ponteiro cru ANTES de mover
             obstacles.push_back(std::move(newObstacle)); // Move o unique_ptr para o vetor
          } else {
             // std::cerr << "Warning: Failed to place a new obstacle this frame." << std::endl;
             break; // Para de tentar neste frame se falhar em colocar um
          }
     }
}


void Game::updateFocus() {
    if (!manualNavigationActive) {
        // Foco automático segue o melhor carro visual (o mais avançado)
        // bestCarVisual já foi atualizado em manageInfiniteObstacles
        focusedCar = bestCarVisual;
    } else {
        // Navegação manual ativa
        if (navigableCars.empty() || currentNavIndex < 0 || currentNavIndex >= navigableCars.size()) {
            // Se a lista está vazia ou índice inválido, desativa navegação
            stopManualNavigation();
            focusedCar = bestCarVisual; // Volta para o foco automático
        } else {
            // Verifica se o carro selecionado ainda existe e não está danificado
            Car* potentialFocus = navigableCars[currentNavIndex];
            bool stillExistsAndAlive = false;
            for(const auto& carPtr : cars) { // Verifica no vetor principal 'cars'
                if(carPtr.get() == potentialFocus && !potentialFocus->isDamaged()) {
                    stillExistsAndAlive = true;
                    break;
                }
            }

            if (stillExistsAndAlive) {
                focusedCar = potentialFocus; // Mantém o foco manual
            } else {
                // Carro focado manualmente foi danificado ou removido
                navigableCars.erase(navigableCars.begin() + currentNavIndex); // Remove da lista navegável
                if (navigableCars.empty()) {
                    // Se era o último, desativa navegação
                    stopManualNavigation();
                    focusedCar = bestCarVisual; // Volta para o foco automático
                } else {
                    // Seleciona o próximo (ou anterior, com wrap around)
                    currentNavIndex %= navigableCars.size();
                    focusedCar = navigableCars[currentNavIndex];
                }
            }
        }
    }

    // Fallback: se nenhum carro focado, tenta focar o melhor visual ou o primeiro carro
    if (!focusedCar) {
        if (bestCarVisual) {
            focusedCar = bestCarVisual;
        } else if (!cars.empty() && cars[0]) {
            focusedCar = cars[0].get();
        }
        // Se ainda assim for nulo, não há carros válidos
    }
}


void Game::updateStatusPanel() {
     int nonDamagedCount = 0;
     for (const auto& carPtr : cars) {
         if (carPtr && !carPtr->isDamaged()) {
             nonDamagedCount++;
         }
     }

    std::ostringstream statusStream;
    statusStream << std::fixed << std::setprecision(1);
    statusStream << "Generation: " << generationCount << "\n";
    statusStream << "Time: " << generationClock.getElapsedTime().asSeconds() << "s\n";
    if (isPaused) { statusStream << "\n--- PAUSED ---\n\n"; } else { statusStream << "\n"; }
    statusStream << "Alive: " << nonDamagedCount << " / " << cars.size() << "\n";
    statusStream << "Obstacles: " << obstacles.size() << "\n\n"; // Mostra contagem atual

    if (manualNavigationActive && !navigableCars.empty() && focusedCar) {
         // Recalcula o rank atual do carro focado dentro da lista 'navigableCars'
         int currentRank = -1;
         // Ordena temporariamente para garantir a ordem correta do rank (pode otimizar se a ordem for estável)
         std::sort(navigableCars.begin(), navigableCars.end(), [](const Car* a, const Car* b) {
             return a->position.y < b->position.y; // Menor Y = mais avançado = menor rank
         });
         auto it = std::find(navigableCars.begin(), navigableCars.end(), focusedCar);
         if (it != navigableCars.end()) {
             currentRank = static_cast<int>(std::distance(navigableCars.begin(), it)) + 1;
         }

         statusStream << "Manual Nav: ON (Rank " << (currentRank != -1 ? std::to_string(currentRank) : "?") << ")\n";
         statusStream << "Focus Fitness: " << focusedCar->getFitness() << "\n";
         statusStream << "Focus Y Pos: " << focusedCar->position.y << "\n";
         statusStream << "Focus Speed: " << focusedCar->getSpeed() << "\n";
    } else if (focusedCar){
         statusStream << "Manual Nav: OFF\n";
         statusStream << "Focus Fitness: " << focusedCar->getFitness() << "\n";
         statusStream << "Focus Y Pos: " << focusedCar->position.y << "\n";
         statusStream << "Focus Speed: " << focusedCar->getSpeed() << "\n";
    } else {
         statusStream << "Manual Nav: OFF\nFocus: N/A\n";
    }
    statusText.setString(statusStream.str());
}


void Game::render() {
    window.clear(sf::Color(100, 100, 100)); // Fundo cinza

    // 1. Desenha Painel de Status
    window.setView(statusView);
    window.draw(statusPanelBackground);
    window.draw(statusText);

    // 2. Desenha Simulação (Car View)
    if (focusedCar) {
        // Centraliza a view um pouco acima do carro focado
        float viewCenterX = carView.getSize().x / 2.0f;
        float viewCenterY = focusedCar->position.y - window.getSize().y * 0.3f;
        carView.setCenter({viewCenterX, viewCenterY});
    } else {
        // Reseta para o centro padrão se não houver carro focado
        carView.setCenter({carView.getSize().x / 2.0f, carView.getSize().y / 2.0f});
    }
    window.setView(carView);
    road.draw(window);
    for (const auto& obsPtr : obstacles) { if(obsPtr) obsPtr->draw(window); }

    // Desenha outros carros (não focados) primeiro
    for (const auto& carPtr : cars) {
         if (carPtr && carPtr.get() != focusedCar) {
             carPtr->draw(window, false); // false = não desenhar sensor
         }
    }
    // Desenha o carro focado por último (e seu sensor, se for o caso)
    if(focusedCar) {
        focusedCar->draw(window, true); // true = desenhar sensor
    }

    // 3. Desenha Rede Neural (Network View)
    window.setView(networkView);
    // Fundo da view da rede
    sf::RectangleShape networkBackground(networkView.getSize());
    networkBackground.setFillColor(sf::Color(60, 60, 60));
    // networkBackground.setPosition(networkView.getCenter() - networkView.getSize() / 2.f); // Posição relativa à view
    networkBackground.setPosition({0.f, 0.f}); // Origem da view é (0,0)
    window.draw(networkBackground);

    NeuralNetwork* brainToDraw = nullptr;
    if (focusedCar && focusedCar->useBrain && focusedCar->brain) {
        brainToDraw = &(*(focusedCar->brain));
    }

    if (brainToDraw && !brainToDraw->levels.empty()) {
        // Passa as coordenadas e tamanho da área de desenho para Visualizer
        Visualizer::drawNetwork(window, *brainToDraw, font,
                                 0.f, 0.f, // Posição inicial (relativa à view)
                                 networkView.getSize().x, networkView.getSize().y);
    } else {
        sf::Text noBrainText(font, "No brain to display", 20);
        noBrainText.setFillColor(sf::Color::White);
         sf::FloatRect textBounds = noBrainText.getLocalBounds();
         noBrainText.setOrigin({textBounds.position.x + textBounds.size.x/2.0f , textBounds.position.y + textBounds.size.y/2.0f});
         noBrainText.setPosition({networkView.getSize().x / 2.0f, networkView.getSize().y / 2.0f}); // Centraliza na view
        window.draw(noBrainText);
    }

    window.display();
}


// --- Métodos de Ação ---

void Game::resetGeneration() {
    std::cout << "--- RESETTING Generation " << generationCount << " ---" << std::endl;
    generationCount++; // Incrementa a contagem da geração

    // 1. Reseta estado dos carros
    for (auto& carPtr : cars) {
        if (carPtr) carPtr->resetForNewGeneration(START_Y_POSITION, road);
    }

    // 2. Reaplica o cérebro (melhor da geração anterior com mutações)
    //    bestBrainOfGeneration já deve conter o melhor cérebro aqui
    applyBrainsToGeneration(NUM_AI_CARS);

    // 3. Gera novos obstáculos
    generateInitialObstacles(NUM_OBSTACLES, -1500.0f, -100.0f, 20.0f, 40.0f, 40.0f, 80.0f);

    // 4. Reseta estados de controle e foco
    manualNavigationActive = false;
    currentNavIndex = -1;
    navigableCars.clear();
    focusedCar = cars.empty() ? nullptr : cars[0].get();
    bestCarVisual = focusedCar; // Assume o primeiro como melhor visual inicialmente

    // 5. Reseta clocks e pausa
    clock.restart();
    generationClock.restart();
    isPaused = false;

    std::cout << "--- Generation " << generationCount << " Started ---\n" << std::endl;
}


void Game::saveBestBrain() {
    if (bestBrainOfGeneration) {
        if (bestBrainOfGeneration->saveToFile(BRAIN_FILENAME)) {
            std::cout << "Saved Best Generation Brain to " << BRAIN_FILENAME << std::endl;
        } else {
            std::cerr << "Error saving brain." << std::endl;
        }
    } else {
         std::cerr << "No best brain available to save." << std::endl;
    }
}

void Game::discardSavedBrain() {
    if (std::remove(BRAIN_FILENAME.c_str()) == 0) {
        std::cout << "Discarded saved brain file: " << BRAIN_FILENAME << std::endl;
    } else {
        std::cout << "No brain file to discard or error removing." << std::endl;
    }
     // Opcional: Recarregar/Resetar o cérebro atual para um estado padrão?
     // bestBrainOfGeneration = std::make_unique<NeuralNetwork>(networkStructure);
     // applyBrainsToGeneration(NUM_AI_CARS); // Reaplica o novo cérebro padrão
}

void Game::togglePause() {
    isPaused = !isPaused;
    std::cout << (isPaused ? "GAME PAUSED" : "GAME RESUMED") << std::endl;
}

void Game::startManualNavigation() {
    manualNavigationActive = true;
    navigableCars.clear();
    for (const auto& carPtr : cars) {
        if (carPtr && !carPtr->isDamaged()) {
            navigableCars.push_back(carPtr.get());
        }
    }
    if (navigableCars.empty()) {
        manualNavigationActive = false;
        currentNavIndex = -1;
        std::cout << "No cars available to navigate." << std::endl;
        return;
    }

    // Ordena por posição Y (menor Y = mais avançado = menor rank)
    std::sort(navigableCars.begin(), navigableCars.end(), [](const Car* a, const Car* b) {
        return a->position.y < b->position.y;
    });

    // Tenta encontrar o carro focado atualmente na lista ordenada
    currentNavIndex = 0; // Default para o primeiro (rank 1)
    if (focusedCar) {
        auto it = std::find(navigableCars.begin(), navigableCars.end(), focusedCar);
        if (it != navigableCars.end()) {
            currentNavIndex = static_cast<int>(std::distance(navigableCars.begin(), it));
        }
    }

    focusedCar = navigableCars[currentNavIndex]; // Foca no carro selecionado
    std::cout << "Manual Navigation ON. Cars available: " << navigableCars.size()
              << ". Current Rank: " << (currentNavIndex + 1) << std::endl;
}

void Game::navigateManual(sf::Keyboard::Key key) {
    if (!manualNavigationActive || navigableCars.empty()) return;

    if (key == sf::Keyboard::Key::N) { // Próximo
        currentNavIndex = (currentNavIndex + 1) % navigableCars.size();
    } else if (key == sf::Keyboard::Key::B) { // Anterior
        currentNavIndex = (currentNavIndex - 1 + navigableCars.size()) % navigableCars.size();
    }

    focusedCar = navigableCars[currentNavIndex]; // Atualiza o foco
    std::cout << "Navigating to Rank: " << (currentNavIndex + 1) << std::endl;
     // updateFocus() irá revalidar se o carro ainda está vivo
}

void Game::stopManualNavigation() {
    if (manualNavigationActive) {
        manualNavigationActive = false;
        currentNavIndex = -1;
        navigableCars.clear();
        std::cout << "Manual Navigation OFF." << std::endl;
        // updateFocus() cuidará de selecionar o foco automático (bestCarVisual)
    }
}

// --- Funções Auxiliares (Movidas para a classe) ---

void Game::populateCarVector(int N, float startY) {
    cars.clear(); // Limpa o vetor existente
    cars.reserve(N);
    for (int i = 0; i < N; ++i) {
        // Cria carros AI com cores aleatórias
        cars.push_back(std::make_unique<Car>(
            road.getLaneCenter(1), // Começa na faixa do meio (índice 1)
            startY,
            30.0f, 50.0f,          // Largura e Altura
            ControlType::AI,
            3.0f,                  // Max Speed
            getRandomColor()       // Cor Aleatória
        ));
    }
    std::cout << "Generated " << cars.size() << " AI cars." << std::endl;
}

void Game::generateInitialObstacles(int N, float minY, float maxY, float minW, float maxW, float minH, float maxH) {
    obstacles.clear();
    obstacleRawPtrs.clear();
    obstacles.reserve(N);
    const sf::Color obstacleColor = sf::Color(128, 128, 128); // Cinza
    const float minVerticalGapAdjacentLane = 55.0f;
    const float minVerticalGapSameLane = 75.0f;
    int obstaclesPlaced = 0;
    int totalAttemptsOverall = 0;
    const int maxTotalAttempts = N * 25 * 5; // Limite generoso de tentativas totais

    while (obstaclesPlaced < N && totalAttemptsOverall < maxTotalAttempts) {
        totalAttemptsOverall++;
        // Chama generateSingleObstacle para tentar adicionar um
        auto newObstacle = generateSingleObstacle(minY, maxY, minW, maxW, minH, maxH, obstacleColor, minVerticalGapAdjacentLane, minVerticalGapSameLane);

        if (newObstacle) {
            obstacleRawPtrs.push_back(newObstacle.get()); // Adiciona ponteiro cru
            obstacles.push_back(std::move(newObstacle)); // Adiciona unique_ptr
            obstaclesPlaced++;
        } else {
            // std::cerr << "Warning: Failed to place initial obstacle #" << obstaclesPlaced + 1 << std::endl;
            // Continua tentando gerar outros
        }
    }

    if (obstaclesPlaced < N) {
        std::cerr << "Warning: Only generated " << obstaclesPlaced << " out of " << N
                  << " requested initial obstacles due to spacing constraints." << std::endl;
    }
    std::cout << "Generated " << obstacles.size() << " initial obstacles." << std::endl;
}


// Corrected definition signature in src/Game.cpp
std::unique_ptr<Obstacle> Game::generateSingleObstacle(
    float minY, float maxY,
    float minW, float maxW, float minH, float maxH,
    const sf::Color& color,
    float minVerticalGapAdjacentLane, float minVerticalGapSameLane,
    int maxPlacementRetries) // Default argument value only goes in declaration
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
        for (const auto& existingObsPtr : obstacles) { // Check against ALL existing
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



void Game::applyBrainsToGeneration(int N) {
     if (cars.empty() || !bestBrainOfGeneration) {
          std::cerr << "Error: Cannot apply brains - no cars or no best brain available." << std::endl;
          return;
     }

     // Elitismo: Melhor cérebro no primeiro carro
     if (cars[0]->useBrain) {
         // Garante que o primeiro carro tenha um cérebro (se não tiver, cria um baseado no bestBrain)
         if (!cars[0]->brain) {
             cars[0]->brain.emplace(*bestBrainOfGeneration); // Cria cópia
         } else {
             *(cars[0]->brain) = *bestBrainOfGeneration; // Copia o conteúdo (operador=)
         }
         // Opcional: Zerar mutação para o elite?
         // NeuralNetwork::mutate(*(cars[0]->brain), 0.0f);
     }

     // Mutação para os outros carros AI
     for (int i = 1; i < cars.size(); ++i) {
         if (cars[i] && cars[i]->useBrain) {
              // Garante que o carro tenha um cérebro
              if (!cars[i]->brain) {
                 cars[i]->brain.emplace(*bestBrainOfGeneration); // Cria cópia do melhor
             } else {
                 *(cars[i]->brain) = *bestBrainOfGeneration; // Copia base do melhor
             }
             // Aplica mutação
             NeuralNetwork::mutate(*(cars[i]->brain), 0.15f); // Taxa de mutação
         }
     }
     std::cout << "Applied brains to generation " << generationCount << "." << std::endl;
}