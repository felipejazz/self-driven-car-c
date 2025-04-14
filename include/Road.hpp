#ifndef ROAD_HPP
#define ROAD_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>
#include "Utils.hpp"

class Road {
public:
    float x;
    float width;
    int laneCount;
    float top;
    float bottom;
    float left;
    float right;
    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> borders;

    Road(float centerX, float roadWidth, int lanes = 3);
    float getLaneCenter(int laneIndex) const;
    void draw(sf::RenderTarget& target);

private:
    void setupBorders();
};

#endif