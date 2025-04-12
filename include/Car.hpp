// In include/Car.hpp

#ifndef CAR_HPP
#define CAR_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "Controls.hpp"
#include "Network.hpp"
#include "Road.hpp"
#include "Sensor.hpp"

class Car {
public:
    // --- Membros Públicos Existentes ---
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

    // --- Novo Status e Constantes ---
    bool isFollowing = false;
    // ****** AJUSTADO PARA TESTE ******
    constexpr static int OSCILATION_START_THRESHOLD = 1;   // Nº de inversões para marcar como 'following' (Diminuído para 1)
    constexpr static int FOLLOWING_DAMAGE_THRESHOLD = 1; // Nº de frames seguindo para danificar (ex: 3 seg a 60fps)
    constexpr static float FOLLOWING_Y_PENALTY = 50.0f;   // Penalidade na posição Y para seleção do melhor carro
    constexpr static float DISTANCE_CHANGE_THRESHOLD = 0.1f; // Limiar para detectar mudança de distância

    // --- Construtor e Métodos Públicos ---
    Car(float x, float y, float w, float h, ControlType type = ControlType::AI, float maxSpd = 3.0f, sf::Color col = sf::Color::Blue);
    void loadTexture(const std::string& filename);
    void update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                std::vector<Car*>& traffic);
    void draw(sf::RenderTarget& target, bool drawSensor = false);
    std::vector<sf::Vector2f> getPolygon() const;
    NeuralNetwork* getBrain() { return brain ? &(*brain) : nullptr; }
    const NeuralNetwork* getBrain() const { return brain ? &(*brain) : nullptr; }

    // Função auxiliar pública para calcular a distância
    float calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut = nullptr);


private:
    // --- Métodos Privados ---
    void move();
    bool assessDamage(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                      const std::vector<Car*>& traffic);

    // --- Variáveis Privadas para Detecção ---
    float desiredAcceleration = 0.0f;
    float lastAppliedAcceleration = 0.0f;
    int lastTurnDirection = 0;
    float previousDistanceAhead = -1.0f;
    int distanceChangeDirection = 0;
    int lastDistanceChangeDirection = 0;
    int directionFlipCounter = 0;
    int followingDuration = 0; // Contador de frames no estado 'following'
};

#endif // CAR_HPP