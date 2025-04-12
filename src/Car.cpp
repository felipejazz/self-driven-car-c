#include "Car.hpp"
#include "Utils.hpp"
#include <cmath>
#include <iostream>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Transform.hpp>
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
      lastTurnDirection(0),
      desiredAcceleration(0.0f),
      lastAppliedAcceleration(0.0f),
      previousDistanceAhead(-1.0f),
      aggressiveDeltaFrames(0),
      stoppedFrames(0),
      reversingFrames(0),
      netDistanceTraveled(0.0f), // Inicializa score
      previousYPosition(y)     // Inicializa posição anterior
{
    useBrain = (controlType == ControlType::AI);
    loadTexture("assets/car.png");
    if (!textureLoaded) { std::cerr << "Warning: Failed to load car texture." << std::endl; }
    if (controlType != ControlType::DUMMY) {
        sensor = std::make_unique<Sensor>(*this);
        if (useBrain && sensor) {
             int rayCountValue = sensor ? static_cast<int>(sensor->rayCount) : 5;
             const std::vector<int> networkStructure = {rayCountValue, 6, 4};
             brain.emplace(NeuralNetwork(networkStructure));
        }
    }
}

// loadTexture (sem alterações)
void Car::loadTexture(const std::string& filename) {
    textureLoaded = false; if (!texture.loadFromFile(filename)) return;
    texture.setSmooth(true); sprite.setTexture(texture, true);
    sf::FloatRect textureRect = sprite.getLocalBounds();
    if (textureRect.size.x > 0 && textureRect.size.y > 0) { sprite.setScale({width / textureRect.size.x, height / textureRect.size.y}); }
    else { sprite.setScale({1.0f, 1.0f}); std::cerr << "Warn: Tex zero dim: " << filename << std::endl; return; }
    sprite.setOrigin({textureRect.size.x / 2.0f, textureRect.size.y / 2.0f});
    sprite.setColor(color); textureLoaded = true;
}

// calculateMinDistanceAhead (sem alterações)
float Car::calculateMinDistanceAhead(const std::vector<Car*>& traffic, Car** nearestCarOut) {
    float minDistance = std::numeric_limits<float>::max(); Car* nearest = nullptr;
    for (const auto& trafficCarPtr : traffic) {
        if (trafficCarPtr && trafficCarPtr != this) {
            if (trafficCarPtr->position.y < this->position.y && std::abs(trafficCarPtr->position.x - this->position.x) < this->width * 2.0f ) {
                float distance = std::hypot(this->position.x - trafficCarPtr->position.x, this->position.y - trafficCarPtr->position.y);
                if (distance < minDistance) { minDistance = distance; nearest = trafficCarPtr; }
            }
        }
    }
    if (nearestCarOut) *nearestCarOut = nearest;
    return (nearest) ? minDistance : std::numeric_limits<float>::max();
}


// Update
void Car::update(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                 std::vector<Car*>& traffic)
{
    if (damaged) return;

    // 1. Atualiza Sensor
    if (sensor) { sensor->update(roadBorders, traffic); }

    // 2. Determina Controles
    bool intendedTurnLeft = false, intendedTurnRight = false, setControlsFromNN = false;
    desiredAcceleration = 0.0f;
    if (controlType == ControlType::KEYS) { controls.update(); }
    else if (useBrain && brain && sensor) {
        std::vector<float> sensorOffsets(sensor->rayCount);
        for (size_t i = 0; i < sensor->readings.size(); ++i) { sensorOffsets[i] = sensor->readings[i] ? (1.0f - sensor->readings[i]->offset) : 0.0f; }
        std::vector<float> outputs = NeuralNetwork::feedForward(sensorOffsets, *brain);
        if (outputs.size() >= 4) {
             if (outputs[0] > 0.55f) desiredAcceleration = acceleration; else if (outputs[0] < 0.45f) desiredAcceleration = -acceleration * 1.5f; else desiredAcceleration = 0.0f;
             intendedTurnLeft = (outputs[1] > 0.5f); intendedTurnRight = (outputs[2] > 0.5f); setControlsFromNN = true;
        }
        controls.forward = false; controls.reverse = false;
    }
    else if (controlType == ControlType::DUMMY) { controls.forward = true; controls.reverse = false; controls.left = false; controls.right = false; }

    // 3. Lógica de Following Agressivo
    if (controlType == ControlType::AI && sensor) {
        float currentDistanceAhead = calculateMinDistanceAhead(traffic); bool distanceValid = currentDistanceAhead < std::numeric_limits<float>::max();
        if (distanceValid && previousDistanceAhead >= 0) { float distanceDelta = currentDistanceAhead - previousDistanceAhead; if (std::abs(distanceDelta) < AGGRESSIVE_DELTA_THRESHOLD) aggressiveDeltaFrames++; else aggressiveDeltaFrames = 0; } else aggressiveDeltaFrames = 0;
        if (aggressiveDeltaFrames >= AGGRESSIVE_DELTA_DURATION_THRESHOLD) { std::cout << "WARN: Car " << this << " damaged - AGGRESSIVE following." << std::endl; damaged = true; aggressiveDeltaFrames = 0; return; }
        previousDistanceAhead = distanceValid ? currentDistanceAhead : -1.0f;
    } else { aggressiveDeltaFrames = 0; previousDistanceAhead = -1.0f; }

    // 4. Lógica Anti-Pêndulo
    if (setControlsFromNN) { int turn = 0; if(intendedTurnLeft && !intendedTurnRight) turn = -1; else if(intendedTurnRight && !intendedTurnLeft) turn = 1; bool osc = (turn != 0 && turn == -lastTurnDirection); if(osc) { controls.left=false; controls.right=false;} else { controls.left=intendedTurnLeft; controls.right=intendedTurnRight;} lastTurnDirection=turn; }
    else if (controlType == ControlType::KEYS) { int turn = 0; if(controls.left && !controls.right) turn = -1; else if(controls.right && !controls.left) turn = 1; lastTurnDirection=turn; }
    else { lastTurnDirection = 0; }

    // 5. Move o Carro
    move();

    // 6. Atualiza Score (Distância Líquida Percorrida)
    if (controlType == ControlType::AI) {
        float deltaY = previousYPosition - position.y; // Y diminui -> deltaY positivo -> para frente
        netDistanceTraveled += deltaY;
    }

    // 7. Lógica de Dano por Estar Parado
    if (controlType == ControlType::AI) {
        if (std::abs(speed) < STOPPED_SPEED_THRESHOLD) stoppedFrames++; else stoppedFrames = 0;
        if (stoppedFrames >= STOPPED_DURATION_THRESHOLD) { std::cout << "WARN: Car " << this << " damaged - STOPPED." << std::endl; damaged = true; stoppedFrames = 0; return; }
    } else { stoppedFrames = 0; }

    // 8. Lógica de Dano por Marcha à Ré
    if (controlType == ControlType::AI) {
        if (speed < 0.0f) reversingFrames++; else reversingFrames = 0;
        if (reversingFrames >= REVERSE_DURATION_THRESHOLD) { std::cout << "WARN: Car " << this << " damaged - REVERSING." << std::endl; damaged = true; reversingFrames = 0; return; }
    } else { reversingFrames = 0; }

    // Atualiza Posição Anterior para o PRÓXIMO frame
    previousYPosition = position.y;

    // 9. Avalia Dano por Colisão
    if (!damaged) { damaged = assessDamage(roadBorders, traffic); }

} // Fim de Car::update

// move (sem alterações)
void Car::move() {
    float currentAppliedAccel = 0.0f; if (!damaged) { if (controlType == ControlType::AI) currentAppliedAccel = desiredAcceleration; else { if (controls.forward) currentAppliedAccel += acceleration; if (controls.reverse) currentAppliedAccel -= acceleration; } }
    lastAppliedAcceleration = currentAppliedAccel; speed += currentAppliedAccel;
    if (std::abs(speed) > friction) speed -= friction * std::copysign(1.0f, speed); else speed = 0.0f;
    speed = std::clamp(speed, -maxSpeed / 2.0f, maxSpeed);
    if (std::abs(speed) > 1e-5 && !damaged) { float flip = (speed > 0.0f) ? 1.0f : -1.0f; float turnRate = 0.03f; if (controls.left) angle -= turnRate * flip; if (controls.right) angle += turnRate * flip; }
    position.x -= std::sin(angle) * speed; position.y -= std::cos(angle) * speed;
}

// getPolygon (sem alterações)
std::vector<sf::Vector2f> Car::getPolygon() const {
    std::vector<sf::Vector2f> points(4); const float rad = std::hypot(width, height) / 2.0f; const float alpha = std::atan2(width, height);
    float angle_minus_alpha = angle - alpha; float angle_plus_alpha = angle + alpha; float pi = static_cast<float>(M_PI);
    points[0] = { position.x - std::sin(angle_minus_alpha) * rad, position.y - std::cos(angle_minus_alpha) * rad };
    points[1] = { position.x - std::sin(angle_plus_alpha) * rad,  position.y - std::cos(angle_plus_alpha) * rad };
    points[2] = { position.x - std::sin(pi + angle_minus_alpha) * rad, position.y - std::cos(pi + angle_minus_alpha) * rad };
    points[3] = { position.x - std::sin(pi + angle_plus_alpha) * rad,  position.y - std::cos(pi + angle_plus_alpha) * rad };
    return points;
}

// assessDamage (sem alterações)
bool Car::assessDamage(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders, const std::vector<Car*>& traffic) {
    if (damaged) return true; std::vector<sf::Vector2f> carPoly = getPolygon();
    for (const auto& border : roadBorders) { for (size_t i = 0; i < carPoly.size(); ++i) { if (getIntersection(carPoly[i], carPoly[(i + 1) % carPoly.size()], border.first, border.second)) return true; } }
    for (const Car* otherCarPtr : traffic) { if (otherCarPtr == nullptr || otherCarPtr == this) continue; if (polysIntersect(carPoly, otherCarPtr->getPolygon())) return true; }
    return false;
}

// draw (sem alterações)
void Car::draw(sf::RenderTarget& target, bool drawSensorFlag) {
     sf::Color drawColorToUse = color; if (damaged) drawColorToUse = sf::Color(150, 150, 150, 180);
     if (textureLoaded) { sprite.setColor(drawColorToUse); sprite.setPosition(position); sprite.setRotation(sf::degrees(angle * 180.0f / static_cast<float>(M_PI))); target.draw(sprite); }
     else { sf::RectangleShape fallbackRect({width, height}); fallbackRect.setOrigin({width / 2.0f, height / 2.0f}); fallbackRect.setPosition(position); fallbackRect.setRotation(sf::degrees(angle * 180.0f / static_cast<float>(M_PI))); sf::Color fallbackColor = damaged ? drawColorToUse : color; if (!damaged) { switch (controlType) { case ControlType::DUMMY: fallbackColor = sf::Color::Red; break; case ControlType::AI: fallbackColor = sf::Color(0, 0, 255, 180); break; case ControlType::KEYS: fallbackColor = sf::Color::Blue; break; default: fallbackColor = sf::Color::White; break; } } fallbackRect.setFillColor(fallbackColor); target.draw(fallbackRect); }
     if (sensor && drawSensorFlag && !damaged) sensor->draw(target);
 }