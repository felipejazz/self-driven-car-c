// In include/Sensor.hpp

#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <vector>
#include <optional>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Color.hpp>
#include <cmath>
#include "Utils.hpp"
#include "Road.hpp"
#include "Obstacle.hpp"

class Car;

class Sensor {
public:
    const Car& car;
    int rayCount = 5;
    float rayLength = 150.0f;
    float raySpread = M_PI / 2.0f;
    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> rays;
    std::vector<std::optional<IntersectionData>> readings;

    Sensor(const Car& attachedCar);
    int getRayCount() { return rayCount; }
    void update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                const std::vector<Obstacle*>& obstacles);
    void draw(sf::RenderTarget& target);

private:
    void castRays();
    std::optional<IntersectionData> getReading(
        const std::pair<sf::Vector2f, sf::Vector2f>& ray,
        const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
        const std::vector<Obstacle*>& obstacles);
};

#endif