#include "Car.hpp"
#include "Utils.hpp"
#include <cmath>
#include <iostream>
#include <SFML/Graphics/RectangleShape.hpp>
#include <limits>
#include <algorithm>

// Construtor
Car::Car(float x, float y, float w, float h, ControlType type, float maxSpd, sf::Color col)
    : position(x, y),
      width(w),
      height(h),
      maxSpeed(maxSpd),
      controlType(type),
      controls(type),
      color(col),
      textureLoaded(false),
      sprite(texture),
      isFollowing(false), // Inicializa novo status
      lastTurnDirection(0),
      desiredAcceleration(0.0f),
      lastAppliedAcceleration(0.0f),
      previousDistanceAhead(-1.0f),
      distanceChangeDirection(0),
      lastDistanceChangeDirection(0),
      directionFlipCounter(0),
      followingDuration(0) // Inicializa novo contador
{
    useBrain = (controlType == ControlType::AI);
    loadTexture("assets/car.png");
    if (!textureLoaded) {
        std::cerr << "Warning: Failed to load car texture 'assets/car.png'. Using fallback rectangle." << std::endl;
    }
    if (controlType != ControlType::DUMMY) {
        sensor = std::make_unique<Sensor>(*this);
        if (useBrain && sensor) {
             const std::vector<int> networkStructure = {static_cast<int>(sensor->rayCount), 6, 4};
             brain.emplace(NeuralNetwork(networkStructure));
        }
    }
}

// loadTexture
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
         std::cerr << "Warning: Texture loaded with zero dimensions: " << filename << ". Setting scale to 1." << std::endl;
         return;
    }
    sprite.setOrigin({textureRect.size.x / 2.0f, textureRect.size.y / 2.0f});
    sprite.setColor(color);
    textureLoaded = true;
}

// calculateMinDistanceAhead
float Car::calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut) {
    float minDistance = std::numeric_limits<float>::max();
    Car* nearest = nullptr;
    for (const auto& trafficCarPtr : traffic) {
        if (trafficCarPtr && trafficCarPtr != this) {
            if (trafficCarPtr->position.y < this->position.y) {
                float distance = std::hypot(this->position.x - trafficCarPtr->position.x, this->position.y - trafficCarPtr->position.y);
                if (distance < minDistance) {
                    minDistance = distance;
                    nearest = trafficCarPtr;
                }
            }
        }
    }
    if (nearestCarOut) *nearestCarOut = nearest;
    return (nearest) ? minDistance : std::numeric_limits<float>::max();
}


// Update function with refined following logic
void Car::update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                 std::vector<Car*>& traffic)
{
    if (damaged) return;

    // 1. Atualizar Sensor
    if (sensor) {
        sensor->update(roadBorders, traffic);
    }

    // 2. Calcular Controles e Aceleração Desejada
    bool intendedTurnLeft = false;
    bool intendedTurnRight = false;
    bool setControlsFromNN = false;
    desiredAcceleration = 0.0f;

    if (controlType == ControlType::KEYS) {
        controls.update();
    }
    else if (useBrain && brain && sensor) {
        std::vector<float> sensorOffsets(sensor->rayCount);
        for (size_t i = 0; i < sensor->readings.size(); ++i) {
            sensorOffsets[i] = sensor->readings[i] ? (1.0f - sensor->readings[i]->offset) : 0.0f;
        }
        std::vector<float> outputs = NeuralNetwork::feedForward(sensorOffsets, *brain);
        if (outputs.size() >= 4) {
             if (outputs[0] > 0.55f) desiredAcceleration = acceleration;
             else if (outputs[0] < 0.45f) desiredAcceleration = -acceleration * 1.5f;
             else desiredAcceleration = 0.0f;
             intendedTurnLeft = (outputs[1] > 0.5f);
             intendedTurnRight = (outputs[2] > 0.5f);
             setControlsFromNN = true;
        }
         controls.forward = false;
         controls.reverse = false;
    }
    else if (controlType == ControlType::DUMMY) {
         controls.forward = true;
         controls.reverse = false;
         controls.left = false;
         controls.right = false;
    }

    // --- 3. LÓGICA REFINADA DE DETECÇÃO DE "FOLLOWING" ---
    bool isCurrentlyOscillating = false; // Flag para este quadro
    if (controlType == ControlType::AI && sensor) {
        float currentDistanceAhead = calculateMinDistanceAhead(traffic);
        int currentChangeDirection = 0;
        bool distanceValidAndClose = currentDistanceAhead < sensor->rayLength;

        if (distanceValidAndClose && previousDistanceAhead > 0) {
            float distanceDelta = currentDistanceAhead - previousDistanceAhead;
            if (distanceDelta < -DISTANCE_CHANGE_THRESHOLD) currentChangeDirection = 1;
            else if (distanceDelta > DISTANCE_CHANGE_THRESHOLD) currentChangeDirection = -1;

            if (currentChangeDirection != 0 && lastDistanceChangeDirection != 0 && currentChangeDirection != lastDistanceChangeDirection) {
                directionFlipCounter++;
                // Log do Flip Counter (descomente se necessário)
                // if(directionFlipCounter > 0) std::cout << "DEBUG: Car " << this << " FlipCounter: " << directionFlipCounter << std::endl;
                isCurrentlyOscillating = (directionFlipCounter >= OSCILATION_START_THRESHOLD);
            } else if (currentChangeDirection != 0 && currentChangeDirection == lastDistanceChangeDirection) {
                if(directionFlipCounter > 0) directionFlipCounter = 0; // Só reseta se era > 0
            } else if (currentChangeDirection == 0 && std::abs(speed) < friction * 1.5f) {
                if(directionFlipCounter > 0) directionFlipCounter = 0; // Só reseta se era > 0
            }

            if (currentChangeDirection != 0) lastDistanceChangeDirection = currentChangeDirection;

        } else {
            if(directionFlipCounter > 0) directionFlipCounter = 0; // Reset se perdeu carro/dist inválida
            if(lastDistanceChangeDirection != 0) lastDistanceChangeDirection = 0; // Reset se perdeu carro/dist inválida
        }
        previousDistanceAhead = (currentDistanceAhead < std::numeric_limits<float>::max()) ? currentDistanceAhead : -1.0f;

        // Atualiza estado 'isFollowing' e 'followingDuration'
        if (isCurrentlyOscillating) {
            if (!isFollowing) { // Transição para true
                 std::cout << "****** DEBUG: Car " << this << " SET isFollowing = TRUE ******" << std::endl;
                 isFollowing = true;
                 followingDuration = 0; // Reseta duração ao começar
                 // directionFlipCounter = 0; // Não reseta aqui para permitir detecção contínua
            }
             // Se já era true, continua true
        } else { // Não está oscilando neste frame
             if (isFollowing) { // Transição para false
                  std::cout << "****** DEBUG: Car " << this << " SET isFollowing = FALSE ******" << std::endl;
                  isFollowing = false;
                  followingDuration = 0;
             }
             // Se já era false, continua false
        }

        // Incrementa duração e verifica dano se estiver seguindo
        if (isFollowing) {
            followingDuration++;

             // Log da Duração (descomente e ajuste frequência se necessário)
            std::cout << "DEBUG: Car " << this << " followingDuration: " << followingDuration << std::endl;
            if (followingDuration >= FOLLOWING_DAMAGE_THRESHOLD) {
                std::cout << "WARN: Car " << this << " damaged due to prolonged following ("
                          << followingDuration << " frames)." << std::endl;
                damaged = true;
                return; // Para a atualização
            }
        }

    } else { // Reseta se não for AI ou sem sensor
        if(isFollowing) isFollowing = false; // Garante reset se tipo mudar
        if(followingDuration > 0) followingDuration = 0;
        if(previousDistanceAhead > -1.0f) previousDistanceAhead = -1.0f;
        if(lastDistanceChangeDirection != 0) lastDistanceChangeDirection = 0;
        if(directionFlipCounter > 0) directionFlipCounter = 0;
    }
    // --- Fim da Lógica de Following ---


    // 4. Lógica Anti-Pêndulo LATERAL (sem mudanças)
    if (setControlsFromNN) {
        int currentIntendedTurn = 0;
        if (intendedTurnLeft && !intendedTurnRight) currentIntendedTurn = -1;
        else if (intendedTurnRight && !intendedTurnLeft) currentIntendedTurn = 1;
        bool isOscillatingLaterally = (currentIntendedTurn != 0 && currentIntendedTurn == -lastTurnDirection);
        if (isOscillatingLaterally) {
            controls.left = false; controls.right = false;
        } else {
            controls.left = intendedTurnLeft; controls.right = intendedTurnRight;
        }
        lastTurnDirection = currentIntendedTurn;
    } else if (controlType == ControlType::KEYS) {
        int currentFinalTurnDirection = 0;
        if (controls.left && !controls.right) currentFinalTurnDirection = -1;
        else if (controls.right && !controls.left) currentFinalTurnDirection = 1;
        lastTurnDirection = currentFinalTurnDirection;
    } else if (controlType == ControlType::DUMMY) {
        lastTurnDirection = 0;
    }

    // 5. Mover o Carro (sem mudanças)
    move();

    // 6. Avaliar Danos por Colisão (sem mudanças)
    if (!damaged) {
        damaged = assessDamage(roadBorders, traffic);
    }

} // End of Car::update

// move
void Car::move() {
    float currentAppliedAccel = 0.0f;
    if (!damaged) {
        if (controlType == ControlType::AI) currentAppliedAccel = desiredAcceleration;
        else {
            if (controls.forward) currentAppliedAccel += acceleration;
            if (controls.reverse) currentAppliedAccel -= acceleration;
        }
    }
    speed += currentAppliedAccel;
    if (std::abs(speed) > friction) speed -= friction * std::copysign(1.0f, speed);
    else speed = 0.0f;
    speed = std::clamp(speed, -maxSpeed / 2.0f, maxSpeed);
    if (std::abs(speed) > 1e-5 && !damaged) {
        float flip = (speed > 0.0f) ? 1.0f : -1.0f;
        float turnRate = 0.03f;
        if (controls.left) angle -= turnRate * flip;
        if (controls.right) angle += turnRate * flip;
    }
    position.x -= std::sin(angle) * speed;
    position.y -= std::cos(angle) * speed;
}

// getPolygon
std::vector<sf::Vector2f> Car::getPolygon() const {
    std::vector<sf::Vector2f> points(4);
    const float rad = std::hypot(width, height) / 2.0f;
    const float alpha = std::atan2(width, height);
    float angle_minus_alpha = angle - alpha;
    float angle_plus_alpha = angle + alpha;
    float pi = static_cast<float>(M_PI);
    points[0] = { position.x - std::sin(angle_minus_alpha) * rad, position.y - std::cos(angle_minus_alpha) * rad };
    points[1] = { position.x - std::sin(angle_plus_alpha) * rad, position.y - std::cos(angle_plus_alpha) * rad };
    points[2] = { position.x - std::sin(pi + angle_minus_alpha) * rad, position.y - std::cos(pi + angle_minus_alpha) * rad };
    points[3] = { position.x - std::sin(pi + angle_plus_alpha) * rad, position.y - std::cos(pi + angle_plus_alpha) * rad };
    return points;
}

// assessDamage
bool Car::assessDamage(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                       const std::vector<Car*>& traffic)
{
    if (damaged) return true;
    std::vector<sf::Vector2f> carPoly = getPolygon();
    for (const auto& border : roadBorders) {
        std::vector<sf::Vector2f> borderPoly = {border.first, border.second, border.second, border.first};
        if (polysIntersect(carPoly, borderPoly)) {
             std::cout << "Collision detected: Car " << this << " with road border." << std::endl;
            return true;
        }
    }
    for (const Car* otherCarPtr : traffic) {
         if (otherCarPtr == nullptr || otherCarPtr == this) continue;
        if (polysIntersect(carPoly, otherCarPtr->getPolygon())) {
            std::cout << "Collision detected: Car " << this << " with Car " << otherCarPtr << std::endl;
            return true;
        }
    }
    return false;
}

// draw
void Car::draw(sf::RenderTarget& target, bool drawSensorFlag) {
    sf::Color drawColorToUse = color;
    if (damaged) {
        drawColorToUse = sf::Color(150, 150, 150, 180); // Cinza se danificado
    } else if (isFollowing) {
        drawColorToUse = sf::Color(255, 165, 0, 200); // Laranja aviso
        // Log da Cor (descomente e ajuste se necessário)
        // if (this == bestCar) // Log apenas para o melhor carro para não poluir muito
        //     std::cout << "DEBUG DRAW: Car " << this << " isFollowing=TRUE, Color=Orange" << std::endl;
    } else {
         // Log da Cor (descomente e ajuste se necessário)
         // if (this == bestCar)
         //     std::cout << "DEBUG DRAW: Car " << this << " isFollowing=FALSE, Color=Base/Normal" << std::endl;
    }

    if (textureLoaded) {
        sprite.setColor(drawColorToUse); // Aplica a cor
        sprite.setPosition(position);
        sprite.setRotation(sf::degrees(-angle));
        target.draw(sprite);
    } else {
        sf::RectangleShape fallbackRect({width, height});
        fallbackRect.setOrigin({width / 2.0f, height / 2.0f});
        fallbackRect.setPosition(position);
        fallbackRect.setRotation(sf::degrees(-angle));
        sf::Color fallbackColor;
        if (damaged || isFollowing) {
            fallbackColor = drawColorToUse; // Usa a cor especial
        } else { // Usa a cor baseada no tipo
            switch (controlType) {
                case ControlType::DUMMY: fallbackColor =sf::Color::Red; break;
                case ControlType::AI:    fallbackColor = sf::Color(0, 0, 255, 180); break;
                case ControlType::KEYS:  fallbackColor = sf::Color::Blue; break;
                default: fallbackColor = sf::Color::White; break;
            }
        }
        fallbackRect.setFillColor(fallbackColor);
        target.draw(fallbackRect);
    }

    if (sensor && drawSensorFlag && !damaged) {
        sensor->draw(target);
    }
}