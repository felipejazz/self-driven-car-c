// In include/Car.hpp

#ifndef CAR_HPP
#define CAR_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <cmath>

// --- Project Headers ---
// Forward declarations first if needed, then includes
class Sensor;
class NeuralNetwork;
class Road;
#include "Controls.hpp" // Include full definition as Car HAS a Controls object
#include "Network.hpp" 
#include "Utils.hpp" // For lerp, getValueColor
#include "Visualizer.hpp" // For Visualizer 
class Car {
public:
    // --- Public Members ---
    sf::Vector2f position;
    float width;
    float height;
    float speed = 0.0f;
    float acceleration = 0.2f;
    float maxSpeed;
    float friction = 0.05f;
    float angle = 0.0f;
    bool damaged = false;
    ControlType controlType;
    bool useBrain = false;

    Controls controls;
    std::unique_ptr<Sensor> sensor;
    std::optional<NeuralNetwork> brain;

    // Graphics
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Color color;
    bool textureLoaded = false;

    // --- Constantes para Regras de Dano ---
    constexpr static float AGGRESSIVE_DELTA_THRESHOLD = 3.0f;
    constexpr static int AGGRESSIVE_DELTA_DURATION_THRESHOLD = 120; // 2 sec @ 60fps
    constexpr static float STOPPED_SPEED_THRESHOLD = 0.1f;
    constexpr static int STOPPED_DURATION_THRESHOLD = 120; // 2 sec @ 60fps
    constexpr static int REVERSE_DURATION_THRESHOLD = 180; // 3 sec @ 60fps

    // --- Construtor & Public Methods ---
    Car(float x, float y, float w, float h, ControlType type = ControlType::AI, float maxSpd = 3.0f, sf::Color col = sf::Color::Blue);

    void loadTexture(const std::string& filename);
    void update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                std::vector<Car*>& traffic);
    void draw(sf::RenderTarget& target, bool drawSensor = false);
    std::vector<sf::Vector2f> getPolygon() const;

    // Accessors
    NeuralNetwork* getBrain() { return brain ? &(*brain) : nullptr; }
    const NeuralNetwork* getBrain() const { return brain ? &(*brain) : nullptr; }
    float calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut = nullptr);
    float getPreviousDistanceAhead() const { return previousDistanceAhead; }
    int getAggressiveDeltaFrames() const { return aggressiveDeltaFrames; }
    // --- NOVO GETTER PARA O SCORE ---
    float getNetDistanceTraveled() const { return netDistanceTraveled; }

private:
    // --- Private Methods ---
    void move();
    bool assessDamage(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                      const std::vector<Car*>& traffic);

    // --- Private State Variables ---
    float desiredAcceleration = 0.0f;
    float lastAppliedAcceleration = 0.0f;
    int lastTurnDirection = 0;
    // Tracking para regras de dano
    float previousDistanceAhead = -1.0f;
    int aggressiveDeltaFrames = 0;
    int stoppedFrames = 0;
    int reversingFrames = 0;
    // --- NOVAS VARIÁVEIS PARA SCORE ---
    float netDistanceTraveled = 0.0f; // Score acumulado
    float previousYPosition;         // Posição Y no frame anterior
};

// Includes que dependem da definição completa de Car
#include "Sensor.hpp"
#include "Network.hpp"

#endif // CAR_HPP