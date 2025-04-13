// In include/Sensor.hpp

#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <vector>
#include <optional>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Color.hpp>
#include "Utils.hpp"    // For IntersectionData
#include "Road.hpp"     // Para definição completa de Road
#include "Obstacle.hpp" // <-- Inclui a definição de Obstacle

// Forward declaration
class Car;
// class Obstacle; // Não precisa se incluirmos Obstacle.hpp

class Sensor {
public:
    const Car& car; // Reference to the car it's attached to
    int rayCount = 5; // Reduzido de 15 para 5 como no código original
    float rayLength = 150.0f; // Aumentado de 75 para 150 como no código original
    float raySpread = M_PI / 2.0f; // Approx 90 degrees

    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> rays;
    std::vector<std::optional<IntersectionData>> readings;

    Sensor(const Car& attachedCar);
    int getRayCount() { return rayCount;}
    // Atualizado para receber Obstacles
    void update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                const std::vector<Obstacle*>& obstacles); // <-- MUDOU AQUI

    void draw(sf::RenderTarget& target);


private:
    void castRays();

    // Atualizado para receber Obstacles
    std::optional<IntersectionData> getReading(
        const std::pair<sf::Vector2f, sf::Vector2f>& ray,
        const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
        const std::vector<Obstacle*>& obstacles); // <-- MUDOU AQUI
};

#endif // SENSOR_HPP