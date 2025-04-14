#ifndef CAR_HPP
#define CAR_HPP

#include "Controls.hpp"
#include "Sensor.hpp"
#include "Network.hpp"
#include "Obstacle.hpp"
#include "Road.hpp"
#include <SFML/Graphics.hpp>
#include <optional>
#include <memory>
#include <vector>
#include <deque>
#include <string>
#include <unordered_set>
#include <cstdint>
#include <cmath>

class Car {
public:
    // --- Public Member ---
    sf::Vector2f position;
    float width;
    float height;
    float angle = 0.0f;
    std::optional<NeuralNetwork> brain;
    bool useBrain = false;

    // --- Constructor ---
    Car(float x, float y, float w, float h, ControlType type = ControlType::AI, float maxSpd = 3.0f, sf::Color col = sf::Color::Blue);

    void update(const Road& road, const std::vector<Obstacle*>& obstacles, sf::Time deltaTime);
    void draw(sf::RenderTarget& target, bool drawSensorFlag = false);
    std::vector<sf::Vector2f> getPolygon() const;
    bool isDamaged() const { return damaged; }
    float getFitness() const { return currentFitness; }
    float getSpeed() const { return speed; }
    int getSensorRayCount() const;
    void resetForNewGeneration(float startY, const Road& road);

private:
    // Physics and Control
     float speed = 0.0f;
     float acceleration = 0.2f;
     float brakePower;
     float maxSpeed;
     float friction = 0.05f;
     bool damaged = false;
     ControlType controlType;
     Controls controls;
     std::unique_ptr<Sensor> sensor;
     sf::Color color;
     sf::Texture texture;
     sf::Sprite sprite;
     bool textureLoaded;

    void updateBasedOnControls(Controls controls);

    // AI Logic and states
    float desiredAcceleration = 0.0f;
    float lastAppliedAcceleration = 0.0f;
    float stoppedTimer = 0.0f;
    float reversingTimer = 0.0f;
    float stuckCheckTimer = 0.0f;
    float stuckCheckStartY = 0.0f;

    // Previous variables 
    float previousYPosition = 0.0f;
    float previousAngle = 0.0f;
    int previousLaneIndex = -1;

    // Fitness logics
    float currentFitness = 0.0f;
    std::unordered_set<long long> passedObstacleIDs;

    // Auxiliary methods
    void loadTexture(const std::string& filename);
    void move(float aiBrakeSignal, sf::Time deltaTime);
    void checkStoppedStatus(sf::Time deltaTime);
    void checkReversingStatus(sf::Time deltaTime);
    void checkStuckStatus(sf::Time deltaTime);
    void updateOvertakeStatus(const std::vector<Obstacle*>& obstacles);
    bool checkForCollision(const std::vector<std::pair<sf::Vector2f, sf::Vector2f>>& roadBorders,
                             const std::vector<Obstacle*>& obstacles,
                             Obstacle*& hitObstacle);
    float calculateDesiredAcceleration(std::vector<float> outputs);
    int getCurrentLaneIndex(const Road& road) const;

    // CONSTANTS TO DO MOVE TO MAIN HEADER CLASS
    static constexpr float FITNESS_SPINNING_PENALTY = 1.0f;
    static constexpr float FITNESS_LANE_CHANGE_BONUS = 25.0f;
    static constexpr float FITNESS_SPEED_REWARD_THRESHOLD = 1.5f;
    static constexpr float FITNESS_FRAME_SURVIVAL_REWARD = 0.05f;
    static constexpr float FITNESS_SPEED_REWARD = 0.1f;
    static constexpr float FITNESS_OVERTAKE_BONUS = 50.0f;
    static constexpr float FITNESS_STOPPED_PENALTY = 50.0f;
    static constexpr float FITNESS_REVERSING_PENALTY = 50.0f;
    static constexpr float FITNESS_STUCK_PENALTY = 60.0f;
    static constexpr float FITNESS_FAIL_OVERTAKE_PENALTY = 75.0f;
    static constexpr float FITNESS_COLLISION_PENALTY = 100.0f;

    // Spinning car controls
    static constexpr float SPINNING_ANGLE_THRESHOLD_RAD_PER_SEC = M_PI;
    static constexpr float SPINNING_MIN_Y_MOVEMENT_PER_SEC = 2.0f;

    static constexpr float STOPPED_SPEED_THRESHOLD = 0.05f;
    static constexpr float STOPPED_TIME_THRESHOLD_SECONDS = 5.0f;
    static constexpr float REVERSE_TIME_THRESHOLD_SECONDS = 2.0f;
    static constexpr float STUCK_DISTANCE_Y_THRESHOLD = 5.0f;
    static constexpr float STUCK_TIME_THRESHOLD_SECONDS = 1.5f;
};

#endif // CAR_HPP