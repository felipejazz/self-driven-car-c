#ifndef ROAD_HPP
#define ROAD_HPP

#include <SFML/Graphics.hpp> // <<--- ESTE É O PONTO CHAVE
#include <vector>
#include <limits> // Required for infinity

class Road {
public:
    float x; // Center x-coordinate
    float width;
    int laneCount;

    float left;
    float right;
    float top;
    float bottom;

    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> borders; // Pairs of points defining border lines

    Road(float centerX, float roadWidth, int lanes = 3);

    // Get the center x-coordinate of a specific lane index
    float getLaneCenter(int laneIndex) const;

    void draw(sf::RenderTarget& target);
};

#endif // ROAD_HPP