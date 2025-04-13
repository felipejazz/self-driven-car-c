// include/Car.hpp (COMPLETO E ATUALIZADO com timer para 'reversing')
#ifndef CAR_HPP
#define CAR_HPP

#include "Controls.hpp"    // Certifique-se que este define ControlType
#include "Sensor.hpp"      // Precisa da definição de Sensor
#include "Network.hpp"     // Precisa da definição de NeuralNetwork
#include <SFML/Graphics.hpp>
#include <optional>
#include <memory>
#include <vector>

class Car {
public:
    // Membros públicos
    sf::Vector2f position;
    float width;
    float height;
    float angle = 0.0f; // Assume radianos
    std::optional<NeuralNetwork> brain;
    bool useBrain = false;

    // --- Métodos Públicos ---
    Car(float x, float y, float w, float h, ControlType type = ControlType::AI, float maxSpd = 3.0f, sf::Color col = sf::Color::Blue);
    void update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                std::vector<Car*>& traffic,
                sf::Time deltaTime);
    void draw(sf::RenderTarget& target, bool drawSensorFlag = false);
    std::vector<sf::Vector2f> getPolygon() const;
    bool checkForCollision(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders, const std::vector<Car*>& traffic);

    // Getters para estado
    bool isDamaged() const { return damaged; }
    float getScore() const { return netDistanceTraveled; }
    float getSpeed() const { return speed; }

    // Getters para lógica interna (logging)
    int getSensorRayCount() const;
    float getPreviousDistanceAhead() const;
    int getAggressiveDeltaFrames() const;

    // Métodos de Ação
    void resetDamage();
    void resetForTraffic(const sf::Vector2f& startPos, float startSpeed);

    // Cálculo de distância
    float calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut = nullptr);

private:
    // Estado e Propriedades Privadas
    float speed = 0.0f;
    float acceleration = 0.2f;
    float brakePower = 0.4f;
    float maxSpeed;
    float friction = 0.05f;
    bool damaged = false;

    // Controle Privado
    ControlType controlType;
    Controls controls;

    // Sensor e Cérebro Privados
    std::unique_ptr<Sensor> sensor;

    // Gráficos Privados
    sf::Color color;
    sf::Texture texture;
    sf::Sprite sprite;
    bool textureLoaded;
    void loadTexture(const std::string& filename);

    // Lógica Interna do Update (Privado)
    int lastTurnDirection = 0;
    float desiredAcceleration = 0.0f; // Usado pela IA para intenção de ré
    float lastAppliedAcceleration = 0.0f;
    float previousDistanceAhead = -1.0f;
    int aggressiveDeltaFrames = 0;
    float aggressiveFollowingTimer = 0.0f;
    float stoppedTimer = 0.0f;
    // int reversingFrames = 0; // *** REMOVIDO contador de frames ***
    float reversingTimer = 0.0f; // *** NOVO: Timer para marcha à ré ***
    float netDistanceTraveled = 0.0f;
    float previousYPosition = 0.0f;

    // Métodos Auxiliares Privados
    void move(float aiBrakeSignal, sf::Time deltaTime);
    void checkAggressiveFollowing(const std::vector<Car*>& traffic, sf::Time deltaTime);
    void checkStoppedStatus(sf::Time deltaTime);
    void checkReversingStatus(sf::Time deltaTime); // *** NOVO: Função para checar ré ***

    // Constantes de Comportamento Internas
    static constexpr float AGGRESSIVE_DISTANCE_THRESHOLD = 10.0f;
    static constexpr float AGGRESSIVE_DISTANCE_MAX = 60.0f;
    static constexpr float AGGRESSIVE_DELTA_THRESHOLD = -0.1f;
    static constexpr float AGGRESSIVE_FOLLOWING_DURATION_THRESHOLD = 2.0f;
    static constexpr float STOPPED_SPEED_THRESHOLD = 0.05f;
    static constexpr float STOPPED_TIME_THRESHOLD_SECONDS = 3.0f;
    static constexpr float REVERSE_TIME_THRESHOLD_SECONDS = 1.5f; // *** NOVO: Limite em segundos ***
};

#endif // CAR_HPP