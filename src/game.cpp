#include "game.hpp"
#include "util.hpp"
#include <iostream>


Game::Game(int numCars,ControlType cType)
    : numCars(numCars),
      mainCarType(cType),
      road(0.001f, 3, 800, 600)
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
    createCar(static_cast<int>(spawnX), static_cast<int>(spawnY), mainCarType);


    for (int i = 1; i < numCars; i++) {
        int lane = i % 3; 
        float sx = road.getLaneCenter(lane);
        float sy = 300.f - i * 150.f;
        createCar(static_cast<int>(sx), static_cast<int>(sy), ControlType::DUMMY);
    }
}

void Game::createCar(int xPos, int yPos, ControlType ctype) {
    Car* newCar = new Car(xPos, yPos, ctype);
    cars.push_back(newCar);
}

void Game::update(float deltaTime) {
    for (auto &car : cars) {
        bool hitBoundary = (car->getControlType() != ControlType::DUMMY) 
                           && !road.isWithinRoad(*car);
    
        car->updateMovement(deltaTime, road.getFriction(), hitBoundary);
    }
    for (auto &car : cars) {
        car->updateSensor(road.getBorders(), cars);
    }

    for (size_t i = 0; i < cars.size(); ++i) {
        for (size_t j = i + 1; j < cars.size(); ++j) {
            if (polysIntersect(cars[i]->getPolygon(), cars[j]->getPolygon())) {
                cars[i]->stop();
                cars[i]->setDamage();
                cars[j]->stop();
                cars[j]->setDamage();
            }
        }
    }

    float playerSpeed = 0.f;
    if (!cars.empty()) {
        playerSpeed = cars[0]->getSpeed();
    }
    road.update(deltaTime, playerSpeed);

    spawnTraffic(deltaTime);

    updateTraffic(deltaTime);
}


void Game::spawnTraffic(float deltaTime) {
    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval) {
        spawnTimer = 0.f;

        if (!cars.empty()) {
            float mainCarY = cars[0]->getY();
            int lane = getLaneAvailable();
            float sx = road.getLaneCenter(lane);
            float sy = mainCarY - 850.f;

            bool canSpawn = true;
            for (auto &c : cars) {
                if (std::fabs(c->getY() - sy) < 120.f &&
                    std::fabs(c->getX() - sx) < 50.f) {
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

void Game::updateTraffic(float deltaTime)
{
    const int DESIRED_CARS = 30;


    if (cars.empty()) return;

    Car* mainCar = cars[0];
    float mainCarY = mainCar->getY();

    float removeThreshold = 1500.0f;

    for (size_t i = 1; i < cars.size();)
    {
        Car* c = cars[i];
        if (c->getY() > mainCarY + removeThreshold)
        {
            delete c;
            cars.erase(cars.begin() + i);
        }
        else {
            i++;
        }
    }

    while (static_cast<int>(cars.size()) < DESIRED_CARS)
    {
        int lane = getLaneAvailable();
        float spawnX = road.getLaneCenter(lane);

        float spawnY = mainCarY - 1000.0f; 
        createCar(static_cast<int>(spawnX), static_cast<int>(spawnY), ControlType::DUMMY);
    }
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

    if (!cars.empty()) {
        float desiredY = cars[0]->getY() - (windowHeight * 0.4f);
        float desiredX = cars[0]->getX();
        view.setCenter(sf::Vector2f(desiredX, desiredY));
    }
    window.setView(view);

    road.render(window);
    for (auto &car : cars) {
        window.draw(*car);
    }

    window.setView(window.getDefaultView());
    window.display();
}

void Game::clean() {
    for (auto &car : cars) {
        delete car;
    }
    cars.clear();
    window.close();
}
