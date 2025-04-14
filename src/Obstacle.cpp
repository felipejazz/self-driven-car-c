#include "Obstacle.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>
#include <atomic>
#include <iostream> // Added for potential error messages

std::atomic<long long> Obstacle::nextId(0);

Obstacle::Obstacle(float x, float y, float w, float h, sf::Color col)
    : position(x, y), width(w), height(h), color(col),
      id(nextId.fetch_add(1, std::memory_order_relaxed)),
      sprite(texture) // Initialize sprite with texture reference
{
    updatePolygon();
    loadTexture("assets/obstacle.png"); // Attempt to load obstacle sprite
    if (!textureLoaded) {
        // Optional: Add a warning if the texture fails to load
        // std::cerr << "Warning: Failed to load obstacle texture (assets/obstacle.png)." << std::endl;
    }
}

long long Obstacle::getId() const {
    return id;
}

void Obstacle::loadTexture(const std::string& filename) {
    textureLoaded = false;
    if (!texture.loadFromFile(filename)) return;
    texture.setSmooth(true);
    sprite.setTexture(texture, true);
    sf::FloatRect textureRect = sprite.getLocalBounds();
    if (textureRect.size.x > 0 && textureRect.size.y > 0) {
        sprite.setScale({width / textureRect.size.x, height / textureRect.size.y});
    } else {
        sprite.setScale({1.0f, 1.0f});
        // Handle error case: texture loaded but has zero dimensions?
        return;
    }
    sprite.setOrigin({textureRect.size.x / 2.0f, textureRect.size.y / 2.0f});
    sprite.setColor(color); // Apply the obstacle's color tint
    textureLoaded = true;
}

void Obstacle::updatePolygon() {
    polygon.resize(4);
    const float rad = std::hypot(width, height) / 2.0f;
    const float alpha = std::atan2(width, height);

    polygon[0] = { position.x + std::sin(alpha) * rad, position.y - std::cos(alpha) * rad };
    polygon[1] = { position.x + std::sin(-alpha) * rad, position.y - std::cos(-alpha) * rad };
    polygon[2] = { position.x + std::sin(static_cast<float>(M_PI) + alpha) * rad, position.y - std::cos(static_cast<float>(M_PI) + alpha) * rad };
    polygon[3] = { position.x + std::sin(static_cast<float>(M_PI) - alpha) * rad, position.y - std::cos(static_cast<float>(M_PI) - alpha) * rad };
}

std::vector<sf::Vector2f> Obstacle::getPolygon() const {
    return polygon;
}

void Obstacle::draw(sf::RenderTarget& target) const {
    if (textureLoaded) {
        // SFML Sprite's position and origin are already set relative to its center.
        // Rotation is not needed for obstacles based on the original code.
        // Create a mutable copy to set position for drawing
        sf::Sprite currentSprite = sprite;
        currentSprite.setPosition(position);
        target.draw(currentSprite);
    } else {
        sf::RectangleShape rectShape({width, height});
        rectShape.setOrigin({width / 2.0f, height / 2.0f});
        rectShape.setPosition(position);
        rectShape.setFillColor(color);
        rectShape.setOutlineColor(sf::Color::Black);
        rectShape.setOutlineThickness(1.f);
        target.draw(rectShape);
    }
}