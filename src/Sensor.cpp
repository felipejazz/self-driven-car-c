#include "Sensor.hpp"
#include "Car.hpp" // Include Car header here to avoid circular dependency issues with forward declaration
#include "Utils.hpp"
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp> // Include for VertexArray
#include <SFML/Graphics/RenderTarget.hpp> // Include for RenderTarget
#include <SFML/Graphics/PrimitiveType.hpp> // Include for PrimitiveType
#include <algorithm> // for std::min_element
#include <limits> // for numeric_limits
#include <cmath> // for std::sin, std::cos

Sensor::Sensor(const Car& attachedCar) : car(attachedCar) {
    // Initialize rays and readings vectors
    rays.resize(rayCount);
    readings.resize(rayCount);
}

void Sensor::update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                    const std::vector<Car*>& traffic) { // Change traffic param to non-const pointer vector
    castRays();
    readings.clear(); // Clear previous readings
    readings.resize(rayCount); // Resize back, filled with nullopt

    for (int i = 0; i < rayCount; ++i) {
        // Pass the non-const traffic vector to getReading
        readings[i] = getReading(rays[i], roadBorders, traffic);
    }
}

void Sensor::castRays() {
     rays.clear(); // Clear old rays
     rays.resize(rayCount); // Ensure correct size

    for (int i = 0; i < rayCount; ++i) {
        // Ensure raySpread is float for lerp if needed, or cast inside
        float spread = static_cast<float>(raySpread);
        float angleRatio = (rayCount == 1) ? 0.5f : static_cast<float>(i) / (rayCount - 1);

        float rayAngle = lerp(
            spread / 2.0f,
            -spread / 2.0f,
            angleRatio
        ) + car.angle; // Add car's current angle (assuming car.angle is float)

        sf::Vector2f start = car.position; // Assuming car.position is sf::Vector2f
        sf::Vector2f end = {
            start.x - std::sin(rayAngle) * rayLength, // rayLength should be float
            start.y - std::cos(rayAngle) * rayLength
        };
         rays[i] = {start, end};
    }
}


// Change traffic param to non-const pointer vector if getPolygon isn't const
std::optional<IntersectionData> Sensor::getReading(
    const std::pair<sf::Vector2f, sf::Vector2f>& ray,
    const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
    const std::vector<Car*>& traffic) // Keep const if getPolygon is const
{
    std::vector<IntersectionData> touches;

    // Check intersection with road borders
    for (const auto& border : roadBorders) {
        // Ensure getIntersection is compatible with sf::Vector2f
        auto touch = getIntersection(ray.first, ray.second, border.first, border.second);
        if (touch) {
            touches.push_back(*touch);
        }
    }

    // Check intersection with traffic cars
    for (Car* trafficCar : traffic) { // Use non-const Car* if getPolygon isn't const
         if (!trafficCar) continue; // Safety check

        // Call getPolygon() - ensure it returns std::vector<sf::Vector2f>
        // If getPolygon is not const, trafficCar must be Car* not const Car*
        const std::vector<sf::Vector2f>& poly = trafficCar->getPolygon(); // Assuming getPolygon is const

        if (poly.size() < 2) continue; // Need at least 2 points for segments

        for (size_t j = 0; j < poly.size(); ++j) {
            auto touch = getIntersection(
                ray.first,
                ray.second,
                poly[j],
                poly[(j + 1) % poly.size()] // Wrap around for the last segment
            );
            if (touch) {
                touches.push_back(*touch);
            }
        }
    }

    if (touches.empty()) {
        return std::nullopt;
    } else {
        // Find the touch with the minimum offset (closest intersection)
        auto minIt = std::min_element(touches.begin(), touches.end(),
            [](const IntersectionData& a, const IntersectionData& b) {
                return a.offset < b.offset;
            });
        return *minIt; // Return the closest intersection data
    }
}


void Sensor::draw(sf::RenderTarget& target) {
    sf::VertexArray lines(sf::PrimitiveType::Lines); // Correct primitive type
    for (int i = 0; i < rayCount; ++i) {
        sf::Vector2f rayEnd = rays[i].second; // Default end of the ray cast

        if (readings[i]) {
            // If there's an intersection reading, the visual end is the intersection point
            rayEnd = readings[i]->point;

             // Draw the segment from the intersection point to the original ray end (black)
             // ***********************************************************************
             // * FIX: Use brace initialization {} for sf::Vertex
             // ***********************************************************************
            lines.append(sf::Vertex{rays[i].second, sf::Color::Black}); // Corrected
            lines.append(sf::Vertex{rayEnd, sf::Color::Black});         // Corrected
             // ***********************************************************************
        }

        // Draw the segment from the car to the intersection point (or ray end if no intersection) (yellow)
        // ***********************************************************************
        // * FIX: Use brace initialization {} for sf::Vertex
        // ***********************************************************************
        lines.append(sf::Vertex{rays[i].first, sf::Color::Yellow});     // Corrected
        lines.append(sf::Vertex{rayEnd, sf::Color::Yellow});            // Corrected
        // ***********************************************************************

    }
     if (lines.getVertexCount() > 0) {
        target.draw(lines);
     }
}