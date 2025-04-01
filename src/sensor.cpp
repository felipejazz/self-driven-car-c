#include "sensor.hpp"
#include "car.hpp"
#include "util.hpp"
#include <cmath>
#include <algorithm>
#include <optional>
#include <iostream>

Sensor::Sensor(Car* car)
    : m_car(car),
      m_rayCount(5),
      m_rayLength(150.f),
      m_raySpread((float)M_PI / 2.f)
{
}

void Sensor::update(
    const std::vector<std::vector<sf::Vector2f>>& roadBorders,
    const std::vector<Car*>& otherCars
){
    castRays();
    m_readings.clear();
    m_readings.reserve(m_rays.size());

    std::vector<std::vector<sf::Vector2f>> allEdges;

    for(const auto& border : roadBorders){
        allEdges.push_back(border);
    }

    for(const auto& carPtr : otherCars){
        if(carPtr == m_car) continue;

        const auto& poly = carPtr->getPolygon();
        for(size_t i=0; i<poly.size(); i++){
            sf::Vector2f p1 = poly[i];
            sf::Vector2f p2 = poly[(i+1) % poly.size()];
            allEdges.push_back({p1, p2});
        }
    }
    

    for (const auto& ray : m_rays) {
        auto reading = getReading(ray, allEdges);
        m_readings.push_back(reading.value_or(sf::Vector2f(-1, -1)));
    }
}

std::optional<sf::Vector2f> Sensor::getReading(
    const std::pair<sf::Vector2f, sf::Vector2f>& ray,
    const std::vector<std::vector<sf::Vector2f>>& roadBorders
) {
    std::vector<std::pair<sf::Vector2f, float>> touches;

    for (const auto& border : roadBorders) {
        sf::Vector2f intersect;
        float offset;
        if (getIntersection(ray.first, ray.second, border[0], border[1], intersect, offset)) {
            touches.emplace_back(intersect, offset);
        }
    }

    if (touches.empty()) {
        return std::nullopt;
    } else {

        auto minIt = std::min_element(touches.begin(), touches.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });
        return minIt->first;
    }
}
void Sensor::castRays() {
    m_rays.clear();
    m_rays.reserve(m_rayCount);

    for (int i = 0; i < m_rayCount; i++) {
        float t = (m_rayCount == 1) ? 0.5f : (float)i / (m_rayCount - 1);
        float rayAngle = lerp(m_raySpread / 2.f, -m_raySpread / 2.f, t);
        rayAngle += m_car->m_angle;

        sf::Vector2f start(m_car->m_x, m_car->m_y);
        sf::Vector2f end(
            m_car->m_x - std::sin(rayAngle) * m_rayLength,
            m_car->m_y - std::cos(rayAngle) * m_rayLength
        );
        m_rays.push_back({ start, end });
    }
}

void Sensor::draw(sf::RenderTarget& target) const {

    for (size_t i = 0; i < m_rays.size(); i++) {
        sf::Vector2f start = m_rays[i].first;
        sf::Vector2f end = m_rays[i].second;

        sf::Vector2f mid = end;
        if (i < m_readings.size() && m_readings[i].x >= 0.f) {
            mid = m_readings[i];
        }
        drawLine(target, start, mid, sf::Color::Yellow, 2.f);
        drawLine(target, mid, end, sf::Color::Black, 2.f);
    }
}

void Sensor::drawLine(sf::RenderTarget& target,
                      const sf::Vector2f& start,
                      const sf::Vector2f& end,
                      sf::Color color,
                      float thickness) const
{
    sf::Vector2f diff = end - start;
    float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    if (length < 0.001f) return;

    float angle = std::atan2(diff.y, diff.x) * 180.f / (float)M_PI;

    sf::RectangleShape line(sf::Vector2f(length, thickness));
    line.setFillColor(color);
    line.setPosition(start);
    line.setRotation(sf::degrees(angle));

    target.draw(line);
}
