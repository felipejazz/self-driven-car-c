// include/Car.hpp

#ifndef CAR_HPP
#define CAR_HPP

#include "Controls.hpp"
#include "Sensor.hpp"
#include "Network.hpp"
#include "Obstacle.hpp" // Inclui Obstacle.hpp para a definição completa
#include "Road.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <memory>
#include <vector>
#include <deque>
#include <string>
#include <unordered_set> // <-- Inclui para unordered_set
#include <cstdint> // <-- Incluído para uint8_t

class Car {
public:
    // --- Membros Públicos ---
    sf::Vector2f position;
    float width;
    float height;
    float angle = 0.0f;
    std::optional<NeuralNetwork> brain;
    bool useBrain = false;

    // --- Construtor ---
    Car(float x, float y, float w, float h, ControlType type = ControlType::AI, float maxSpd = 3.0f, sf::Color col = sf::Color::Blue);

    // --- Métodos Públicos ---
    void update(const Road& road, const std::vector<Obstacle*>& obstacles, sf::Time deltaTime);
    void draw(sf::RenderTarget& target, bool drawSensorFlag = false);
    std::vector<sf::Vector2f> getPolygon() const;
    bool isDamaged() const { return damaged; }
    float getFitness() const { return currentFitness; }
    float getSpeed() const { return speed; }
    int getSensorRayCount() const;
    void resetForNewGeneration(float startY, const Road& road);

private:
    // --- Membros Privados (Física/Controle) ---
     float speed = 0.0f;
     float acceleration = 0.2f;
     float brakePower;
     float maxSpeed;
     float friction = 0.05f;
     bool damaged = false;
     ControlType controlType;
     Controls controls;
     std::unique_ptr<Sensor> sensor;
     sf::Color color;
     sf::Texture texture;
     sf::Sprite sprite;
     bool textureLoaded;

    void updateBasedOnControls(Controls controls);

    // --- Membros Privados (Lógica de IA e Estado) ---
    float desiredAcceleration = 0.0f;
    float lastAppliedAcceleration = 0.0f;
    float stoppedTimer = 0.0f;
    float reversingTimer = 0.0f;
    float previousYPosition = 0.0f;
    float currentFitness = 0.0f;
    // --- NOVOS/REUTILIZADOS para checagem de "preso" ---
    float stuckCheckTimer = 0.0f;
    float stuckCheckStartY = 0.0f; // Posição Y no início da checagem de "preso"


    // --- NOVO: Rastreamento de Ultrapassagem ---
    std::unordered_set<long long> passedObstacleIDs; // IDs dos obstáculos ultrapassados

    // --- Métodos Auxiliares Privados ---
    void loadTexture(const std::string& filename);
    void move(float aiBrakeSignal, sf::Time deltaTime);
    void checkStoppedStatus(sf::Time deltaTime);
    void checkReversingStatus(sf::Time deltaTime);
    void checkStuckStatus(sf::Time deltaTime); // <-- NOVA FUNÇÃO
    // Adicionado para ultrapassagem
    void updateOvertakeStatus(const std::vector<Obstacle*>& obstacles);
    // Modificado para retornar qual obstáculo foi atingido
    bool checkForCollision(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                             const std::vector<Obstacle*>& obstacles,
                             Obstacle*& hitObstacle); // Parâmetro de saída

    float calculateDesiredAcceleration(std::vector<float> outputs);
    // --- Constantes de Comportamento Internas ---
    static constexpr float STOPPED_SPEED_THRESHOLD = 0.05f;
    static constexpr float STOPPED_TIME_THRESHOLD_SECONDS = 5.0f;
    static constexpr float REVERSE_TIME_THRESHOLD_SECONDS = 2.0f;
    // --- NOVAS constantes para checagem de "preso" ---
    static constexpr float STUCK_DISTANCE_Y_THRESHOLD = 5.0f; // Distância mínima em Y para não ser considerado preso
    static constexpr float STUCK_TIME_THRESHOLD_SECONDS = 1.5f; // Tempo para a checagem de "preso"
    // static constexpr float BLOCKED_SPEED_THRESHOLD = 0.5f; // Relacionado a penalidade de bloqueio (não implementado ainda)
    // static constexpr float BLOCKED_SENSOR_THRESHOLD = 0.8f; // Relacionado a penalidade de bloqueio (não implementado ainda)

    // --- Constantes de Fitness ---
    static constexpr float FITNESS_SPEED_REWARD_THRESHOLD = 1.5f;
    static constexpr float FITNESS_FRAME_SURVIVAL_REWARD = 0.05f;
    static constexpr float FITNESS_SPEED_REWARD = 0.1f;
    static constexpr float FITNESS_OVERTAKE_BONUS = 50.0f; // <-- RECOMPENSA POR ULTRAPASSAR
    static constexpr float FITNESS_STOPPED_PENALTY = 50.0f;
    static constexpr float FITNESS_REVERSING_PENALTY = 50.0f;
    static constexpr float FITNESS_STUCK_PENALTY = 60.0f; // <-- PENALIDADE POR FICAR PRESO
    static constexpr float FITNESS_FAIL_OVERTAKE_PENALTY = 75.0f; // <-- PENALIDADE POR COLIDIR ANTES DE PASSAR
    // static constexpr float FITNESS_BLOCKED_PENALTY_PER_FRAME = 0.15f; // Relacionado a bloqueio (não implementado)
    static constexpr float FITNESS_COLLISION_PENALTY = 100.0f; // Penalidade base por qualquer colisão
};

#endif // CAR_HPP