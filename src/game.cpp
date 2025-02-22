#include "game.hpp"
#include <iostream>

Game::Game() {
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    unsigned int desktopWidth = desktopMode.size.x;
    unsigned int desktopHeight = desktopMode.size.y;

    float xAxisFactor = 0.2f;
    float yAxisFactor = 0.15f;
    
    windowWidth = static_cast<unsigned int>((1 - xAxisFactor) * desktopWidth);
    windowHeight = static_cast<unsigned int>((1 - yAxisFactor) * desktopHeight);
    windowScale = 1;

    std::cout << "Resolução da tela: " << desktopWidth << " x " << desktopHeight << std::endl;
    std::cout << "Resolução do jogo: " << windowWidth << " x " << windowHeight << std::endl;

    window.create(sf::VideoMode({windowWidth, windowHeight}), "Self Driven Car");
    window.setFramerateLimit(60);
}

void Game::run() {

    while (running()) {
        handleEvents();
        update();
        render();
    }
    clean();
}

void Game::handleEvents() {
    while (const std::optional event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>())
            window.close();
    }

}

void Game::update() {

    car.updateMovement();
}

void Game::render() {
    
    window.clear(sf::Color::White);
    window.draw(car);
    window.display();
}

void Game::clean() {
    window.close();
}

bool Game::running() {
    return window.isOpen();
}
