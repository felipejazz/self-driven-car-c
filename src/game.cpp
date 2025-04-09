#include "game.hpp"
#include "util.hpp"
#include <iostream>


Game::Game(int numCars,ControlType cType)
    : numCars(numCars),
      mainCarType(cType),
      road(0.001f, 3, 800, 600),
      m_generation(1),
      m_populationSize(numCars),
      m_mutationRate(0.1f),
      m_bestCarStuckTime(0.0f),
      m_bestBrain(nullptr)
{
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    unsigned int desktopWidth  = desktopMode.size.x;
    unsigned int desktopHeight = desktopMode.size.y;

    float xAxisFactor = 0.2f;
    float yAxisFactor = 0.15f;

    windowWidth  = static_cast<unsigned int>((1.0f - xAxisFactor) * desktopWidth);
    windowHeight = static_cast<unsigned int>((1.0f - yAxisFactor) * desktopHeight);
    windowScale  = 1;

    std::cout << "Resolução da tela: " << desktopWidth << " x " << desktopHeight << std::endl;
    std::cout << "Resolução do jogo: " << windowWidth << " x " << windowHeight << std::endl;

    unsigned int roadWidth = windowWidth - static_cast<unsigned int>(windowWidth * 0.8f);
    float centerX = windowWidth * 0.5f; 
    road = Road(
        centerX,
        static_cast<float>(roadWidth),
        3,
        0.1f
    );

    window.create(sf::VideoMode({windowWidth, windowHeight}), "Self Driven Car");
    window.setFramerateLimit(60);

    populate();
}

Game::~Game() {
    clean();
    delete m_bestBrain;
}



void Game::run() {
    sf::Clock clock;
    while (running()) {
        float deltaTime = clock.restart().asSeconds();

        handleEvents();
        update(deltaTime);
        render();
    }
    clean();
}

bool Game::running() {
    return window.isOpen();
}

void Game::handleEvents() {
    while (auto e = window.pollEvent()) {
        if (e->is<sf::Event::Closed>()) {
            window.close();
            continue;
        }

        auto keyPressed = e->getIf<sf::Event::KeyPressed>();
        if (keyPressed) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                window.close();
            }
            if (keyPressed->code == sf::Keyboard::Key::R) {
                resetGame();
            }
            if (!cars.empty()) {
                if (cars[0]->isKeyboard()) {
                    cars[0]->getControls()->setKeyPressed(keyPressed->code);
                }
            }
            continue;
        }

        auto keyReleased = e->getIf<sf::Event::KeyReleased>();
        if (keyReleased) {
            if (!cars.empty()) {
                if (cars[0]->isKeyboard()) {
                    cars[0]->getControls()->setKeyReleased(keyReleased->code);
                }
            }
            continue;
        }
    }
}

void Game::resetGame() {
    for (auto &car : cars) {
        delete car;
    }
    cars.clear();
    populate();
}

void Game::populate() {
    float spawnX = road.getLaneCenter(1); // lane do meio
    float spawnY = 300.f;
    
    if (mainCarType == ControlType::AI) {
        // Create AI cars - all in the middle lane at the same position
        for (int i = 0; i < numCars; i++) {
            createCar(static_cast<int>(spawnX), static_cast<int>(spawnY), ControlType::AI);
            
            // If we have a best brain from previous generation, use it with mutation
            if (m_bestBrain && i > 0) {
                NeuralNetwork* brain = m_bestBrain->clone();
                brain->mutate(m_mutationRate);
                cars.back()->setBrain(brain);
            }
        }
    } else {
        // Original populate logic for non-AI mode
        createCar(static_cast<int>(spawnX), static_cast<int>(spawnY), mainCarType);
        
        for (int i = 1; i < numCars; i++) {
            int lane = i % 3;
            float sx = road.getLaneCenter(lane);
            float sy = 300.f - i * 150.f;
            createCar(static_cast<int>(sx), static_cast<int>(sy), ControlType::DUMMY);
        }
    }
}
void Game::createCar(int xPos, int yPos, ControlType ctype) {
    Car* newCar = new Car(xPos, yPos, ctype);
    cars.push_back(newCar);
}

void Game::update(float deltaTime) {
    // Update all cars
    for (auto &car : cars) {
        bool hitBoundary = (car->getControlType() != ControlType::DUMMY) 
                           && !road.isWithinRoad(*car);
        
        car->updateMovement(deltaTime, road.getFriction(), hitBoundary);
        
        // Update fitness for AI cars
        if (car->isAi()) {
            car->updateFitness();
            car->checkIfStuck(deltaTime);
            car->checkIfFollowing(cars);
        }
    }
    
    // Update sensors
    for (auto &car : cars) {
        car->updateSensor(road.getBorders(), cars);
    }
    
    // Check for collisions with specific rules:
    // 1. AI cars don't collide with each other
    // 2. Traffic cars don't collide with each other
    // 3. AI cars collide with traffic and walls
    // 4. Traffic cars collide with walls
    
    // First, check AI cars against traffic
    size_t aiCarCount = (mainCarType == ControlType::AI) ? numCars : 0;
    
    // AI cars collide with traffic
    for (size_t i = 0; i < aiCarCount; ++i) {
        // Skip damaged AI cars
        if (cars[i]->isDamaged()) continue;
        
        // Check collision with traffic cars
        for (size_t j = aiCarCount; j < cars.size(); ++j) {
            if (polysIntersect(cars[i]->getPolygon(), cars[j]->getPolygon())) {
                cars[i]->stop();
                cars[i]->setDamage();
                cars[j]->stop();
                cars[j]->setDamage();
            }
        }
        
        // Check collision with road borders
        for (const auto& border : road.getBorders()) {
            if (polysIntersect(cars[i]->getPolygon(), border)) {
                cars[i]->stop();
                cars[i]->setDamage();
            }
        }
    }
    
    // Traffic cars only collide with road borders
    for (size_t i = aiCarCount; i < cars.size(); ++i) {
        // Skip damaged traffic cars
        if (cars[i]->isDamaged()) continue;
        
        // Check collision with road borders
        for (const auto& border : road.getBorders()) {
            if (polysIntersect(cars[i]->getPolygon(), border)) {
                cars[i]->stop();
                cars[i]->setDamage();
            }
        }
    }
    
    // Find the best car for camera focus
    Car* bestCar = findBestCar();
    if (bestCar) {
        // Mark the best car
        for (auto& car : cars) {
            car->setIsBest(car == bestCar);
        }
        
        // Check if best car is stuck
        if (bestCar->getStuckTime() > 5.0f) {
            std::cout << "Generation " << m_generation << " ended - best car stuck for 5 seconds" << std::endl;
            evolve();
        }
    }
    
    // Update road based on best car's speed
    float playerSpeed = bestCar ? bestCar->getSpeed() : 0.f;
    road.update(deltaTime, playerSpeed);
    
    // Always spawn traffic, even in AI mode
    spawnTraffic(deltaTime);
    updateTraffic(deltaTime);
}
void Game::spawnTraffic(float deltaTime) {
    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval) {
        spawnTimer = 0.f;

        if (!cars.empty()) {
            // Get the Y position of the reference car (first car or best car)
            float referenceY;
            if (mainCarType == ControlType::AI) {
                Car* bestCar = findBestCar();
                referenceY = bestCar ? bestCar->getY() : cars[0]->getY();
            } else {
                referenceY = cars[0]->getY();
            }
            
            int lane = getLaneAvailable();
            float sx = road.getLaneCenter(lane);
            float sy = referenceY - 850.f;

            // Verificar se há espaço suficiente para spawnar um novo carro
            bool canSpawn = true;
            
            // Verificar colisão com outros carros de tráfego
            size_t aiCarCount = (mainCarType == ControlType::AI) ? numCars : 0;
            for (size_t i = aiCarCount; i < cars.size(); ++i) {
                if (std::fabs(cars[i]->getY() - sy) < 150.f &&
                    std::fabs(cars[i]->getX() - sx) < 60.f) {
                    canSpawn = false;
                    break;
                }
            }
            
            if (canSpawn) {
                createCar(static_cast<int>(sx), static_cast<int>(sy), ControlType::DUMMY);
            }
        }
    }
}

void Game::updateTraffic(float deltaTime) {
    const int DESIRED_CARS = 30;

    if (cars.empty()) return;

    // Get the Y position of the reference car (first car or best car)
    float referenceY;
    if (mainCarType == ControlType::AI) {
        Car* bestCar = findBestCar();
        referenceY = bestCar ? bestCar->getY() : cars[0]->getY();
    } else {
        referenceY = cars[0]->getY();
    }

    float removeThreshold = 1500.0f;

    // Start from index that skips AI cars
    size_t startIndex = (mainCarType == ControlType::AI) ? numCars : 1;
    
    for (size_t i = startIndex; i < cars.size();) {
        Car* c = cars[i];
        if (c->getY() > referenceY + removeThreshold) {
            delete c;
            cars.erase(cars.begin() + i);
        } else {
            i++;
        }
    }

    // Adicionar carros de tráfego até atingir o número desejado
    while (static_cast<int>(cars.size()) < DESIRED_CARS + (mainCarType == ControlType::AI ? numCars : 0)) {
        int lane = getLaneAvailable();
        float spawnX = road.getLaneCenter(lane);
        float spawnY = referenceY - 1000.0f;
        
        // Verificar se há espaço suficiente para spawnar um novo carro
        bool canSpawn = true;
        
        // Verificar colisão com outros carros de tráfego
        size_t aiCarCount = (mainCarType == ControlType::AI) ? numCars : 0;
        for (size_t i = aiCarCount; i < cars.size(); ++i) {
            if (std::fabs(cars[i]->getY() - spawnY) < 150.f &&
                std::fabs(cars[i]->getX() - spawnX) < 60.f) {
                canSpawn = false;
                break;
            }
        }
        
        if (canSpawn) {
            createCar(static_cast<int>(spawnX), static_cast<int>(spawnY), ControlType::DUMMY);
        } else {
            // Se não puder spawnar, tente outra pista ou espere o próximo frame
            break;
        }
    }
}
Car* Game::findBestCar() const {
    if (cars.empty()) return nullptr;
    
    Car* best = nullptr;
    float bestFitness = -std::numeric_limits<float>::max();
    
    for (auto& car : cars) {
        if (car->isAi()) {
            float fitness = car->getFitness();
            if (fitness > bestFitness) {
                bestFitness = fitness;
                best = car;
            }
        }
    }
    
    return best ? best : cars[0];
}

void Game::evolve() {
    m_generation++;
    std::cout << "Starting generation " << m_generation << std::endl;
    
    // Save the best brain
    Car* bestCar = findBestCar();
    if (bestCar && bestCar->getBrain()) {
        delete m_bestBrain;
        m_bestBrain = bestCar->getBrain()->clone();
    }
    
    // Reset the game with the new generation
    resetGame();
}

int Game::getLaneAvailable() {
    int laneCount = 3;
    std::vector<int> laneCounts(laneCount, 0);

    for (auto car : cars) {
        int lane = road.getLaneFromPosition(car->getX());
        laneCounts[lane]++;
    }

    int availableLane = 0;
    for (int i = 1; i < laneCount; ++i) {
        if (laneCounts[i] < laneCounts[availableLane]) {
            availableLane = i;
        }
    }
    return availableLane;
}

void Game::render() {
    window.clear(sf::Color(100, 100, 100));

    sf::View view;
    view.setSize(sf::Vector2f(windowWidth, windowHeight));

    // Focus camera on the best car
    Car* bestCar = findBestCar();
    if (bestCar) {
        float desiredY = bestCar->getY() - (windowHeight * 0.4f);
        float desiredX = bestCar->getX();
        view.setCenter(sf::Vector2f(desiredX, desiredY));
    }
    window.setView(view);

    road.render(window);
    for (auto &car : cars) {
        window.draw(*car);
    }
    
    // Draw generation info
    if (mainCarType == ControlType::AI) {
        sf::View defaultView = window.getDefaultView();
        window.setView(defaultView);
        
        static bool fontLoaded = false;
        static sf::Font s_font;
        
        if (!fontLoaded) {
            // Tente vários caminhos possíveis para a fonte
            if (s_font.openFromFile("../resources/LiberationSans-Regular.ttf")) {
                fontLoaded = true;
            } else if (s_font.openFromFile("resources/LiberationSans-Regular.ttf")) {
                fontLoaded = true;
            } else if (s_font.openFromFile("/usr/share/fonts/TTF/LiberationSans-Regular.ttf")) {
                fontLoaded = true;
            } else if (s_font.openFromFile("/usr/share/fonts/liberation/LiberationSans-Regular.ttf")) {
                fontLoaded = true;
            } else {
                std::cout << "Não foi possível carregar nenhuma fonte. Informações de geração não serão exibidas." << std::endl;
                std::cout << "Procurando em: " << std::endl;
                std::cout << "  - ../resources/LiberationSans-Regular.ttf" << std::endl;
                std::cout << "  - resources/LiberationSans-Regular.ttf" << std::endl;
                std::cout << "  - /usr/share/fonts/TTF/LiberationSans-Regular.ttf" << std::endl;
                std::cout << "  - /usr/share/fonts/liberation/LiberationSans-Regular.ttf" << std::endl;
            }
        }
        
        if (fontLoaded) {
            // Crie o texto usando o construtor correto
            sf::Text genText(s_font, "Generation: " + std::to_string(m_generation), 24);
            genText.setFillColor(sf::Color::White);
            genText.setPosition(sf::Vector2f(10, 10));
            window.draw(genText);
        } else {
            // Fallback para quando não há fonte disponível
            sf::RectangleShape genBackground(sf::Vector2f(120, 30));
            genBackground.setFillColor(sf::Color(0, 0, 0, 128));
            genBackground.setPosition(sf::Vector2f(10, 10));
            window.draw(genBackground);
            
            // Desenhe barras para representar a geração
            for (int i = 0; i < std::min(m_generation, 10); i++) {
                sf::RectangleShape bar(sf::Vector2f(8, 20));
                bar.setFillColor(sf::Color::White);
                bar.setPosition(sf::Vector2f(15 + i * 10, 15));
                window.draw(bar);
            }
        }
        
        window.setView(view);
    }

    window.display();
}

void Game::clean() {
    for (auto &car : cars) {
        delete car;
    }
    cars.clear();
    window.close();
}
