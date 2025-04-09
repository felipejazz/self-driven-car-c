#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <utility> // std::pair

class Car;  // forward declaration

class Sensor {
private:
    Car* m_car;
    int  m_rayCount;    // Número de raios
    float m_rayLength;  // Comprimento dos raios
    float m_raySpread;  // Abertura total dos raios (em radianos)

    // Cada raio é um par (start, end)
    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> m_rays;
    // "Leituras" – ponto de interseção mais próximo para cada raio (ou (-1,-1) se não houver)
    std::vector<sf::Vector2f> m_readings;

    // Métodos auxiliares
    void castRays();
    std::optional<sf::Vector2f> getReading(
        const std::pair<sf::Vector2f, sf::Vector2f>& ray,
        const std::vector<std::vector<sf::Vector2f>>& roadBorders
    );

    // Desenha uma "linha" (simulada com um retângulo fino rotacionado)
    void drawLine(sf::RenderTarget& target,
                  const sf::Vector2f& start,
                  const sf::Vector2f& end,
                  sf::Color color,
                  float thickness) const;

public:
    Sensor(Car* car);
    ~Sensor() = default;

    // Atualiza as leituras com base nas bordas da estrada
    // No sensor.hpp
    void update(
        const std::vector<std::vector<sf::Vector2f>>& roadBorders,
        const std::vector<Car*>& otherCars = std::vector<Car*>{}
    );

    void updateSensor(
        const std::vector<std::vector<sf::Vector2f>>& roadBorders,
        const std::vector<Car*>& otherCars
    );
    std::vector<sf::Vector2f>& getReadings() { return m_readings;}
    float getRayCount() { return m_rayCount; }
    float getRayLength() { return m_rayLength; }
    const std::vector<sf::Vector2f>& getPolygon() const;
    // Desenha o sensor (raios) no target
    void draw(sf::RenderTarget& target) const;
};
