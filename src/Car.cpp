// Car.cpp
#include "Car.hpp"
#include "Utils.hpp" // Presume que Utils.hpp contém M_PI, lerp, getRandomSigned etc.
#include <cmath>
#include <iostream>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <limits>
#include <algorithm>

// --- Construtor ---
Car::Car(float x, float y, float w, float h, ControlType type, float maxSpd, sf::Color col)
    : position(x, y),
      width(w),
      height(h),
      maxSpeed(maxSpd),
      // acceleration: // usa valor padrão
      brakePower(acceleration * 2.0f), // <-- INICIALIZADO: Força de freio (ex: 2x aceleração)
      // friction: // usa valor padrão
      controlType(type),
      controls(type), // Inicializa controls.brake como false
      color(col),
      textureLoaded(false),
      sprite(texture),
      lastTurnDirection(0),
      desiredAcceleration(0.0f),
      lastAppliedAcceleration(0.0f),
      previousDistanceAhead(-1.0f),
      aggressiveDeltaFrames(0),
      stoppedFrames(0),
      reversingFrames(0),
      netDistanceTraveled(0.0f),
      previousYPosition(y)
{
    useBrain = (controlType == ControlType::AI);
    loadTexture("assets/car.png");
    if (!textureLoaded) { std::cerr << "Warning: Failed to load car texture." << std::endl; }
    if (controlType != ControlType::DUMMY) {
        sensor = std::make_unique<Sensor>(*this); // Cria o sensor
        if (useBrain && sensor) {
             int rayCountValue = sensor ? static_cast<int>(sensor->rayCount) : 5; // Obter contagem de raios do sensor
             // --- MODIFICADO: Rede Neural com 5 saídas ---
             const std::vector<int> networkStructure = {rayCountValue, 6, 5}; // Input, Hidden, Output(Frente/Ré, Esq, Dir, Freio, NãoUsado)
             brain.emplace(NeuralNetwork(networkStructure)); // Cria a rede neural
        }
    }
}

// --- loadTexture (sem alterações) ---
void Car::loadTexture(const std::string& filename) {
    textureLoaded = false; if (!texture.loadFromFile(filename)) return;
    texture.setSmooth(true); sprite.setTexture(texture, true);
    sf::FloatRect textureRect = sprite.getLocalBounds();
    if (textureRect.size.x > 0 && textureRect.size.y > 0) { sprite.setScale({width / textureRect.size.x, height / textureRect.size.y}); }
    else { sprite.setScale({1.0f, 1.0f}); std::cerr << "Warn: Tex zero dim: " << filename << std::endl; return; }
    sprite.setOrigin({textureRect.size.x / 2.0f, textureRect.size.y / 2.0f});
    sprite.setColor(color); textureLoaded = true;
}

int Car::getSensorRayCount() const {
    // Retorna a contagem de raios se o sensor existir, senão 0
    return sensor ? static_cast<int>(sensor->rayCount) : 0;
}

float Car::getPreviousDistanceAhead() const {
    return previousDistanceAhead;
}

int Car::getAggressiveDeltaFrames() const {
    return aggressiveDeltaFrames;
}

// --- Novas Implementações de Métodos de Ação ---

void Car::resetDamage() {
    damaged = false;
}

void Car::resetForTraffic(const sf::Vector2f& startPos, float startSpeed) {
    damaged = false;
    speed = startSpeed;
    position = startPos;
    angle = 0.0f; // Resetar ângulo para frente
    netDistanceTraveled = 0.0f; // Resetar score
    previousYPosition = startPos.y; // Resetar posição Y anterior

    // Resetar contadores de estado interno
    stoppedFrames = 0;
    reversingFrames = 0;
    aggressiveDeltaFrames = 0;
    previousDistanceAhead = -1.0f; // Resetar distância anterior
    lastTurnDirection = 0;
    desiredAcceleration = 0.0f;
    lastAppliedAcceleration = 0.0f;

    // Se tiver sensor, talvez resetar leituras? (O update fará isso)
    // if (sensor) { /* sensor->reset(); ? */ }

    // Resetar controles internos
    controls.forward = (controlType == ControlType::DUMMY); // DUMMY começa andando
    controls.reverse = false;
    controls.left = false;
    controls.right = false;
    controls.brake = false;
}

// --- calculateMinDistanceAhead (sem alterações) ---
float Car::calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut) {
    float minDistance = std::numeric_limits<float>::max(); Car* nearest = nullptr;
    for (const auto& trafficCarPtr : traffic) {
        if (trafficCarPtr && trafficCarPtr != this) {
            // Verifica se o carro está na frente (y menor) e aproximadamente na mesma coluna (x próximo)
            if (trafficCarPtr->position.y < this->position.y && std::abs(trafficCarPtr->position.x - this->position.x) < this->width * 2.0f ) {
                float distance = std::hypot(this->position.x - trafficCarPtr->position.x, this->position.y - trafficCarPtr->position.y);
                if (distance < minDistance) { minDistance = distance; nearest = trafficCarPtr; }
            }
        }
    }
    if (nearestCarOut) *nearestCarOut = nearest;
    return (nearest) ? minDistance : std::numeric_limits<float>::max();
}


// --- Update ---
void Car::update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                 std::vector<Car*>& traffic)
{
    if (damaged) return;

    // 1. Atualiza Sensor
    if (sensor) { sensor->update(roadBorders, traffic); }

    // 2. Determina Controles
    bool intendedTurnLeft = false;
    bool intendedTurnRight = false;
    bool setControlsFromNN = false;
    float aiBrakeSignal = 0.0f; // <-- ADICIONADO: Sinal de freio da IA (0 a 1)
    desiredAcceleration = 0.0f; // Resetar aceleração desejada pela IA

    if (controlType == ControlType::KEYS) {
        controls.update(); // Lê o teclado para forward, reverse, left, right, brake
    }
    else if (useBrain && brain && sensor) {
        // Prepara inputs para a rede
        std::vector<float> sensorOffsets(sensor->rayCount);
        for (size_t i = 0; i < sensor->readings.size(); ++i) {
            sensorOffsets[i] = sensor->readings[i] ? (1.0f - sensor->readings[i]->offset) : 0.0f;
        }
        // Alimenta a rede
        std::vector<float> outputs = NeuralNetwork::feedForward(sensorOffsets, *brain);

        // --- MODIFICADO: Interpreta as 5 saídas ---
        if (outputs.size() >= 5) { // Verifica se temos as 5 saídas esperadas
             // Saída 0: Aceleração / Ré
             if (outputs[0] > 0.6f) desiredAcceleration = acceleration; // Acelerar para frente
             else if (outputs[0] < 0.4f) desiredAcceleration = -acceleration; // Dar ré (força igual à aceleração)
             else desiredAcceleration = 0.0f; // Neutro

             // Saída 1: Virar à Esquerda
             intendedTurnLeft = (outputs[1] > 0.5f);
             // Saída 2: Virar à Direita
             intendedTurnRight = (outputs[2] > 0.5f);

             // Saída 3: Freio Dedicado
             aiBrakeSignal = outputs[3]; // Armazena o sinal de freio (0 a 1)

             // Saída 4: Não utilizada atualmente

             setControlsFromNN = true; // Marca que controles vieram da IA
        }
        // Garante que os controles manuais não interfiram na IA
        controls.forward = false;
        controls.reverse = false;
        controls.brake = false; // Garante que o freio manual não seja ativado pela IA
    }
    else if (controlType == ControlType::DUMMY) {
        controls.forward = true; // DUMMY sempre tenta ir para frente
        controls.reverse = false;
        controls.left = false;
        controls.right = false;
        controls.brake = false;
    }

    // 3. Lógica de Following Agressivo (sem alterações)
    if (controlType == ControlType::AI && sensor) {
        float currentDistanceAhead = calculateMinDistanceAhead(traffic); bool distanceValid = currentDistanceAhead < std::numeric_limits<float>::max();
        if (distanceValid && previousDistanceAhead >= 0) { float distanceDelta = currentDistanceAhead - previousDistanceAhead; if (std::abs(distanceDelta) < AGGRESSIVE_DELTA_THRESHOLD) aggressiveDeltaFrames++; else aggressiveDeltaFrames = 0; } else aggressiveDeltaFrames = 0;
        if (aggressiveDeltaFrames >= AGGRESSIVE_DELTA_DURATION_THRESHOLD) { std::cout << "WARN: Car " << this << " damaged - AGGRESSIVE following." << std::endl; damaged = true; aggressiveDeltaFrames = 0; return; }
        previousDistanceAhead = distanceValid ? currentDistanceAhead : -1.0f;
    } else { aggressiveDeltaFrames = 0; previousDistanceAhead = -1.0f; }

    // 4. Lógica Anti-Pêndulo (sem alterações)
    if (setControlsFromNN) {
        int turn = 0; if(intendedTurnLeft && !intendedTurnRight) turn = -1; else if(intendedTurnRight && !intendedTurnLeft) turn = 1; bool osc = (turn != 0 && turn == -lastTurnDirection); if(osc) { controls.left=false; controls.right=false;} else { controls.left=intendedTurnLeft; controls.right=intendedTurnRight;} lastTurnDirection=turn;
    }
    else if (controlType == ControlType::KEYS) {
        int turn = 0; if(controls.left && !controls.right) turn = -1; else if(controls.right && !controls.left) turn = 1; lastTurnDirection=turn;
    }
    else { lastTurnDirection = 0; }

    // 5. Move o Carro
    move(aiBrakeSignal); // <-- MODIFICADO: Passa o sinal de freio da IA

    // 6. Atualiza Score (Distância Líquida Percorrida) (sem alterações)
    if (controlType == ControlType::AI) {
        float deltaY = previousYPosition - position.y;
        netDistanceTraveled += deltaY;
    }

    // 7. Lógica de Dano por Estar Parado (sem alterações)
    if (controlType == ControlType::AI) {
        if (std::abs(speed) < STOPPED_SPEED_THRESHOLD) stoppedFrames++; else stoppedFrames = 0;
        if (stoppedFrames >= STOPPED_DURATION_THRESHOLD) { std::cout << "WARN: Car " << this << " damaged - STOPPED." << std::endl; damaged = true; stoppedFrames = 0; return; }
    } else { stoppedFrames = 0; }

    // 8. Lógica de Dano por Marcha à Ré (sem alterações - ré ainda é possível via output[0])
    if (controlType == ControlType::AI) {
        if (speed < 0.0f && desiredAcceleration < 0.0f) // Verifica se está ativamente em ré
             reversingFrames++;
        else
            reversingFrames = 0;

        if (reversingFrames >= REVERSE_DURATION_THRESHOLD) {
            std::cout << "WARN: Car " << this << " damaged - REVERSING." << std::endl;
            damaged = true; reversingFrames = 0; return;
        }
    } else { reversingFrames = 0; }


    // Atualiza Posição Anterior para o PRÓXIMO frame (sem alterações)
    previousYPosition = position.y;

    // 9. Avalia Dano por Colisão (sem alterações)
    if (!damaged) { damaged = assessDamage(roadBorders, traffic); }

} // Fim de Car::update

// --- move (MODIFICADO para Freio Dedicado) ---
void Car::move(float aiBrakeSignal) { // Aceita o sinal de freio da IA (0 a 1)
    if (damaged) { speed = 0; return; } // Carro danificado não se move

    float thrust = 0.0f;         // Força de aceleração/ré
    float appliedBrakingForce = 0.0f; // Força de frenagem a ser aplicada

    // 1. Determina Força de Propulsão (Thrust)
    if (controlType == ControlType::AI) {
        thrust = desiredAcceleration; // Usar valor determinado pela IA em update()
    } else { // KEYS ou DUMMY
        if (controls.forward) thrust += acceleration;
        if (controls.reverse) thrust -= acceleration; // Usar 'acceleration' para ré também
    }

    // 2. Determina Força de Frenagem Aplicada
    if (controlType == ControlType::AI) {
        // Interpreta o sinal da IA (ex: aplica freio proporcionalmente se sinal > 0.5)
        if (aiBrakeSignal > 0.5f) {
            // Pode ser proporcional: appliedBrakingForce = brakePower * (aiBrakeSignal - 0.5f) * 2.0f;
            // Ou binário:
            appliedBrakingForce = brakePower;
        }
    } else { // KEYS
        if (controls.brake) { // Verifica controle manual de freio
            appliedBrakingForce = brakePower;
        }
    }

    // 3. Aplica Propulsão (Thrust) à Velocidade
    speed += thrust;

    // 4. Aplica Frenagem e Atrito como Desaceleração
    // A força de frenagem e o atrito sempre se opõem ao movimento atual.
    float totalDeceleration = appliedBrakingForce + friction;

    if (std::abs(speed) > 1e-5) { // Só aplica desaceleração se houver velocidade
        if (std::abs(speed) > totalDeceleration) {
            speed -= totalDeceleration * std::copysign(1.0f, speed); // Reduz a velocidade
        } else {
            speed = 0.0f; // Para o carro se a desaceleração for maior que a velocidade
        }
    }

    // 5. Aplica Limites de Velocidade
    speed = std::clamp(speed, -maxSpeed / 1.5f, maxSpeed); // Limita velocidade de ré também (ex: maxSpeed/1.5)

    // 6. Atualiza Ângulo (Direção) - Lógica original mantida
    if (std::abs(speed) > 1e-5) { // Só vira se estiver em movimento
        float flip = (speed > 0.0f) ? 1.0f : -1.0f; // Inverte direção ao dar ré
        float turnRate = 0.03f;
        if (controls.left) angle -= turnRate * flip;
        if (controls.right) angle += turnRate * flip;
    }

    // 7. Atualiza Posição - Lógica original mantida
    position.x -= std::sin(angle) * speed;
    position.y -= std::cos(angle) * speed;

    // Guarda a última aceleração aplicada (pode precisar redefinir o que isso significa)
    lastAppliedAcceleration = thrust; // Ou talvez thrust - appliedBrakingForce?
}


// --- getPolygon (sem alterações) ---
std::vector<sf::Vector2f> Car::getPolygon() const {
    std::vector<sf::Vector2f> points(4); const float rad = std::hypot(width, height) / 2.0f; const float alpha = std::atan2(width, height);
    float angle_minus_alpha = angle - alpha; float angle_plus_alpha = angle + alpha; float pi = static_cast<float>(M_PI);
    points[0] = { position.x - std::sin(angle_minus_alpha) * rad, position.y - std::cos(angle_minus_alpha) * rad };
    points[1] = { position.x - std::sin(angle_plus_alpha) * rad,  position.y - std::cos(angle_plus_alpha) * rad };
    points[2] = { position.x - std::sin(pi + angle_minus_alpha) * rad, position.y - std::cos(pi + angle_minus_alpha) * rad };
    points[3] = { position.x - std::sin(pi + angle_plus_alpha) * rad,  position.y - std::cos(pi + angle_plus_alpha) * rad };
    return points;
}

// --- assessDamage (sem alterações) ---
bool Car::assessDamage(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders, const std::vector<Car*>& traffic) {
    if (damaged) return true; std::vector<sf::Vector2f> carPoly = getPolygon();
    // Checa colisão com bordas
    for (const auto& border : roadBorders) {
        for (size_t i = 0; i < carPoly.size(); ++i) {
            if (getIntersection(carPoly[i], carPoly[(i + 1) % carPoly.size()], border.first, border.second)) {
                std::cout << "WARN: Car " << this << " damaged - BORDER collision." << std::endl;
                return true;
            }
        }
     }
    // Checa colisão com outros carros
    for (const Car* otherCarPtr : traffic) {
        if (otherCarPtr == nullptr || otherCarPtr == this) continue; // Ignora a si mesmo ou ponteiros nulos
        if (polysIntersect(carPoly, otherCarPtr->getPolygon())) {
             std::cout << "WARN: Car " << this << " damaged - TRAFFIC collision with " << otherCarPtr << std::endl;
             return true;
         }
     }
    return false;
}

// --- draw (sem alterações) ---
void Car::draw(sf::RenderTarget& target, bool drawSensorFlag) {
     sf::Color drawColorToUse = color; if (damaged) drawColorToUse = sf::Color(150, 150, 150, 180); // Cinza se danificado
     if (textureLoaded) {
         sprite.setColor(drawColorToUse); sprite.setPosition(position); sprite.setRotation(sf::degrees(angle * 180.0f / static_cast<float>(M_PI))); target.draw(sprite);
     }
     else { // Fallback se textura não carregou
        sf::RectangleShape fallbackRect({width, height}); fallbackRect.setOrigin({width / 2.0f, height / 2.0f}); fallbackRect.setPosition(position); fallbackRect.setRotation(sf::degrees(angle * 180.0f / static_cast<float>(M_PI)));
        sf::Color fallbackColor = damaged ? drawColorToUse : color;
        // Muda a cor do fallback se não estiver danificado para identificar tipo
        if (!damaged) {
             switch (controlType) {
                case ControlType::DUMMY: fallbackColor = sf::Color::Red; break;
                case ControlType::AI:    fallbackColor = sf::Color(0, 0, 255, 180); break; // Azul translúcido para IA
                case ControlType::KEYS:  fallbackColor = sf::Color::Blue; break; // Azul sólido para jogador
                default: fallbackColor = sf::Color::White; break;
             }
        }
        fallbackRect.setFillColor(fallbackColor);
        fallbackRect.setOutlineColor(sf::Color::White); // Adiciona contorno para visibilidade
        fallbackRect.setOutlineThickness(1);
        target.draw(fallbackRect);
     }
     // Desenha o sensor se existir, não estiver danificado e a flag estiver ativa
     if (sensor && drawSensorFlag && !damaged) sensor->draw(target);
 }

// --- Getters adicionados para consistência com header assumido ---
// (Implementação trivial, apenas retorna o membro)
// bool Car::isDamaged() const { return damaged; }
// float Car::getScore() const { return netDistanceTraveled; }
// sf::Vector2f Car::getPosition() const { return position; }
// float Car::getAngle() const { return angle; }
// float Car::getSpeed() const { return speed; }