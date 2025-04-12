// Car.hpp (REVISADO)
#ifndef CAR_HPP
#define CAR_HPP

#include "Controls.hpp"
#include "Sensor.hpp"
#include "Network.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <memory>
#include <vector> // Inclua para std::vector

class Car {
public: // <-- MOVIDO PARA PÚBLICO OU ADICIONADO GETTERS
    // Membros movidos para public para acesso direto onde necessário (ex: Sensor, main)
    sf::Vector2f position;
    float width;
    float height;
    float angle = 0.0f;
    std::optional<NeuralNetwork> brain; // <<< MOVIDO PARA PUBLIC
    bool useBrain = false;             // <<< MOVIDO PARA PUBLIC

    // --- Métodos Públicos ---
    Car(float x, float y, float w, float h, ControlType type = ControlType::AI, float maxSpd = 3.0f, sf::Color col = sf::Color::Blue);
    void update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders, std::vector<Car*>& traffic);
    void draw(sf::RenderTarget& target, bool drawSensorFlag = false);
    std::vector<sf::Vector2f> getPolygon() const;
    bool assessDamage(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders, const std::vector<Car*>& traffic);

    // Getters para estado
    bool isDamaged() const { return damaged; }
    float getScore() const { return netDistanceTraveled; }
    float getSpeed() const { return speed; } // Getter para velocidade

    // Getters para lógica interna (usados em main.cpp para logging/lógica de geração)
    int getSensorRayCount() const;             // <-- NOVO GETTER
    float getPreviousDistanceAhead() const;   // <-- NOVO GETTER
    int getAggressiveDeltaFrames() const;     // <-- NOVO GETTER

    // Métodos de Ação
    void resetDamage();                         // <-- NOVO MÉTODO
    void resetForTraffic(const sf::Vector2f& startPos, float startSpeed); // <-- NOVO MÉTODO

    // Métodos que main estava chamando e eram privados
    float calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut = nullptr); // <-- MOVIDO PARA PUBLIC


private:
    // Estado e Propriedades (Privados)
    float speed = 0.0f;
    float acceleration = 0.2f;
    float brakePower = 0.4f;
    float maxSpeed;
    float friction = 0.05f;
    bool damaged = false; // Mantido privado, usar isDamaged() e resetDamage()

    // Controle (Privado)
    ControlType controlType;
    Controls controls;

    // Sensor (Privado - use getSensorRayCount)
    std::unique_ptr<Sensor> sensor;

    // Gráficos (Privado)
    sf::Color color;
    sf::Texture texture;
    sf::Sprite sprite;
    bool textureLoaded;
    void loadTexture(const std::string& filename);

    // Lógica Interna Update (Privado)
    int lastTurnDirection;
    float desiredAcceleration;
    float lastAppliedAcceleration;
    float previousDistanceAhead; // Acessado via getPreviousDistanceAhead()
    int aggressiveDeltaFrames;   // Acessado via getAggressiveDeltaFrames()
    int stoppedFrames;
    int reversingFrames;
    float netDistanceTraveled; // Acessado via getScore()
    float previousYPosition;

    // Métodos Privados
    void move(float aiBrakeSignal);


    // Constantes internas (exemplo)
    const float AGGRESSIVE_DELTA_THRESHOLD = 0.5f;
    const int AGGRESSIVE_DELTA_DURATION_THRESHOLD = 15; // frames
    const float STOPPED_SPEED_THRESHOLD = 0.1f;
    const int STOPPED_DURATION_THRESHOLD = 50; // frames
    const int REVERSE_DURATION_THRESHOLD = 30; // frames
};

#endif // CAR_HPP