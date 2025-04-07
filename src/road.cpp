#include "road.hpp"
#include "util.hpp"     
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

Road::Road(float x, float width, int laneCount, float friction)
    : m_x(x),
      m_width(width),
      m_laneCount(laneCount),
      m_friction(friction)
{
    m_left  = x - width / 2.0f;
    m_right = x + width / 2.0f;

    float infinity = 1000000.0f;
    m_top    = -infinity;
    m_bottom =  infinity;
    m_borders = {
        { sf::Vector2f(m_left, m_top), sf::Vector2f(m_left, m_bottom) },
        { sf::Vector2f(m_right, m_top), sf::Vector2f(m_right, m_bottom) }
    };
}

float Road::getLaneCenter(int laneIndex) {
    float laneWidth = m_width / (float)m_laneCount;
    float clampedIndex = std::min((float)laneIndex, (float)m_laneCount - 1.0f);
    return m_left + laneWidth / 2.0f + clampedIndex * laneWidth;
}

void Road::render(sf::RenderWindow& window) {
    sf::RectangleShape roadSurface(sf::Vector2f(m_right - m_left, m_bottom - m_top));
    roadSurface.setFillColor(sf::Color::Black);
    roadSurface.setPosition(sf::Vector2f(m_left, m_top));
    window.draw(roadSurface);

    for (int i = 1; i < m_laneCount; i++) {
        float t = i / (float)m_laneCount;
        float x = lerp(m_left, m_right, t);
        for (float y = m_top; y < m_bottom; y += 40.0f) {
            sf::RectangleShape dash(sf::Vector2f(2.0f, 20.0f));
            dash.setFillColor(sf::Color::Yellow);
            dash.setPosition(sf::Vector2f(x, y));

            window.draw(dash);
        }
    }

    {
        float borderHeight = (m_bottom - m_top); 
        sf::RectangleShape leftBorder(sf::Vector2f(4.0f, borderHeight));
        leftBorder.setFillColor(sf::Color::White);
        leftBorder.setPosition(sf::Vector2f( m_left - 2.0f, m_top)); 
        window.draw(leftBorder);
    }

    {
        float borderHeight = (m_bottom - m_top);
        sf::RectangleShape rightBorder(sf::Vector2f(4.0f, borderHeight));
        rightBorder.setFillColor(sf::Color::White);
        rightBorder.setPosition(sf::Vector2f( m_right - 2.0f, m_top));
        window.draw(rightBorder);
    }
}

sf::Vector2f Road::getSpawnPosition() const {
    float spawnX = (m_left + m_right) * 0.5f;
    float spawnY = 300.f; 
    return sf::Vector2f(spawnX, spawnY);
}

const std::vector<std::vector<sf::Vector2f>>& Road::getBorders() const {
    return m_borders;
}

bool Road::isWithinRoad(const Car& car) const {
    const auto& carPoly = car.getPolygon();
    
    for (const auto& point : carPoly) {
        bool insideHoriz = (point.x >= m_left) && (point.x <= m_right);
        bool insideVert  = (point.y >= m_top) && (point.y <= m_bottom);
        
        if (!insideHoriz || !insideVert) {
            return false;
            std::cout << "Car is not within a road: " << std::endl;
        }
    }
    return true;
}

int Road::getLaneFromPosition(float x) const {
    float laneWidth = m_width / static_cast<float>(m_laneCount);
    int lane = static_cast<int>((x - m_left) / laneWidth);
    if (lane < 0) lane = 0;
    if (lane >= m_laneCount) lane = m_laneCount - 1;
    return lane;
}

void Road::update(float /*deltaTime*/, float /*playerSpeed*/) {

}
