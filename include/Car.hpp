// In include/Car.hpp

#ifndef CAR_HPP
#define CAR_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory> // For unique_ptr
#include <optional> // For optional brain

#include "Controls.hpp"
#include "Sensor.hpp"
#include "Network.hpp" // Include network header

class Road; // Forward declaration

class Car {
public:
    sf::Vector2f position;
    float width;
    float height;
    float speed = 0.0f;
    float acceleration = 0.2f;
    float maxSpeed;
    float friction = 0.05f;
    float angle = 0.0f; // Radians, 0 = pointing up (negative Y)
    bool damaged = false;
    ControlType controlType;
    bool useBrain = false; // True if controlType is AI
    bool textureLoaded = false;
    static constexpr float PENDULUM_DISTANCE_MIN = 40.0f; // Distância mínima para ativar o threshold
    static constexpr float PENDULUM_DISTANCE_MAX = 80.0f; 

    Controls controls;
    std::unique_ptr<Sensor> sensor; // Sensor is optional for DUMMY cars
    std::optional<NeuralNetwork> brain; // Brain is optional

    // Graphics
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Color color; // Original color, used if texture loads

    Car(float x, float y, float w, float h, ControlType type = ControlType::AI, float maxSpd = 3.0f, sf::Color col = sf::Color::Blue);

    void loadTexture(const std::string& filename);

    void update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                std::vector<Car*>& traffic); // Pass vector of pointers

    void draw(sf::RenderTarget& target, bool drawSensor = false);

    std::vector<sf::Vector2f> getPolygon() const;

    NeuralNetwork* getBrain() { return brain ? &(*brain) : nullptr; }
    const NeuralNetwork* getBrain() const { return brain ? &(*brain) : nullptr; }


private:
    void move();
    bool assessDamage(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                      const std::vector<Car*>& traffic); // Vector of pointers
    
    float calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut = nullptr);
    // --- Adicionado para detecção de pêndulo ---
    int pendulumCounter = 0;       // Contador de oscilações
    int lastTurnDirection = 0;     // -1 para esquerda, 1 para direita, 0 para nenhum/reto
    // --- Fim da adição ---
};

#endif // CAR_HPP