#include "Sensor.hpp"
#include "Car.hpp" // Include Car header here
#include "Obstacle.hpp" // Include Obstacle header here
#include "Utils.hpp"
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <algorithm> // for std::min_element
#include <limits> // for numeric_limits
#include <cmath> // for std::sin, std::cos

Sensor::Sensor(const Car& attachedCar) : car(attachedCar) {
    rays.resize(rayCount);
    readings.resize(rayCount);
}

// Atualizado para receber Obstacles
void Sensor::update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                    const std::vector<Obstacle*>& obstacles) { // <-- MUDOU AQUI
    castRays();
    readings.clear();
    readings.resize(rayCount);

    for (int i = 0; i < rayCount; ++i) {
        readings[i] = getReading(rays[i], roadBorders, obstacles); // <-- Passa obstacles
    }
}

// (castRays sem mudanças)
// Em Sensor::castRays() no arquivo self-driven/src/Sensor.cpp
// Em Sensor::castRays() no arquivo self-driven/src/Sensor.cpp
void Sensor::castRays() {
    rays.clear();
    rays.resize(rayCount);

   for (int i = 0; i < rayCount; ++i) {
       float angleRatio = (rayCount == 1) ? 0.5f : static_cast<float>(i) / (rayCount - 1);

       // Ângulo relativo do raio (sem mudança aqui)
       float relativeRayAngle = lerp(
           raySpread / 2.0f,
           -raySpread / 2.0f,
           angleRatio
       );

       // Combina com o ângulo do carro (sem mudança aqui, ASSUMINDO car.angle é padrão)
       float finalRayAngle = relativeRayAngle + car.angle;

       // Ponto inicial
       sf::Vector2f start = car.position;

       sf::Vector2f end = {
           start.x + std::sin(finalRayAngle) * rayLength, 
           start.y - std::cos(finalRayAngle) * rayLength  
       };


       rays[i] = {start, end};
   }
}

// Atualizado para receber Obstacles
std::optional<IntersectionData> Sensor::getReading(
    const std::pair<sf::Vector2f, sf::Vector2f>& ray,
    const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
    const std::vector<Obstacle*>& obstacles) // <-- MUDOU AQUI
{
    std::vector<IntersectionData> touches;

    // Interseção com bordas da estrada
    for (const auto& border : roadBorders) {
        auto touch = getIntersection(ray.first, ray.second, border.first, border.second);
        if (touch) {
            touches.push_back(*touch);
        }
    }

    // Interseção com obstáculos
    for (const Obstacle* obsPtr : obstacles) { // <-- MUDOU AQUI
         if (!obsPtr) continue;

        // Obtém polígono do obstáculo
        const std::vector<sf::Vector2f>& poly = obsPtr->getPolygon(); // <-- MUDOU AQUI

        if (poly.size() < 2) continue; // Precisa de pelo menos 2 pontos

        for (size_t j = 0; j < poly.size(); ++j) {
            auto touch = getIntersection(
                ray.first,
                ray.second,
                poly[j],
                poly[(j + 1) % poly.size()] // Fecha o polígono
            );
            if (touch) {
                touches.push_back(*touch);
            }
        }
    }

    if (touches.empty()) {
        return std::nullopt; // Nenhuma interseção
    } else {
        // Encontra a interseção mais próxima (menor offset)
        auto minIt = std::min_element(touches.begin(), touches.end(),
            [](const IntersectionData& a, const IntersectionData& b) {
                return a.offset < b.offset;
            });
        return *minIt; // Retorna a interseção mais próxima
    }
}

// (draw sem mudanças)
void Sensor::draw(sf::RenderTarget& target) {
    sf::VertexArray lines(sf::PrimitiveType::Lines);
    for (int i = 0; i < rayCount; ++i) {
        sf::Vector2f rayEnd = rays[i].second;

        if (readings[i]) {
            rayEnd = readings[i]->point; // Ponto final visual é a interseção

            // Linha da interseção até o fim original (preto)
            lines.append(sf::Vertex{rays[i].second, sf::Color::Black});
            lines.append(sf::Vertex{rayEnd, sf::Color::Black});
        }

        // Linha do carro até a interseção (ou fim original) (amarelo)
        lines.append(sf::Vertex{rays[i].first, sf::Color::Yellow});
        lines.append(sf::Vertex{rayEnd, sf::Color::Yellow});
    }
     if (lines.getVertexCount() > 0) {
        target.draw(lines);
     }
}