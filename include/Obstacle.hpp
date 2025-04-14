#ifndef OBSTACLE_HPP
#define OBSTACLE_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <atomic> // Para ID atômico
#include <string> // Added for texture loading

class Obstacle {
public:
    sf::Vector2f position;
    float width;
    float height;
    sf::Color color;
    std::vector<sf::Vector2f> polygon; // Para detecção de colisão
    long long id; // Identificador único

    Obstacle(float x, float y, float w, float h, sf::Color col = sf::Color(128, 128, 128));

    void draw(sf::RenderTarget& target) const;
    std::vector<sf::Vector2f> getPolygon() const;

    long long getId() const;

private:
    void updatePolygon();
    static std::atomic<long long> nextId;

    // --- Added for Sprite ---
    sf::Texture texture;
    sf::Sprite sprite;
    bool textureLoaded = false;
    void loadTexture(const std::string& filename);
    // --- End Added for Sprite ---
};

#endif // OBSTACLE_HPP