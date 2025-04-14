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
#include <functional> // Para std::bind e placeholders (se necessário para callbacks)

// --- Construtor ---
Car::Car(float x, float y, float w, float h, ControlType type, float maxSpd, sf::Color col)
    : position(x, y),
      width(w), height(h), maxSpeed(maxSpd), acceleration(0.2f),
      brakePower(acceleration * 2.0f), friction(0.05f), controlType(type),
      controls(type), color(col), textureLoaded(false), sprite(texture),
      desiredAcceleration(0.0f), lastAppliedAcceleration(0.0f),
      stoppedTimer(0.0f), reversingTimer(0.0f),
      previousYPosition(y),
      previousAngle(0.0f), // Inicializa ângulo anterior
      currentFitness(0.0f),
      previousLaneIndex(-1), // Inicializa índice da faixa anterior
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
    // Nota: previousLaneIndex será inicializado corretamente em resetForNewGeneration
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

int Car::getSensorRayCount() const { return sensor ? static_cast<int>(sensor->rayCount) : 0; }

void Car::resetForNewGeneration(float startY, const Road& road) {
    position = { road.getLaneCenter(1), startY }; // Começa na faixa 1
    angle = 0.0f;
    speed = 0.0f;
    damaged = false;
    currentFitness = 0.0f;
    previousYPosition = startY;
    previousAngle = 0.0f; // Reseta ângulo anterior
    stoppedTimer = 0.0f;
    reversingTimer = 0.0f;
    desiredAcceleration = 0.0f;
    lastAppliedAcceleration = 0.0f;
    stuckCheckTimer = 0.0f;
    stuckCheckStartY = startY;
    passedObstacleIDs.clear();
    previousLaneIndex = getCurrentLaneIndex(road); // Reseta índice da faixa anterior
}


float Car::calculateDesiredAcceleration(std::vector<float> output){
    if (output[0] > 0.5f) {
        return acceleration; // Acelerar para frente
    }
    if (output[1] > 0.5f) {
       return -acceleration; // Frear/Ré
    }
    return 0.0f; // Nenhuma aceleração/freio
}

void Car::updateBasedOnControls(Controls control) {
    // Se for controlado por teclas, atualiza o estado interno 'this->controls'
    if (control.type == ControlType::KEYS) {
        this->controls.update(); // Atualiza o estado de 'this->controls' baseado no teclado
        // desiredAcceleration é usado apenas para a lógica de ré, não para aplicar aceleração direta
        desiredAcceleration = 0.0f;
        if(this->controls.forward) desiredAcceleration += acceleration;
        if(this->controls.reverse) desiredAcceleration -= acceleration;
        return; // Sai, pois 'this->controls' já foi atualizado
    }
    // Se for DUMMY, define o estado interno 'this->controls' e sai
    else if (control.type == ControlType::DUMMY) {
        this->controls.forward = true;
        this->controls.reverse = false;
        this->controls.left = false;
        this->controls.right = false;
        desiredAcceleration = acceleration; // Define aceleração desejada (para lógica de ré, se aplicável)
        return;
    }
    // Se for AI, usa a rede neural para definir o estado interno 'this->controls'
    if (control.type == ControlType::AI && sensor && brain) {
        std::vector<float> sensorOffsets(sensor->rayCount);
        for (size_t i = 0; i < sensor->rayCount; ++i) {
             sensorOffsets[i] = (i < sensor->readings.size() && sensor->readings[i])
                                ? (1.0f - sensor->readings[i]->offset) : 0.0f;
        }
        std::vector<float> outputs = NeuralNetwork::feedForward(sensorOffsets, *brain);

        if (outputs.size() == 4) {
            // Calcula aceleração desejada pela AI (usado na lógica de ré)
            desiredAcceleration = calculateDesiredAcceleration(outputs);
            // Atualiza os controles internos do carro
            this->controls.forward = outputs[0] > 0.5f;
            this->controls.reverse = outputs[1] > 0.5f;
            this->controls.left = outputs[2] > 0.5f;
            this->controls.right = outputs[3] > 0.5f;
            return;
        } else {
            std::cerr << "Warning: AI output size mismatch (" << outputs.size() << " instead of 4)." << std::endl;
            // Reseta controles internos para segurança
            this->controls.forward = this->controls.reverse = this->controls.left = this->controls.right = false;
            desiredAcceleration = 0.0f;
        }
    }
    // Caso contrário (ex: AI sem cérebro), reseta controles internos
    else {
        this->controls.forward = this->controls.reverse = this->controls.left = this->controls.right = false;
        desiredAcceleration = 0.0f;
    }
}


// --- Update ---
void Car::update(const Road& road, const std::vector<Obstacle*>& obstacles, sf::Time deltaTime) {
    bool wasAlreadyDamaged = damaged;
    if (wasAlreadyDamaged) {
        speed = 0;
        return;
    }

    // 1. Atualiza Sensor
    if (sensor) { sensor->update(road.borders, obstacles); }

    // 2. Define Controles (Atualiza o estado de 'this->controls')
    updateBasedOnControls(this->controls);

    // 3. Move o Carro (Usa o estado de 'this->controls' para aplicar física)
    move(0.0f, deltaTime);

    // --- Lógica de Mudança de Faixa ---
    int currentLaneIndex = getCurrentLaneIndex(road);
    if (previousLaneIndex == -1 && currentLaneIndex != -1) {
        previousLaneIndex = currentLaneIndex; // Inicializa na primeira faixa válida
    } else if (currentLaneIndex != -1 && previousLaneIndex != -1 && currentLaneIndex != previousLaneIndex) {
        currentFitness += FITNESS_LANE_CHANGE_BONUS;
        previousLaneIndex = currentLaneIndex; // Atualiza para a nova faixa
    } else if (currentLaneIndex != -1) {
        previousLaneIndex = currentLaneIndex; // Mantém atualizado mesmo sem mudança
    }
    // ---------------------------------

    // 4. Atualiza Status de Ultrapassagem
    updateOvertakeStatus(obstacles);

    // 5. Atualiza Fitness Básico (Progresso, Sobrevivência, Velocidade)
    float deltaY = previousYPosition - position.y; // Progresso para frente
    currentFitness += deltaY;
    currentFitness += FITNESS_FRAME_SURVIVAL_REWARD;
    if (speed > FITNESS_SPEED_REWARD_THRESHOLD) {
        currentFitness += FITNESS_SPEED_REWARD;
    }

    // 6. Penalidade por Girar no Lugar
    float dtSeconds = deltaTime.asSeconds();
    if (dtSeconds > 1e-6) {
        float deltaAngle = angle - previousAngle;
        // Normaliza deltaAngle para o intervalo [-PI, PI]
        deltaAngle = std::fmod(deltaAngle + M_PI, 2.0 * M_PI);
        if (deltaAngle < 0.0) deltaAngle += 2.0 * M_PI;
        deltaAngle -= M_PI;

        float angleRate = std::abs(deltaAngle) / dtSeconds; // Radianos por segundo
        float forwardSpeedY = deltaY / dtSeconds; // Unidades por segundo

        // Aplica penalidade se girar muito rápido com pouco avanço
        if (angleRate > SPINNING_ANGLE_THRESHOLD_RAD_PER_SEC && forwardSpeedY < SPINNING_MIN_Y_MOVEMENT_PER_SEC) {
            currentFitness -= FITNESS_SPINNING_PENALTY;
            // std::cout << "Spinning detected! Penalty: " << FITNESS_SPINNING_PENALTY << std::endl; // Debug
        }
    }

    // 7. Atualiza Estado Anterior (Importante fazer ANTES das checagens de dano)
    previousYPosition = position.y;
    previousAngle = angle;

    // 8. Verifica Status Problemáticos (Parado, Ré, Preso) e aplica dano/penalidade se ocorrer
    checkStoppedStatus(deltaTime);
    if (damaged && !wasAlreadyDamaged) { currentFitness -= FITNESS_STOPPED_PENALTY; return; }

    checkReversingStatus(deltaTime);
     if (damaged && !wasAlreadyDamaged) { currentFitness -= FITNESS_REVERSING_PENALTY; return; }

    checkStuckStatus(deltaTime);
     if (damaged && !wasAlreadyDamaged) { currentFitness -= FITNESS_STUCK_PENALTY; return; }

    // 9. Verifica Colisão Final
    Obstacle* hitObstacle = nullptr;
    bool collisionOccurred = checkForCollision(road.borders, obstacles, hitObstacle);
    if (collisionOccurred) {
         if (!damaged) { // Aplica dano e penalidades apenas na primeira vez
            damaged = true;
            currentFitness -= FITNESS_COLLISION_PENALTY;
            if (hitObstacle && passedObstacleIDs.find(hitObstacle->id) == passedObstacleIDs.end()) {
                currentFitness -= FITNESS_FAIL_OVERTAKE_PENALTY;
            }
         }
        speed = 0; // Para o carro na colisão
        return; // Sai do update
    }
} // Fim de Car::update


// --- Implementação do método auxiliar ---
int Car::getCurrentLaneIndex(const Road& road) const {
    if (road.laneCount <= 0) {
        return -1; // Caso inválido
    }
    if (position.x < road.left || position.x > road.right) {
        return -1; // Fora da estrada
    }
    const float laneWidth = road.width / static_cast<float>(road.laneCount);
    int laneIndex = static_cast<int>((position.x - road.left) / laneWidth);
    return std::max(0, std::min(laneIndex, road.laneCount - 1)); // Garante que está no intervalo [0, N-1]
}


void Car::checkStuckStatus(sf::Time deltaTime) {
    if (damaged || controlType != ControlType::AI) {
        stuckCheckTimer = 0.0f;
        stuckCheckStartY = position.y;
        return;
    }
    stuckCheckTimer += deltaTime.asSeconds();
    if (stuckCheckTimer >= STUCK_TIME_THRESHOLD_SECONDS) {
        float distanceMovedForwardY = stuckCheckStartY - position.y;
        if (distanceMovedForwardY < STUCK_DISTANCE_Y_THRESHOLD) {
            damaged = true; // Marca como danificado se não moveu o suficiente
        }
        stuckCheckTimer = 0.0f; // Reseta para a próxima verificação
        stuckCheckStartY = position.y;
    }
}

// --- move ---
void Car::move(float aiBrakeSignal, sf::Time deltaTime) { // aiBrakeSignal não está sendo usado aqui
    float dtSeconds = deltaTime.asSeconds();
    if (dtSeconds <= 0) return;
    const float timeScaleFactor = dtSeconds * 60.0f; // Ajuste para manter consistência

    float currentActualAcceleration = 0.0f;

    // Aplica aceleração baseado no estado de 'this->controls'
    if (controls.forward) {
        currentActualAcceleration += acceleration;
    }
    if (controls.reverse) {
        currentActualAcceleration -= acceleration; // Aceleração reversa
    }

    // Atualiza velocidade
    speed += currentActualAcceleration * timeScaleFactor;

    // Aplica atrito
    float frictionForce = friction * timeScaleFactor;
    if (std::abs(speed) > 1e-5f) {
        if (std::abs(speed) > frictionForce) {
            speed -= frictionForce * std::copysign(1.0f, speed);
        } else {
            speed = 0.0f;
        }
    }

    // Limita velocidade
    speed = std::clamp(speed, -maxSpeed * (2.0f / 3.0f), maxSpeed);

    // Aplica giro baseado no estado de 'this->controls'
    if (std::abs(speed) > 1e-5f) {
        float flip = (speed > 0.0f) ? 1.0f : -1.0f;
        float turnRateRad = 0.03f * timeScaleFactor;
        // Ajuste: Assumindo que ângulo positivo é para a direita (sentido horário)
        // e ângulo negativo é para a esquerda (anti-horário). Confirme isso!
        // Se 'angle += ...' vira para a esquerda, mantenha.
        if (controls.left) angle -= turnRateRad * flip;  // Esquerda: diminui ângulo
        if (controls.right) angle += turnRateRad * flip; // Direita: aumenta ângulo
    }

    // Atualiza posição
    position.x += std::sin(angle) * speed * timeScaleFactor;
    position.y -= std::cos(angle) * speed * timeScaleFactor; // Y negativo é para cima

    lastAppliedAcceleration = currentActualAcceleration;
}


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

    // Usa o 'desiredAcceleration' calculado pela AI (ou controle manual) para intenção
    bool wantsToGoBackward = desiredAcceleration < -1e-5; // Tem intenção de ir para trás?
    bool isMovingBackward = speed < -STOPPED_SPEED_THRESHOLD; // Está de fato se movendo para trás?

    // Penaliza se o carro está ativamente tentando e conseguindo ir de ré por muito tempo
    if (wantsToGoBackward && isMovingBackward) {
         reversingTimer += deltaTime.asSeconds();
    } else {
        reversingTimer = 0.0f; // Reseta se a condição não for mantida
    }

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
     std::vector<sf::Vector2f> points(4);
     const float rad = std::hypot(width, height) / 2.0f;
     const float alpha = std::atan2(width, height);
     // Usar M_PI de <cmath>
     sf::Vector2f rel_top_right     = {  rad * std::sin(alpha), -rad * std::cos(alpha) };
     sf::Vector2f rel_bottom_right  = {  rad * std::sin(-alpha), -rad * std::cos(-alpha) };
     sf::Vector2f rel_bottom_left   = {  rad * std::sin(static_cast<float>(M_PI) + alpha), -rad * std::cos(static_cast<float>(M_PI) + alpha) };
     sf::Vector2f rel_top_left      = {  rad * std::sin(static_cast<float>(M_PI) - alpha), -rad * std::cos(static_cast<float>(M_PI) - alpha) };

     float sinA = std::sin(angle);
     float cosA = std::cos(angle);

     auto rotateAndTranslate = [&](const sf::Vector2f& rel) {
         return position + sf::Vector2f(rel.x * cosA - rel.y * sinA, rel.x * sinA + rel.y * cosA);
     };

     points[0] = rotateAndTranslate(rel_top_right);
     points[1] = rotateAndTranslate(rel_bottom_right);
     points[2] = rotateAndTranslate(rel_bottom_left);
     points[3] = rotateAndTranslate(rel_top_left);
     return points;
}


// --- checkForCollision ---
bool Car::checkForCollision(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                           const std::vector<Obstacle*>& obstacles,
                           Obstacle*& hitObstacle)
{
    hitObstacle = nullptr;
    std::vector<sf::Vector2f> carPoly = getPolygon();

    // Colisão com bordas
    for (const auto& border : roadBorders) {
        for (size_t i = 0; i < carPoly.size(); ++i) {
            if (getIntersection(carPoly[i], carPoly[(i + 1) % carPoly.size()], border.first, border.second)) {
                return true;
            }
        }
    }
    // Colisão com obstáculos
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
        drawColorToUse.r = static_cast<uint8_t>(std::max(0, static_cast<int>(drawColorToUse.r) - 100));
        drawColorToUse.g = static_cast<uint8_t>(std::max(0, static_cast<int>(drawColorToUse.g) - 100));
        drawColorToUse.b = static_cast<uint8_t>(std::max(0, static_cast<int>(drawColorToUse.b) - 100));
        drawColorToUse.a = 180;
    }

    if (textureLoaded) {
        sprite.setColor(drawColorToUse);
        sprite.setPosition(position);
        // Usa sf::degrees para converter radianos para graus que SFML espera
        sprite.setRotation(sf::degrees(angle));
        target.draw(sprite);
    } else {
        sf::RectangleShape fallbackRect({width, height});
        fallbackRect.setOrigin({width / 2.0f, height / 2.0f});
        fallbackRect.setPosition(position);
        fallbackRect.setRotation(sf::degrees(angle)); // Usa sf::degrees
        fallbackRect.setFillColor(drawColorToUse);
        fallbackRect.setOutlineColor(sf::Color::Black);
        fallbackRect.setOutlineThickness(1);
        target.draw(fallbackRect);
    }

    if (sensor && drawSensorFlag) {
        sensor->draw(target);
    }
}