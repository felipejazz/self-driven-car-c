// In include/Sensor.hpp

#ifndef SENSOR_HPP
#define SENSOR_HPP

#include <vector>
#include <optional>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Color.hpp>
#include "Utils.hpp" // For IntersectionData
#include "Road.hpp"  // <<< INCLUA Road.hpp AQUI para definição completa

// Forward declaration
class Car;
// class Road; // <<< REMOVA a declaração antecipada daqui

class Sensor {
public:
    const Car& car; // Reference to the car it's attached to
    int rayCount = 15;
    float rayLength = 75.0f;
    float raySpread = M_PI / 2.0f; // Approx 90 degrees

    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> rays;
    std::vector<std::optional<IntersectionData>> readings;

    Sensor(const Car& attachedCar);

    // O tipo do parâmetro roadBorders está correto, pois recebe os dados,
    // não precisa chamar métodos do objeto Road aqui.
    void update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                const std::vector<Car*>& traffic);

    void draw(sf::RenderTarget& target);

private:
    void castRays();
    std::optional<IntersectionData> getReading(
        const std::pair<sf::Vector2f, sf::Vector2f>& ray,
        const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
        const std::vector<Car*>& traffic);
};

#endif // SENSOR_HPP