#include "Sensor.hpp"
#include "Car.hpp"
#include "Obstacle.hpp"
#include "Utils.hpp"
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <algorithm>
#include <limits>
#include <cmath>

Sensor::Sensor(const Car& attachedCar) : car(attachedCar) {
    rays.resize(rayCount);
    readings.resize(rayCount);
}

void Sensor::update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                    const std::vector<Obstacle*>& obstacles) {
    castRays();
    readings.clear();
    readings.resize(rayCount);

    for (int i = 0; i < rayCount; ++i) {
        readings[i] = getReading(rays[i], roadBorders, obstacles);
    }
}

void Sensor::castRays() {
    rays.clear();
    rays.resize(rayCount);

    for (int i = 0; i < rayCount; ++i) {
        float angleRatio = (rayCount == 1) ? 0.5f : static_cast<float>(i) / (rayCount - 1);
        float relativeRayAngle = lerp(raySpread / 2.0f, -raySpread / 2.0f, angleRatio);
        float finalRayAngle = relativeRayAngle + car.angle;
        sf::Vector2f start = car.position;
        sf::Vector2f end = {
            start.x + std::sin(finalRayAngle) * rayLength,
            start.y - std::cos(finalRayAngle) * rayLength
        };
        rays[i] = {start, end};
    }
}

std::optional<IntersectionData> Sensor::getReading(
    const std::pair<sf::Vector2f, sf::Vector2f>& ray,
    const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
    const std::vector<Obstacle*>& obstacles)
{
    std::vector<IntersectionData> touches;
    for (const auto& border : roadBorders) {
        auto touch = getIntersection(ray.first, ray.second, border.first, border.second);
        if (touch) {
            touches.push_back(*touch);
        }
    }
    for (const Obstacle* obsPtr : obstacles) {
        if (!obsPtr) continue;
        const std::vector<sf::Vector2f>& poly = obsPtr->getPolygon();
        if (poly.size() < 2) continue;
        for (size_t j = 0; j < poly.size(); ++j) {
            auto touch = getIntersection(ray.first, ray.second, poly[j], poly[(j + 1) % poly.size()]);
            if (touch) {
                touches.push_back(*touch);
            }
        }
    }
    if (touches.empty()) {
        return std::nullopt;
    } else {
        auto minIt = std::min_element(touches.begin(), touches.end(),
                                      [](const IntersectionData& a, const IntersectionData& b) {
                                          return a.offset < b.offset;
                                      });
        return *minIt;
    }
}

void Sensor::draw(sf::RenderTarget& target) {
    sf::VertexArray lines(sf::PrimitiveType::Lines);
    for (int i = 0; i < rayCount; ++i) {
        sf::Vector2f rayEnd = rays[i].second;
        if (readings[i]) {
            rayEnd = readings[i]->point;
            lines.append(sf::Vertex{rays[i].second, sf::Color::Black});
            lines.append(sf::Vertex{rayEnd, sf::Color::Black});
        }
        lines.append(sf::Vertex{rays[i].first, sf::Color::Yellow});
        lines.append(sf::Vertex{rayEnd, sf::Color::Yellow});
    }
    if (lines.getVertexCount() > 0) {
        target.draw(lines);
    }
}