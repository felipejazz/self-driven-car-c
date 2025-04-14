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
#include <cstdint>
#include <functional>

// --- Constructor ---
Car::Car(float x, float y, float w, float h, ControlType type, float maxSpd, sf::Color col)
    : position(x, y),
      width(w), height(h), maxSpeed(maxSpd), acceleration(0.2f),
      brakePower(acceleration * 2.0f), friction(0.05f), controlType(type),
      controls(type), color(col), textureLoaded(false), sprite(texture),
      desiredAcceleration(0.0f), lastAppliedAcceleration(0.0f),
      stoppedTimer(0.0f), reversingTimer(0.0f),
      previousYPosition(y),
      previousAngle(0.0f),
      currentFitness(0.0f),
      previousLaneIndex(-1),
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
        sprite.setOrigin({textureRect.size.x / 2.0f, textureRect.size.y / 2.0f});
        sprite.setColor(color);
        textureLoaded = true;
        return;
    }

    sprite.setScale({1.0f, 1.0f});
    sprite.setOrigin({textureRect.size.x / 2.0f, textureRect.size.y / 2.0f});
    sprite.setColor(color);
    textureLoaded = true;
    return;
}

int Car::getSensorRayCount() const { return sensor ? static_cast<int>(sensor->rayCount) : 0; }

void Car::resetForNewGeneration(float startY, const Road& road) {
    position = { road.getLaneCenter(1), startY };
    angle = 0.0f;
    speed = 0.0f;
    damaged = false;
    currentFitness = 0.0f;
    previousYPosition = startY;
    previousAngle = 0.0f;
    stoppedTimer = 0.0f;
    reversingTimer = 0.0f;
    desiredAcceleration = 0.0f;
    lastAppliedAcceleration = 0.0f;
    stuckCheckTimer = 0.0f;
    stuckCheckStartY = startY;
    passedObstacleIDs.clear();
    previousLaneIndex = getCurrentLaneIndex(road);
}


float Car::calculateDesiredAcceleration(std::vector<float> output){
    if (output[0] > 0.5f) {
        return acceleration;
    }
    if (output[1] > 0.5f) {
       return -acceleration;
    }
    return 0.0f;
}

void Car::updateBasedOnControls(Controls control) {
    if (control.type == ControlType::KEYS) {
        this->controls.update();
        desiredAcceleration = 0.0f;
        if(this->controls.forward) desiredAcceleration += acceleration;
        if(this->controls.reverse) desiredAcceleration -= acceleration;
        return;
    }
    
    if (control.type == ControlType::DUMMY) {
        this->controls.forward = true;
        this->controls.reverse = false;
        this->controls.left = false;
        this->controls.right = false;
        desiredAcceleration = acceleration;
        return;
    }
    if (control.type == ControlType::AI && sensor && brain) {
        std::vector<float> sensorOffsets(sensor->rayCount);
        for (size_t i = 0; i < sensor->rayCount; ++i) {
             sensorOffsets[i] = (i < sensor->readings.size() && sensor->readings[i])
                                ? (1.0f - sensor->readings[i]->offset) : 0.0f;
        }
        std::vector<float> outputs = NeuralNetwork::feedForward(sensorOffsets, *brain);

        if (outputs.size() == 4) {
            desiredAcceleration = calculateDesiredAcceleration(outputs);
            this->controls.forward = outputs[0] > 0.5f;
            this->controls.reverse = outputs[1] > 0.5f;
            this->controls.left = outputs[2] > 0.5f;
            this->controls.right = outputs[3] > 0.5f;
            return;
        }
        std::cerr << "Warning: AI output size mismatch (" << outputs.size() << " instead of 4)." << std::endl;
        this->controls.forward = this->controls.reverse = this->controls.left = this->controls.right = false;
        desiredAcceleration = 0.0f;
        return;
    }

    this->controls.forward = this->controls.reverse = this->controls.left = this->controls.right = false;
    desiredAcceleration = 0.0f;
    return;
}


void Car::update(const Road& road, const std::vector<Obstacle*>& obstacles, sf::Time deltaTime) {
    bool wasAlreadyDamaged = damaged;
    if (wasAlreadyDamaged) {
        speed = 0;
        return;
    }

    // 1. Update Sensor
    if (sensor) { sensor->update(road.borders, obstacles); }

    // 2. Define controls
    updateBasedOnControls(this->controls);

    // 3. Move car
    move(0.0f, deltaTime);

    // --- Lane change main logic ---
    int currentLaneIndex = getCurrentLaneIndex(road);
    if (previousLaneIndex == -1 && currentLaneIndex != -1) {
        previousLaneIndex = currentLaneIndex;
    } else if (currentLaneIndex != -1 && previousLaneIndex != -1 && currentLaneIndex != previousLaneIndex) {
        currentFitness += FITNESS_LANE_CHANGE_BONUS;
        previousLaneIndex = currentLaneIndex;
    } else if (currentLaneIndex != -1) {
        previousLaneIndex = currentLaneIndex;
    }


    // 4. Update Overtaken status
    updateOvertakeStatus(obstacles);

    // 5. Update Fitness
    float deltaY = previousYPosition - position.y;
    currentFitness += deltaY;
    currentFitness += FITNESS_FRAME_SURVIVAL_REWARD;
    if (speed > FITNESS_SPEED_REWARD_THRESHOLD) {
        currentFitness += FITNESS_SPEED_REWARD;
    }

    // 6. Penalize if car moves spinning
    float dtSeconds = deltaTime.asSeconds();
    if (dtSeconds > 1e-6) {
        float deltaAngle = angle - previousAngle;
        deltaAngle = std::fmod(deltaAngle + M_PI, 2.0 * M_PI);
        if (deltaAngle < 0.0) deltaAngle += 2.0 * M_PI;
        deltaAngle -= M_PI;

        float angleRate = std::abs(deltaAngle) / dtSeconds;
        float forwardSpeedY = deltaY / dtSeconds;

        if (angleRate > SPINNING_ANGLE_THRESHOLD_RAD_PER_SEC && forwardSpeedY < SPINNING_MIN_Y_MOVEMENT_PER_SEC) {
            currentFitness -= FITNESS_SPINNING_PENALTY;
            // std::cout << "Spinning detected! Penalty: " << FITNESS_SPINNING_PENALTY << std::endl; // Debug
        }
    }

    // 7. Update previous state (IMPORTANT do this before check damage)
    previousYPosition = position.y;
    previousAngle = angle;

    // 8. Verify problematic states (Stop, reverse and sutck) and apply penaliaties
    checkStoppedStatus(deltaTime);
    if (damaged && !wasAlreadyDamaged) { currentFitness -= FITNESS_STOPPED_PENALTY; return; }

    checkReversingStatus(deltaTime);
     if (damaged && !wasAlreadyDamaged) { currentFitness -= FITNESS_REVERSING_PENALTY; return; }

    checkStuckStatus(deltaTime);
     if (damaged && !wasAlreadyDamaged) { currentFitness -= FITNESS_STUCK_PENALTY; return; }

    // 9. Verify final colision
    Obstacle* hitObstacle = nullptr;
    bool collisionOccurred = checkForCollision(road.borders, obstacles, hitObstacle);
    if (collisionOccurred) {
         if (!damaged) {
            damaged = true;
            currentFitness -= FITNESS_COLLISION_PENALTY;
            if (hitObstacle && passedObstacleIDs.find(hitObstacle->id) == passedObstacleIDs.end()) {
                currentFitness -= FITNESS_FAIL_OVERTAKE_PENALTY;
            }
         }
        speed = 0;
        return; 
    }
}

int Car::getCurrentLaneIndex(const Road& road) const {
    if (road.laneCount <= 0) {
        return -1;
    }
    if (position.x < road.left || position.x > road.right) {
        return -1;
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
            damaged = true;
        }
        stuckCheckTimer = 0.0f;
        stuckCheckStartY = position.y;
    }
}

void Car::move(float aiBrakeSignal, sf::Time deltaTime) {
    float dtSeconds = deltaTime.asSeconds();
    if (dtSeconds <= 0) return;
    const float timeScaleFactor = dtSeconds * 60.0f;

    float currentActualAcceleration = 0.0f;

    if (controls.forward) {
        currentActualAcceleration += acceleration;
    }
    if (controls.reverse) {
        currentActualAcceleration -= acceleration;
    }

    speed += currentActualAcceleration * timeScaleFactor;


    float frictionForce = friction * timeScaleFactor;
    if (std::abs(speed) > 1e-5f) {
        if (std::abs(speed) > frictionForce) {
            speed -= frictionForce * std::copysign(1.0f, speed);
        } else {
            speed = 0.0f;
        }
    }

    speed = std::clamp(speed, -maxSpeed * (2.0f / 3.0f), maxSpeed);

    if (std::abs(speed) > 1e-5f) {
        float flip = (speed > 0.0f) ? 1.0f : -1.0f;
        float turnRateRad = 0.03f * timeScaleFactor;
        if (controls.left) angle -= turnRateRad * flip;
        if (controls.right) angle += turnRateRad * flip;
    }

    position.x += std::sin(angle) * speed * timeScaleFactor;
    position.y -= std::cos(angle) * speed * timeScaleFactor;

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

void Car::checkReversingStatus(sf::Time deltaTime) {
    if (damaged || controlType != ControlType::AI) { reversingTimer = 0.0f; return; }

    bool wantsToGoBackward = desiredAcceleration < -1e-5;
    bool isMovingBackward = speed < -STOPPED_SPEED_THRESHOLD;

    if (wantsToGoBackward && isMovingBackward) {
         reversingTimer += deltaTime.asSeconds();
    } else {
        reversingTimer = 0.0f;
    }

    if (reversingTimer >= REVERSE_TIME_THRESHOLD_SECONDS) {
        damaged = true;
    }
}

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


std::vector<sf::Vector2f> Car::getPolygon() const {
     std::vector<sf::Vector2f> points(4);
     const float rad = std::hypot(width, height) / 2.0f;
     const float alpha = std::atan2(width, height);

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


bool Car::checkForCollision(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                           const std::vector<Obstacle*>& obstacles,
                           Obstacle*& hitObstacle)
{
    hitObstacle = nullptr;
    std::vector<sf::Vector2f> carPoly = getPolygon();

    for (const auto& border : roadBorders) {
        for (size_t i = 0; i < carPoly.size(); ++i) {
            if (getIntersection(carPoly[i], carPoly[(i + 1) % carPoly.size()], border.first, border.second)) {
                return true;
            }
        }
    }
    for (Obstacle* obsPtr : obstacles) {
        if (!obsPtr) continue;
        if (polysIntersect(carPoly, obsPtr->getPolygon())) {
            hitObstacle = obsPtr;
            return true;
        }
    }
    return false;
}

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
        sprite.setRotation(sf::degrees(angle));
        target.draw(sprite);
    } else {
        sf::RectangleShape fallbackRect({width, height});
        fallbackRect.setOrigin({width / 2.0f, height / 2.0f});
        fallbackRect.setPosition(position);
        fallbackRect.setRotation(sf::degrees(angle));
        fallbackRect.setFillColor(drawColorToUse);
        fallbackRect.setOutlineColor(sf::Color::Black);
        fallbackRect.setOutlineThickness(1);
        target.draw(fallbackRect);
    }

    if (sensor && drawSensorFlag) {
        sensor->draw(target);
    }
}