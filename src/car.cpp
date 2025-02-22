#include "car.hpp"
#include <SFML/Window/Keyboard.hpp>

Car::Car() {
    setSize(sf::Vector2f(50.f, 50.f));
    setFillColor(sf::Color::Red);
    setPosition(sf::Vector2f(375.f, 275.f));
    speed = 5.0;
}

void Car::updateMovement() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        move(sf::Vector2f(-speed, 0.f));
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        move(sf::Vector2f(speed, 0.f));
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        move(sf::Vector2f(0.f, -speed));
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        move(sf::Vector2f(0.f, speed));
}
