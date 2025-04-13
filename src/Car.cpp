// src/Car.cpp (COMPLETO E ATUALIZADO com timer para 'stopped')
#include "Car.hpp"
#include "Utils.hpp"
#include <iostream>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <limits>
#include <algorithm>
#include <cmath>

// --- Construtor ---
Car::Car(float x, float y, float w, float h, ControlType type, float maxSpd, sf::Color col)
    : position(x, y),
      width(w),
      height(h),
      maxSpeed(maxSpd),
      acceleration(0.2f),
      brakePower(acceleration * 2.0f),
      friction(0.05f),
      controlType(type),
      controls(type),
      color(col),
      textureLoaded(false),
      sprite(texture),
      lastTurnDirection(0),
      desiredAcceleration(0.0f),
      lastAppliedAcceleration(0.0f),
      previousDistanceAhead(-1.0f),
      aggressiveDeltaFrames(0),
      aggressiveFollowingTimer(0.0f),
      // stoppedFrames(0), // Removido
      stoppedTimer(0.0f), // Inicializa novo timer
      reversingTimer(0.0f),
      netDistanceTraveled(0.0f),
      previousYPosition(y)
{
    useBrain = (controlType == ControlType::AI);
    loadTexture("assets/car.png");
    if (!textureLoaded) { std::cerr << "Warning: Failed to load car texture (assets/car.png)." << std::endl; }

    if (controlType != ControlType::DUMMY) {
        sensor = std::make_unique<Sensor>(*this);
        if (useBrain && sensor) {
             int rayCountValue = sensor ? static_cast<int>(sensor->rayCount) : 5;
             const std::vector<int> networkStructure = {rayCountValue, 6, 5};
             brain.emplace(NeuralNetwork(networkStructure));
        }
    }
}

// --- loadTexture ---
// ... (código existente sem alterações) ...
void Car::loadTexture(const std::string& filename) {
    textureLoaded = false;
    if (!texture.loadFromFile(filename)) return;
    texture.setSmooth(true);
    sprite.setTexture(texture, true);
    sf::FloatRect textureRect = sprite.getLocalBounds();
    if (textureRect.size.x > 0 && textureRect.size.y > 0) {
        sprite.setScale({width / textureRect.size.x, height / textureRect.size.y});
    } else {
        sprite.setScale({1.0f, 1.0f});
        std::cerr << "Warning: Texture loaded with zero dimensions: " << filename << std::endl;
        return;
    }
    sprite.setOrigin({textureRect.size.x / 2.0f, textureRect.size.y / 2.0f});
    sprite.setColor(color);
    textureLoaded = true;
}


// --- Getters ---
// ... (código existente sem alterações) ...
int Car::getSensorRayCount() const { return sensor ? static_cast<int>(sensor->rayCount) : 0; }
float Car::getPreviousDistanceAhead() const { return previousDistanceAhead; }
int Car::getAggressiveDeltaFrames() const { return aggressiveDeltaFrames; }


// --- resetDamage ---
// ... (código existente sem alterações) ...
void Car::resetDamage() { damaged = false; }

// --- resetForTraffic (MODIFICADO) ---
void Car::resetForTraffic(const sf::Vector2f& startPos, float startSpeed) {
    damaged = false;
    speed = startSpeed;
    position = startPos;
    angle = 0.0f;
    netDistanceTraveled = 0.0f;
    previousYPosition = startPos.y;
    stoppedTimer = 0.0f; // *** Reseta o timer de parado ***
    reversingTimer = 0.0f; // *** Reseta o timer de ré *** = 0;
    aggressiveDeltaFrames = 0;
    aggressiveFollowingTimer = 0.0f;
    previousDistanceAhead = -1.0f;
    lastTurnDirection = 0;
    desiredAcceleration = 0.0f;
    lastAppliedAcceleration = 0.0f;
    controls.forward = (controlType == ControlType::DUMMY);
    controls.reverse = false;
    controls.left = false;
    controls.right = false;
    controls.brake = false;
}

// --- calculateMinDistanceAhead ---
// ... (código existente sem alterações) ...
float Car::calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut) {
    float minDistance = std::numeric_limits<float>::max();
    Car* nearest = nullptr;
    float checkWidth = this->width * 2.0f;

    for (const auto& otherCarPtr : traffic) {
        if (!otherCarPtr || otherCarPtr == this) continue;
        if (otherCarPtr->position.y < this->position.y) {
            if (std::abs(otherCarPtr->position.x - this->position.x) < checkWidth) {
                float frontY = this->position.y - this->height / 2.0f;
                float otherRearY = otherCarPtr->position.y + otherCarPtr->height / 2.0f;
                float distance = frontY - otherRearY;
                if (distance >= 0 && distance < minDistance) {
                    minDistance = distance;
                    nearest = otherCarPtr;
                }
            }
        }
    }
    if (nearestCarOut) *nearestCarOut = nearest;
    return minDistance;
}


// --- Update (MODIFICADO) ---
void Car::update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                 std::vector<Car*>& traffic,
                 sf::Time deltaTime)
{
    if (damaged) return;

    // 1. Atualiza Sensor
    if (sensor) { sensor->update(roadBorders, traffic); }

    // 2. Determina Controles
    bool intendedTurnLeft = false;
    bool intendedTurnRight = false;
    bool setControlsFromNN = false;
    float aiBrakeSignal = 0.0f;
    // Resetar intenção da IA (será definida abaixo se AI)
    desiredAcceleration = 0.0f; // Resetar intenção de aceleração/ré da IA

    if (controlType == ControlType::KEYS) { controls.update(); }
    else if (useBrain && brain && sensor) {
        std::vector<float> sensorOffsets(sensor->rayCount);
        for (size_t i = 0; i < sensor->readings.size(); ++i) { sensorOffsets[i] = sensor->readings[i] ? (1.0f - sensor->readings[i]->offset) : 0.0f; }
        std::vector<float> outputs = NeuralNetwork::feedForward(sensorOffsets, *brain);
        if (outputs.size() >= 5) {
            // *** Atualiza desiredAcceleration baseado na saída 0 ***
            if (outputs[0] > 0.6f) desiredAcceleration = acceleration;
            else if (outputs[0] < 0.4f) desiredAcceleration = -acceleration; // Intenção de ré
            else desiredAcceleration = 0.0f;
            // Define controles booleanos baseado na intenção (para compatibilidade com 'move' e lógica anti-pendulo)
            controls.forward = (desiredAcceleration > 0.0f);
            controls.reverse = (desiredAcceleration < 0.0f); // Intenção de ré

            intendedTurnLeft = (outputs[1] > 0.5f);
            intendedTurnRight = (outputs[2] > 0.5f);
            aiBrakeSignal = outputs[3];
            // Saída 4 não usada diretamente para controle de ré aqui, usamos desiredAcceleration
            setControlsFromNN = true;
        }
        // Garante que controle de freio manual não interfira
        controls.brake = false;
    }
    else if (controlType == ControlType::DUMMY) {
        controls.forward = true; controls.reverse = false; controls.left = false; controls.right = false; controls.brake = false;
        desiredAcceleration = acceleration; // Dummy tem intenção de acelerar
    }

    // 3. Lógica Anti-Pêndulo
    if (setControlsFromNN) {
        int turn = 0; if(intendedTurnLeft && !intendedTurnRight) turn = -1; else if(intendedTurnRight && !intendedTurnLeft) turn = 1;
        bool isOscillating = (turn != 0 && turn == -lastTurnDirection);
        if (isOscillating) { controls.left = false; controls.right = false;} else { controls.left = intendedTurnLeft; controls.right = intendedTurnRight;}
        lastTurnDirection = turn;
    } else if (controlType == ControlType::KEYS) {
        int turn = 0; if(controls.left && !controls.right) turn = -1; else if(controls.right && !controls.left) turn = 1; lastTurnDirection=turn;
    } else { lastTurnDirection = 0; }

    // 4. Move o Carro
    move(aiBrakeSignal, deltaTime);

    // 5. Atualiza Score
    if (controlType == ControlType::AI) {
        float deltaY = previousYPosition - position.y;
        netDistanceTraveled += deltaY;
    }

    // --- Verificações de Dano ---
    // 6. Checa Following Agressivo
    checkAggressiveFollowing(traffic, deltaTime);
    if (damaged) return;

    // 7. Checa Dano por Estar Parado
    checkStoppedStatus(deltaTime);
    if (damaged) return;

    // 8. *** Checa Dano por Marcha à Ré (usando timer) ***
    checkReversingStatus(deltaTime); // Chama a nova função
    if (damaged) return;

    // Atualiza Posição Y Anterior
    previousYPosition = position.y;

    // 9. Avalia Dano por Colisão
    if (!damaged) {
        damaged = checkForCollision(roadBorders, traffic);
        if(damaged) { std::cerr << "WARN: Car " << this << " damaged - COLLISION." << std::endl;}
    }
}


// --- move ---
// ... (código existente sem alterações) ...
void Car::move(float aiBrakeSignal, sf::Time deltaTime) {
    if (damaged) { speed = 0; return; }
    float dtSeconds = deltaTime.asSeconds();
    if (dtSeconds <= 0) return;
    const float timeScaleFactor = dtSeconds * 60.0f;

    float thrust = 0.0f;
    float appliedBrakingForce = 0.0f;

    // Thrust agora considera desiredAcceleration para AI
    if (controlType == ControlType::AI) { thrust = desiredAcceleration; }
    else { if (controls.forward) thrust += acceleration; if (controls.reverse) thrust -= acceleration; }

    if (controlType == ControlType::AI) { if (aiBrakeSignal > 0.5f) appliedBrakingForce = brakePower; }
    else { if (controls.brake) appliedBrakingForce = brakePower; }

    speed += thrust * timeScaleFactor;

    float totalDecelerationForce = (appliedBrakingForce + friction) * timeScaleFactor;
    if (std::abs(speed) > 1e-5f) { if (std::abs(speed) > totalDecelerationForce) { speed -= totalDecelerationForce * std::copysign(1.0f, speed); } else { speed = 0.0f; } }
    speed = std::clamp(speed, -maxSpeed * (2.0f/3.0f), maxSpeed);

    if (std::abs(speed) > 1e-5f) {
        float flip = (speed > 0.0f) ? 1.0f : -1.0f;
        float turnRateRad = 0.03f * timeScaleFactor;
        if (controls.left) angle -= turnRateRad * flip;
        if (controls.right) angle += turnRateRad * flip;
    }
    position.x -= std::sin(angle) * speed * timeScaleFactor;
    position.y -= std::cos(angle) * speed * timeScaleFactor;
    lastAppliedAcceleration = thrust; // Pode ser útil manter isso
}

void Car::checkReversingStatus(sf::Time deltaTime) {
    // Só executa para carros AI que não estão danificados
    if (damaged || controlType != ControlType::AI) {
        reversingTimer = 0.0f; // Reseta se não for AI ou já danificado
        return;
    }

    // Condição para considerar "em ré": velocidade negativa E intenção de ré (da IA)
    // Usamos desiredAcceleration < 0 como indicador da intenção da IA de ir para trás.
    bool isReversingIntended = (speed < -0.05f) && (desiredAcceleration < 0.0f);
    // Alternativa: usar controls.reverse se a IA definir esse booleano corretamente
    // bool isReversingIntended = (speed < -0.05f) && controls.reverse;

    if (isReversingIntended) {
        // Incrementa o timer de ré
        reversingTimer += deltaTime.asSeconds();
    } else {
        // Se não está em ré (ou a intenção mudou), reseta o timer
        reversingTimer = 0.0f;
    }

    // Verifica se o limite de tempo em ré foi atingido
    if (reversingTimer >= REVERSE_TIME_THRESHOLD_SECONDS) {
        // Aplica dano se ainda não foi danificado neste frame
        if (!damaged) {
            damaged = true;
            speed = 0; // Para o carro
            std::cerr << "WARN: Car " << this << " damaged - REVERSING for "
                      << reversingTimer << "s (Threshold: "
                      << REVERSE_TIME_THRESHOLD_SECONDS << "s)." << std::endl;
            reversingTimer = 0.0f; // Reseta após aplicar dano
        }
    }
}

// --- checkAggressiveFollowing ---
// ... (código existente da resposta anterior, sem alterações) ...
void Car::checkAggressiveFollowing(const std::vector<Car*>& traffic, sf::Time deltaTime) {
    if (damaged || controlType != ControlType::AI) return;
    float currentDistance = calculateMinDistanceAhead(traffic);
    bool hasValidCurrent = (currentDistance < std::numeric_limits<float>::max());
    float delta = 0.0f;
    if (hasValidCurrent && previousDistanceAhead >= 0.0f) { delta = currentDistance - previousDistanceAhead; }
    else if (!hasValidCurrent) { delta = 1.0f; }

    bool isFollowingAggressively = hasValidCurrent &&
                                  (currentDistance > AGGRESSIVE_DISTANCE_THRESHOLD) &&
                                  (currentDistance < AGGRESSIVE_DISTANCE_MAX);
                                //   (delta < AGGRESSIVE_DELTA_THRESHOLD);

    if (isFollowingAggressively) { aggressiveDeltaFrames++; } else { aggressiveDeltaFrames = 0; }
    if (isFollowingAggressively) { aggressiveFollowingTimer += deltaTime.asSeconds(); }
    else { aggressiveFollowingTimer = 0.0f; }

    if (aggressiveFollowingTimer >= AGGRESSIVE_FOLLOWING_DURATION_THRESHOLD) {
        if (!damaged) {
            damaged = true; speed = 0;
            std::cerr << "WARN: Car " << this << " damaged - AGGRESSIVE following criteria met (Dist: "
                      << currentDistance << ", Delta: " << delta << ", Time: " << aggressiveFollowingTimer << "s)." << std::endl;
            aggressiveFollowingTimer = 0.0f; aggressiveDeltaFrames = 0;
        }
    }
    previousDistanceAhead = hasValidCurrent ? currentDistance : -1.0f;
}


// --- *** NOVA FUNÇÃO: checkStoppedStatus *** ---
void Car::checkStoppedStatus(sf::Time deltaTime) {
    // Só executa para carros AI que não estão danificados
    if (damaged || controlType != ControlType::AI) {
        stoppedTimer = 0.0f; // Reseta se não for AI ou já danificado
        return;
    }

    // Verifica se a velocidade está abaixo do limiar de "parado"
    if (std::abs(speed) < STOPPED_SPEED_THRESHOLD) {
        // Incrementa o timer de parado
        stoppedTimer += deltaTime.asSeconds();
    } else {
        // Se o carro está se movendo, reseta o timer
        stoppedTimer = 0.0f;
    }

    // Verifica se o limite de tempo parado foi atingido
    if (stoppedTimer >= STOPPED_TIME_THRESHOLD_SECONDS) {
        // Aplica dano se ainda não foi danificado neste frame
        if (!damaged) {
            damaged = true;
            speed = 0; // Garante que pare completamente
            std::cerr << "WARN: Car " << this << " damaged - STOPPED for "
                      << stoppedTimer << "s (Threshold: "
                      << STOPPED_TIME_THRESHOLD_SECONDS << "s)." << std::endl;
            stoppedTimer = 0.0f; // Reseta após aplicar dano
        }
    }
}


// --- getPolygon ---
// ... (código existente sem alterações) ...
std::vector<sf::Vector2f> Car::getPolygon() const {
    std::vector<sf::Vector2f> points(4);
    const float rad = std::hypot(width, height) / 2.0f;
    const float alpha = std::atan2(width, height);
    float currentAngle = angle; // Radianos

    points[0] = { position.x - std::sin(currentAngle - alpha) * rad, position.y - std::cos(currentAngle - alpha) * rad };
    points[1] = { position.x - std::sin(currentAngle + alpha) * rad, position.y - std::cos(currentAngle + alpha) * rad };
    points[2] = { position.x - std::sin(static_cast<float>(M_PI) + currentAngle - alpha) * rad, position.y - std::cos(static_cast<float>(M_PI) + currentAngle - alpha) * rad };
    points[3] = { position.x - std::sin(static_cast<float>(M_PI) + currentAngle + alpha) * rad, position.y - std::cos(static_cast<float>(M_PI) + currentAngle + alpha) * rad };
    return points;
}

// --- checkForCollision ---
// ... (código existente sem alterações) ...
bool Car::checkForCollision(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders, const std::vector<Car*>& traffic) {
    if (damaged) return true;
    std::vector<sf::Vector2f> carPoly = getPolygon();
    for (const auto& border : roadBorders) {
        for (size_t i = 0; i < carPoly.size(); ++i) {
            if (getIntersection(carPoly[i], carPoly[(i + 1) % carPoly.size()], border.first, border.second)) { return true; }
        }
     }
    for (const Car* otherCarPtr : traffic) {
        if (!otherCarPtr || otherCarPtr == this) continue;
        if (polysIntersect(carPoly, otherCarPtr->getPolygon())) { return true; }
     }
    return false;
}

// --- draw ---
// ... (código existente sem alterações) ...
void Car::draw(sf::RenderTarget& target, bool drawSensorFlag) {
     sf::Color drawColorToUse = damaged ? sf::Color(100, 100, 100, 180) : color;
     if (textureLoaded) {
         sprite.setColor(drawColorToUse); sprite.setPosition(position);
         sprite.setRotation(sf::degrees(angle * 180.0f / static_cast<float>(M_PI)));
         target.draw(sprite);
     }
     else {
        sf::RectangleShape fallbackRect({width, height}); fallbackRect.setOrigin({width / 2.0f, height / 2.0f});
        fallbackRect.setPosition(position); fallbackRect.setRotation(sf::degrees(angle * 180.0f / static_cast<float>(M_PI)));
        sf::Color fallbackColor = drawColorToUse;
        if (!damaged) {
             switch (controlType) {
                case ControlType::DUMMY: fallbackColor = sf::Color::Red; break;
                case ControlType::AI:    fallbackColor = sf::Color(0, 0, 255, 180); break;
                case ControlType::KEYS:  fallbackColor = sf::Color::Blue; break;
                default: fallbackColor = sf::Color::White; break;
             }
        }
        fallbackRect.setFillColor(fallbackColor); fallbackRect.setOutlineColor(sf::Color::White); fallbackRect.setOutlineThickness(1);
        target.draw(fallbackRect);
     }
     if (sensor && drawSensorFlag && !damaged) { sensor->draw(target); }
 }