#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "car.hpp"


class Road {
private:
    float m_x;
    float m_width;
    int   m_laneCount;

    float m_left;
    float m_right;
    float m_top;
    float m_bottom;

    float m_friction;

    std::vector<std::vector<sf::Vector2f>> m_borders;

public:
    Road(float x, float width, int laneCount=3, float friction=0.05f);
    ~Road() = default;

    float getLaneCenter(int laneIndex);

    const std::vector<std::vector<sf::Vector2f>>& getBorders() const;
    float getFriction() const { return m_friction; }
    void  setFriction(float f) { m_friction = f; }
    void render(sf::RenderWindow& window);
    int  getLaneFromPosition(float xPos) const;
    sf::Vector2f getSpawnPosition() const;         
    bool isWithinRoad(const Car& car) const;       
    void update(float deltaTime, float playerSpeed);
};
