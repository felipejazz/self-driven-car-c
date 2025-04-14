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
#include <cstdint>       // <-- Incluído para uint8_t
#include <cmath>         // Necessário para M_PI e outras funções matemáticas

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
    float stuckCheckTimer = 0.0f;
    float stuckCheckStartY = 0.0f;

    // --- Variáveis de Estado Anteriores ---
    float previousYPosition = 0.0f;
    float previousAngle = 0.0f; // Para detectar giro
    int previousLaneIndex = -1; // Para detectar mudança de faixa

    // --- Membros de Lógica e Fitness ---
    float currentFitness = 0.0f;
    std::unordered_set<long long> passedObstacleIDs; // IDs dos obstáculos ultrapassados

    // --- Métodos Auxiliares Privados ---
    void loadTexture(const std::string& filename);
    void move(float aiBrakeSignal, sf::Time deltaTime);
    void checkStoppedStatus(sf::Time deltaTime);
    void checkReversingStatus(sf::Time deltaTime);
    void checkStuckStatus(sf::Time deltaTime);
    void updateOvertakeStatus(const std::vector<Obstacle*>& obstacles);
    bool checkForCollision(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                             const std::vector<Obstacle*>& obstacles,
                             Obstacle*& hitObstacle);
    float calculateDesiredAcceleration(std::vector<float> outputs);
    int getCurrentLaneIndex(const Road& road) const;

    // --- Constantes de Fitness ---
    static constexpr float FITNESS_SPINNING_PENALTY = 1.0f;       // Penalidade por girar no lugar
    static constexpr float FITNESS_LANE_CHANGE_BONUS = 25.0f;      // Bônus por mudar de faixa (ajuste)
    static constexpr float FITNESS_SPEED_REWARD_THRESHOLD = 1.5f;
    static constexpr float FITNESS_FRAME_SURVIVAL_REWARD = 0.05f;
    static constexpr float FITNESS_SPEED_REWARD = 0.1f;
    static constexpr float FITNESS_OVERTAKE_BONUS = 50.0f;
    static constexpr float FITNESS_STOPPED_PENALTY = 50.0f;
    static constexpr float FITNESS_REVERSING_PENALTY = 50.0f;
    static constexpr float FITNESS_STUCK_PENALTY = 60.0f;
    static constexpr float FITNESS_FAIL_OVERTAKE_PENALTY = 75.0f;
    static constexpr float FITNESS_COLLISION_PENALTY = 100.0f;

    // --- Constantes de Comportamento ---
    // Limites para detecção de giro
    static constexpr float SPINNING_ANGLE_THRESHOLD_RAD_PER_SEC = M_PI; // 180 graus/seg
    static constexpr float SPINNING_MIN_Y_MOVEMENT_PER_SEC = 2.0f; // Menos de 2 unidades/seg para frente
    // Outros limites
    static constexpr float STOPPED_SPEED_THRESHOLD = 0.05f;
    static constexpr float STOPPED_TIME_THRESHOLD_SECONDS = 5.0f;
    static constexpr float REVERSE_TIME_THRESHOLD_SECONDS = 2.0f;
    static constexpr float STUCK_DISTANCE_Y_THRESHOLD = 5.0f;
    static constexpr float STUCK_TIME_THRESHOLD_SECONDS = 1.5f;
};

#endif // CAR_HPP