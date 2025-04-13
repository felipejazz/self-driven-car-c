// src/Car.cpp

#include "Car.hpp"
#include "Utils.hpp"
#include "Obstacle.hpp"
#include "Road.hpp"
#include <iostream>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <limits>
#include <algorithm>
#include <cmath>
#include <deque>
#include <unordered_set>
#include <cstdint> // Include for uint8_t

// --- Construtor ---
Car::Car(float x, float y, float w, float h, ControlType type, float maxSpd, sf::Color col)
    : position(x, y),
      width(w), height(h), maxSpeed(maxSpd), acceleration(0.2f),
      brakePower(acceleration * 2.0f), friction(0.05f), controlType(type),
      controls(type), color(col), textureLoaded(false), sprite(texture),
      desiredAcceleration(0.0f), lastAppliedAcceleration(0.0f),
      stoppedTimer(0.0f), reversingTimer(0.0f),
      previousYPosition(y),
      currentFitness(0.0f),
      stuckCheckTimer(0.0f),
      stuckCheckStartY(y)
{
    useBrain = (controlType == ControlType::AI);
    loadTexture("assets/car.png");
    if (!textureLoaded) {
        // std::cerr << "Warning: Failed to load car texture (assets/car.png)." << std::endl;
    }

    if (controlType != ControlType::DUMMY) {
        sensor = std::make_unique<Sensor>(*this);
        if (useBrain && sensor) {
            int rayCountValue = sensor->rayCount;
            if (rayCountValue <= 0) {
                 std::cerr << "Error: Sensor created with 0 rays for AI car!" << std::endl;
                 rayCountValue = 5; // Fallback
            }
            const std::vector<int> networkStructure = {rayCountValue, 12, 4};
            brain.emplace(NeuralNetwork(networkStructure));
        }
    }
}

// --- loadTexture ---
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
        return;
    }
    sprite.setOrigin({textureRect.size.x / 2.0f, textureRect.size.y / 2.0f});
    sprite.setColor(color);
    textureLoaded = true;
}

// --- Getters ---
int Car::getSensorRayCount() const { return sensor ? static_cast<int>(sensor->rayCount) : 0; }

// --- resetForNewGeneration ---
void Car::resetForNewGeneration(float startY, const Road& road) {
    position = { road.getLaneCenter(1), startY };
    angle = 0.0f;
    speed = 0.0f;
    damaged = false;
    currentFitness = 0.0f;
    previousYPosition = startY;
    stoppedTimer = 0.0f;
    reversingTimer = 0.0f;
    desiredAcceleration = 0.0f;
    lastAppliedAcceleration = 0.0f;
    stuckCheckTimer = 0.0f;
    stuckCheckStartY = startY;
    passedObstacleIDs.clear();
}


float Car::calculateDesiredAcceleration(std::vector<float> output){
    if (output[0] > 0.5f) { // Acelerar
        return acceleration;
    }
    if (output[1] > 0.5f) { // Frear
       return -acceleration;
    }
    return .0f; // Parar
}

void Car::updateBasedOnControls(Controls control) {
    if (control.type == ControlType::KEYS) {
        controls.update(); // Atualiza baseado no teclado
        if(controls.forward) desiredAcceleration = acceleration;
        if(controls.reverse) desiredAcceleration = -acceleration;
        return;
    }
    else if (control.type == ControlType::DUMMY) {
        controls.forward = true; // Sempre acelera
        controls.reverse = false;
        controls.left = false;
        controls.right = false;
        return;
    }
    if (control.type == ControlType::AI) {
        std::vector<float> sensorOffsets(sensor->rayCount);
        for (size_t i = 0; i < sensor->rayCount; ++i) {
             sensorOffsets[i] = (i < sensor->readings.size() && sensor->readings[i])
                                ? (1.0f - sensor->readings[i]->offset) : 0.0f;
        }
        std::vector<float> outputs = NeuralNetwork::feedForward(sensorOffsets, *brain);
        if (outputs.size() == 4) {
            float desiredAcceleration = calculateDesiredAcceleration(outputs);
            controls.forward = desiredAcceleration > 0.0f; // Acelerar
            controls.reverse = desiredAcceleration < 0.0f; // Frear
            controls.left = (outputs[2] > 0.5f); // Virar esquerda
            controls.right = (outputs[3] > 0.5f); // Virar direita
            return;
        }
    }
    return;
}


// --- Update ---
void Car::update(const Road& road, const std::vector<Obstacle*>& obstacles, sf::Time deltaTime) {
    bool wasAlreadyDamaged = damaged; // Guarda o estado ANTES das checagens de status
    
    if (wasAlreadyDamaged) {
        speed = 0; // Garante que carros já danificados não se movam
        return;    // Sai do update se já começou danificado
    }

    // 1. Update Sensor (se existir)
    if (sensor) { sensor->update(road.borders, obstacles); }

    updateBasedOnControls(controls); // Atualiza controles

    // 4. Move Car
    move(0.0f, deltaTime); // O parâmetro aiBrakeSignal não está sendo usado aqui

    // 5. Update Overtake Status (adiciona bônus se passou por um obstáculo)
    updateOvertakeStatus(obstacles);

    // 6. Update Fitness (distância, sobrevivência por frame, recompensa por velocidade)
    float deltaY = previousYPosition - position.y; // Y diminui ao avançar
    currentFitness += deltaY; // Recompensa principal por avançar
    currentFitness += FITNESS_FRAME_SURVIVAL_REWARD; // Pequena recompensa por sobreviver mais um frame
    if (speed > FITNESS_SPEED_REWARD_THRESHOLD) {
        currentFitness += FITNESS_SPEED_REWARD; // Recompensa extra por estar rápido
    }
    previousYPosition = position.y; // Atualiza a posição Y anterior para o próximo frame

    // 7. Checagens de Status (Parado, Ré, Preso) & Aplica Penalidades se danificado NESTE frame

    // Checa se ficou parado tempo demais
    checkStoppedStatus(deltaTime);
    if (damaged && !wasAlreadyDamaged) { // Se ficou danificado AGORA por estar parado
        currentFitness -= FITNESS_STOPPED_PENALTY;
        // std::cout << "Car " << this << " penalized for stopping." << std::endl; // Debug
        return; // Sai do update
    }

    // Checa se ficou em marcha à ré tempo demais
    checkReversingStatus(deltaTime);
     if (damaged && !wasAlreadyDamaged) { // Se ficou danificado AGORA por estar de ré
        currentFitness -= FITNESS_REVERSING_PENALTY;
        // std::cout << "Car " << this << " penalized for reversing." << std::endl; // Debug
        return; // Sai do update
    }

    // --- NOVA CHECAGEM ---
    // Checa se ficou preso (sem avançar o suficiente)
    checkStuckStatus(deltaTime);
     if (damaged && !wasAlreadyDamaged) { // Se ficou danificado AGORA por estar preso
        currentFitness -= FITNESS_STUCK_PENALTY;
        // std::cout << "Car " << this << " penalized for being stuck." << std::endl; // Debug
        return; // Sai do update
    }
    // --- FIM NOVA CHECAGEM ---


    // 8. Checa Colisão & Aplica Penalidades (se colidiu NESTE frame)
    Obstacle* hitObstacle = nullptr;
    bool collisionOccurred = checkForCollision(road.borders, obstacles, hitObstacle);

    if (collisionOccurred) {
         // Somente aplica penalidade de colisão se não foi danificado *antes* da colisão neste frame
         if (!damaged) { // Evita penalizar duas vezes se já foi danificado por tempo/stuck
            damaged = true; // Marca como danificado pela colisão
            currentFitness -= FITNESS_COLLISION_PENALTY; // Penalidade base por colisão

            if (hitObstacle) { // Colisão com obstáculo específico
                // Se o obstáculo atingido NÃO estava na lista de ultrapassados
                if (passedObstacleIDs.find(hitObstacle->id) == passedObstacleIDs.end()) {
                    currentFitness -= FITNESS_FAIL_OVERTAKE_PENALTY; // Penalidade extra por falhar na ultrapassagem
                    // std::cout << "Car " << this << " penalized for hitting obstacle " << hitObstacle->id << " before passing." << std::endl; // Debug
                } else {
                    // std::cout << "Car " << this << " hit obstacle " << hitObstacle->id << " AFTER passing." << std::endl; // Debug opcional
                }
            }
         }
        speed = 0; // Para o carro na colisão
         // Não precisa de 'return' aqui, pois a condição !damaged no início do loop já trata isso no próximo frame
    }
} // Fim Car::update

// --- checkStuckStatus ---
void Car::checkStuckStatus(sf::Time deltaTime) {
    if (damaged || controlType != ControlType::AI) { // Só checa AI não danificados
        stuckCheckTimer = 0.0f; // Reseta timer se não aplicável
        stuckCheckStartY = position.y; // Atualiza posição inicial da checagem
        return;
    }

    stuckCheckTimer += deltaTime.asSeconds();

    // Se o tempo da checagem foi atingido
    if (stuckCheckTimer >= STUCK_TIME_THRESHOLD_SECONDS) {
        // Calcula o quanto o carro avançou (Y diminui para frente)
        float distanceMovedForwardY = stuckCheckStartY - position.y;

        // Se moveu menos que o limite para frente (ou até moveu para trás)
        if (distanceMovedForwardY < STUCK_DISTANCE_Y_THRESHOLD) {
             // std::cout << "Car " << this << " detectado como preso! DistY: " << distanceMovedForwardY << std::endl; // Debug
            damaged = true; // Marca como danificado
        }

        // Reseta para a próxima checagem, independentemente de ter ficado preso ou não
        stuckCheckTimer = 0.0f;
        stuckCheckStartY = position.y;
    }
}
// --- move ---
void Car::move(float aiBrakeSignal, sf::Time deltaTime) { // aiBrakeSignal não parece ser usado aqui
    float dtSeconds = deltaTime.asSeconds();
    if (dtSeconds <= 0) return;
    const float timeScaleFactor = dtSeconds * 60.0f;

    // --- MODIFICAÇÃO AQUI ---
    // Calcule a aceleração baseada nos controles booleanos
    float currentAcceleration = 0.0f;
    if (controls.forward) {
        currentAcceleration += acceleration;
    }
    if (controls.reverse) {
        // Pode ser freio ou ré. Se a IA só usa para frear, talvez só subtraia.
        // Se pode dar ré, subtrai a aceleração. Vamos assumir que pode dar ré.
        currentAcceleration -= acceleration; // Ou use -brakePower se for só freio
    }
    // --- FIM DA MODIFICAÇÃO ---

    // Aplica a aceleração calculada
    speed += currentAcceleration * timeScaleFactor; // Usa a aceleração calculada dos controles

    // Aplica atrito (como estava antes)
    float frictionForce = friction * timeScaleFactor;
    if (std::abs(speed) > 1e-5f) {
        if (std::abs(speed) > frictionForce) {
            speed -= frictionForce * std::copysign(1.0f, speed);
        } else {
            speed = 0.0f;
        }
    }

    // Limita a velocidade (como estava antes)
    speed = std::clamp(speed, -maxSpeed * (2.0f / 3.0f), maxSpeed);

    // Aplica a direção (como estava antes)
    if (std::abs(speed) > 1e-5f) {
        float flip = (speed > 0.0f) ? 1.0f : -1.0f;
        float turnRateRad = 0.03f * timeScaleFactor;
        if (controls.left) angle -= turnRateRad * flip;
        if (controls.right) angle += turnRateRad * flip;
    }

    // Atualiza a posição (como estava antes)
    position.x += std::sin(angle) * speed * timeScaleFactor;
    position.y -= std::cos(angle) * speed * timeScaleFactor;

    // Mantenha lastAppliedAcceleration se precisar para debug ou outra lógica
    lastAppliedAcceleration = currentAcceleration;
}


// --- checkStoppedStatus ---
void Car::checkStoppedStatus(sf::Time deltaTime) {
    if (damaged || controlType != ControlType::AI) { stoppedTimer = 0.0f; return; }
    if (std::abs(speed) < STOPPED_SPEED_THRESHOLD) { stoppedTimer += deltaTime.asSeconds(); }
    else { stoppedTimer = 0.0f; }
    if (stoppedTimer >= STOPPED_TIME_THRESHOLD_SECONDS) {
        damaged = true;
    }
}

// --- checkReversingStatus ---
void Car::checkReversingStatus(sf::Time deltaTime) {
    if (damaged || controlType != ControlType::AI) { reversingTimer = 0.0f; return; }
    bool isReversingEffectively = (speed < -0.05f) && (desiredAcceleration < 0.0f);
    if (isReversingEffectively) { reversingTimer += deltaTime.asSeconds(); }
    else { reversingTimer = 0.0f; }
    if (reversingTimer >= REVERSE_TIME_THRESHOLD_SECONDS) {
        damaged = true;
    }
}

// --- updateOvertakeStatus ---
void Car::updateOvertakeStatus(const std::vector<Obstacle*>& obstacles) {
    float carFrontY = position.y - height / 2.0f;
    for (const auto* obsPtr : obstacles) {
        if (!obsPtr || passedObstacleIDs.count(obsPtr->id)) continue;
        float obstacleRearY = obsPtr->position.y + obsPtr->height / 2.0f;
        if (carFrontY < obstacleRearY) {
            passedObstacleIDs.insert(obsPtr->id);
            currentFitness += FITNESS_OVERTAKE_BONUS;
        }
    }
}

// --- getPolygon ---
std::vector<sf::Vector2f> Car::getPolygon() const {
    // (Implementation unchanged from previous correct version)
     std::vector<sf::Vector2f> points(4);
     const float rad = std::hypot(width, height) / 2.0f;
     const float alpha = std::atan2(width, height);
     sf::Vector2f rel_top_right     = {  rad * std::sin(alpha), -rad * std::cos(alpha) };
     sf::Vector2f rel_bottom_right  = {  rad * std::sin(-alpha), -rad * std::cos(-alpha) };
     sf::Vector2f rel_bottom_left   = {  rad * std::sin(static_cast<float>(M_PI) + alpha), -rad * std::cos(static_cast<float>(M_PI) + alpha) };
     sf::Vector2f rel_top_left      = {  rad * std::sin(static_cast<float>(M_PI) - alpha), -rad * std::cos(static_cast<float>(M_PI) - alpha) };
     float sinA = std::sin(angle);
     float cosA = std::cos(angle);
     points[0] = position + sf::Vector2f(rel_top_right.x * cosA - rel_top_right.y * sinA, rel_top_right.x * sinA + rel_top_right.y * cosA);
     points[1] = position + sf::Vector2f(rel_bottom_right.x * cosA - rel_bottom_right.y * sinA, rel_bottom_right.x * sinA + rel_bottom_right.y * cosA);
     points[2] = position + sf::Vector2f(rel_bottom_left.x * cosA - rel_bottom_left.y * sinA, rel_bottom_left.x * sinA + rel_bottom_left.y * cosA);
     points[3] = position + sf::Vector2f(rel_top_left.x * cosA - rel_top_left.y * sinA, rel_top_left.x * sinA + rel_top_left.y * cosA);
     return points;
}


// --- checkForCollision ---
bool Car::checkForCollision(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                           const std::vector<Obstacle*>& obstacles,
                           Obstacle*& hitObstacle)
{
    hitObstacle = nullptr;
    std::vector<sf::Vector2f> carPoly = getPolygon();

    // Collision with borders
    for (const auto& border : roadBorders) {
        for (size_t i = 0; i < carPoly.size(); ++i) {
            if (getIntersection(carPoly[i], carPoly[(i + 1) % carPoly.size()], border.first, border.second)) {
                return true;
            }
        }
    }
    // Collision with obstacles
    for (Obstacle* obsPtr : obstacles) {
        if (!obsPtr) continue;
        if (polysIntersect(carPoly, obsPtr->getPolygon())) {
            hitObstacle = obsPtr;
            return true;
        }
    }
    return false;
}

// --- draw ---
void Car::draw(sf::RenderTarget& target, bool drawSensorFlag) {
    sf::Color drawColorToUse = color;
    if (damaged) {
        // CORRECTION: sf::Uint8 -> uint8_t
        drawColorToUse.r = static_cast<uint8_t>(std::max(0, drawColorToUse.r - 100));
        drawColorToUse.g = static_cast<uint8_t>(std::max(0, drawColorToUse.g - 100));
        drawColorToUse.b = static_cast<uint8_t>(std::max(0, drawColorToUse.b - 100));
        drawColorToUse.a = 180;
    }

    if (textureLoaded) {
        sprite.setColor(drawColorToUse);
        sprite.setPosition(position);
        // Assuming 'angle' is in radians, convert to degrees for SFML's setRotation
        sprite.setRotation(sf::degrees(angle) * 180.0f / static_cast<float>(M_PI)); // Convert radians to degrees
        target.draw(sprite);
    } else {
        sf::RectangleShape fallbackRect({width, height});
        fallbackRect.setOrigin({width / 2.0f, height / 2.0f});
        fallbackRect.setPosition(position);
        // Assuming 'angle' is in radians, convert to degrees for SFML's setRotation
        fallbackRect.setRotation(sf::degrees(angle) * 180.0f / static_cast<float>(M_PI)); // Convert radians to degrees
        fallbackRect.setFillColor(drawColorToUse);
        fallbackRect.setOutlineColor(sf::Color::Black);
        fallbackRect.setOutlineThickness(1);
        target.draw(fallbackRect);
    }

    if (sensor && drawSensorFlag) {
        sensor->draw(target);
    }
}