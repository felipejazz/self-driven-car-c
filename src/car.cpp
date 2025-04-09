#include "car.hpp"
#include "util.hpp"
#include "road.hpp"
#include <cmath>
#include <algorithm>
#include <random> 

Car::Car(int x, int y, ControlType ctype)
  : m_x(static_cast<float>(x)),
    m_y(static_cast<float>(y)),
    m_width(30.f),
    m_height(50.f),
    m_speed(0.f),
    m_acceleration(0.2f),
    m_maxSpeed(3.f),
    m_angle(0.f),
    m_damaged(false),
    m_controlType(ctype),
    m_dummySpeed(0.f),
    m_brain(nullptr),
    m_fitness(0.f),
    m_stuckTime(0.f),
    m_lastY(static_cast<float>(y)),
    m_followingPenalty(0.f),
    m_isBest(false)
{
    m_controls = new Controls();
    m_sensor   = new Sensor(this);
    createPolygon();

    if (m_controlType == ControlType::DUMMY) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(m_maxSpeed / 2.0f, m_maxSpeed);
        m_dummySpeed = dist(gen);
    }

    if (m_controlType == ControlType::AI) {
        m_brain = new NeuralNetwork({5, 6, 4});
    }
}

Car::~Car() {
    delete m_controls;
    delete m_sensor;
    delete m_brain;
}

void Car::update(Road* road) {
    if (!m_damaged) {
        if (m_controlType == ControlType::AI && m_brain && m_sensor) {
            // Get sensor readings
            const auto& readings = m_sensor->getReadings();
            
            // Prepare inputs for neural network
            Eigen::VectorXd inputs(static_cast<int>(m_sensor->getRayCount()));
            for (size_t i = 0; i < readings.size(); i++) {
                // Convert sensor readings to normalized distances (0 to 1)
                if (readings[i].x < 0) {
                    // No obstacle detected
                    inputs(i) = 1.0;
                } else {
                    // Calculate distance and normalize
                    float distance = std::hypot(
                        readings[i].x - m_x,
                        readings[i].y - m_y
                    );
                    inputs(i) = distance / m_sensor->getRayLength();
                }
            }
            
            // Feed forward through neural network
            Eigen::VectorXd outputs = m_brain->feedForward(inputs);
            
            // Apply outputs to controls
            m_controls->forward = outputs(0) > 0.5;
            m_controls->left = outputs(1) > 0.5;
            m_controls->right = outputs(2) > 0.5;
            m_controls->reverse = outputs(3) > 0.5;
        }
        
        move(road);
        createPolygon();
        m_damaged = assessDamage(road->getBorders());
    }
    
    // Update sensor
    if (m_sensor) {
        m_sensor->update(road->getBorders(), {});
    }
}

void Car::setBrain(NeuralNetwork* brain) {
    delete m_brain;
    m_brain = brain;
}

void Car::updateFitness() {
    m_fitness = -m_y;
    
    m_fitness -= m_followingPenalty;

    if (m_damaged) {
        m_fitness *= 0.5f;
    }
}

void Car::checkIfStuck(float deltaTime) {
    float movementThreshold = 0.5f;
    float yDiff = std::abs(m_y - m_lastY);
    
    if (yDiff < movementThreshold) {
        m_stuckTime += deltaTime;
    } else {
        m_stuckTime = 0.f;
    }
    
    m_lastY = m_y;
}

void Car::checkIfFollowing(const std::vector<Car*>& cars) {
    if (m_damaged) return;
    
    // Check if there's a car directly in front
    float followingDistance = 100.0f;
    float laneWidth = 50.0f;
    
    for (const auto& otherCar : cars) {
        if (otherCar == this || otherCar->isDummy()) continue;
        
        // Check if other car is in front (lower Y)
        if (otherCar->getY() < m_y && 
            otherCar->getY() > m_y - followingDistance &&
            std::abs(otherCar->getX() - m_x) < laneWidth) {
            
            // Apply a penalty based on how close the car is following
            float distance = m_y - otherCar->getY();
            float penalty = (followingDistance - distance) * 2.0f;
            m_followingPenalty += penalty * 0.01f;
            return;
        }
    }
}

void Car::setDamage() {
    m_damaged = true;
}

void Car::stop() {
    m_speed = 0.f;
}

void Car::updateSensor(
    const std::vector<std::vector<sf::Vector2f>>& roadBorders,
    const std::vector<Car*>& otherCars
) {
    if (m_sensor) {
        // Filter out AI cars from sensor detection if this is an AI car
        std::vector<Car*> filteredCars;
        if (m_controlType == ControlType::AI) {
            for (const auto& car : otherCars) {
                if (car != this && !car->isAi()) {
                    filteredCars.push_back(car);
                }
            }
            m_sensor->update(roadBorders, filteredCars);
        } else {
            m_sensor->update(roadBorders, otherCars);
        }
    }
}


void Car::updateMovement(float deltaTime, float friction, bool hitBoundary) {
    if (m_damaged) {
        m_speed = 0.f;
        return;
    }
    if (hitBoundary) {
        m_speed = 0.f;
        return;
    }

    if (m_controlType == ControlType::KEYS) {
        // Teclado
        if (m_controls->forward) {
            m_speed += (1.7f * m_acceleration * deltaTime * 60.f);
        }
        if (m_controls->reverse) {
            m_speed -= (1.7f * m_acceleration * deltaTime * 60.f);
        }
        float maxSpeed = 2.0f * m_maxSpeed; 
        m_speed = std::clamp(m_speed, -maxSpeed / 2.f, maxSpeed);
    }
    else if (m_controlType == ControlType::AI) {
        m_speed += (m_acceleration * deltaTime * 45.f);
        m_speed = std::clamp(m_speed, -m_maxSpeed / 2.f, m_maxSpeed);
    }
    else if (m_controlType == ControlType::DUMMY) {
        if (m_speed < m_dummySpeed) {
            m_speed += (m_acceleration * deltaTime * 45.f);
            if (m_speed > m_dummySpeed) m_speed = m_dummySpeed;
        }
    }

    float totalFriction = friction * (deltaTime * 60.f);
    if (m_speed > 0.f) {
        m_speed -= totalFriction;
        m_speed = std::max(m_speed, 0.f);
    }
    else if (m_speed < 0.f) {
        m_speed += totalFriction;
        m_speed = std::min(m_speed, 0.f);
    }
    if (std::fabs(m_speed) > 0.001f) {
        float flip = (m_speed > 0.f) ? 1.f : -1.f;
        if (m_controlType == ControlType::KEYS) {
            if (m_controls->left) {
                m_angle += 0.03f * flip * (deltaTime * 60.f);
            }
            if (m_controls->right) {
                m_angle -= 0.03f * flip * (deltaTime * 60.f);
            }
        }
    }

    float moveDist = m_speed * (deltaTime * 60.f);
    m_x -= std::sin(m_angle) * moveDist;
    m_y -= std::cos(m_angle) * moveDist;

    createPolygon();
}

float Car::getSpeed() const {
    return m_speed;
}

bool Car::assessDamage(const std::vector<std::vector<sf::Vector2f>>& roadBorders) {
    for (const auto& border : roadBorders) {
        if (polysIntersect(m_polygon, border)) {
            return true;
        }
    }
    return false;
}

const std::vector<sf::Vector2f>& Car::getPolygon() const {
    return m_polygon;
}
void Car::createPolygon() {
    m_polygon.clear();
    float rad = std::hypot(m_width, m_height) / 2.f;
    float alpha = std::atan2(m_width, m_height);

    sf::Vector2f p1{
        m_x - std::sin(m_angle - alpha) * rad,
        m_y - std::cos(m_angle - alpha) * rad
    };
    sf::Vector2f p2{
        m_x - std::sin(m_angle + alpha) * rad,
        m_y - std::cos(m_angle + alpha) * rad
    };
    sf::Vector2f p3{
        m_x - std::sin((float)M_PI + m_angle - alpha) * rad,
        m_y - std::cos((float)M_PI + m_angle - alpha) * rad
    };
    sf::Vector2f p4{
        m_x - std::sin((float)M_PI + m_angle + alpha) * rad,
        m_y - std::cos((float)M_PI + m_angle + alpha) * rad
    };

    m_polygon.push_back(p1);
    m_polygon.push_back(p2);
    m_polygon.push_back(p3);
    m_polygon.push_back(p4);
}

void Car::move(Road* road) {
    if (m_controls->forward) m_speed += m_acceleration;
    if (m_controls->reverse) m_speed -= m_acceleration;

    if (m_speed > m_maxSpeed) m_speed = m_maxSpeed;
    if (m_speed < -m_maxSpeed/2.f) m_speed = -m_maxSpeed/2.f;

    float friction = road->getFriction();
    if (m_speed > 0.f) {
        m_speed -= friction;
        if (m_speed < 0.f) m_speed = 0.f;
    } else if (m_speed < 0.f) {
        m_speed += friction;
        if (m_speed > 0.f) m_speed = 0.f;
    }

    if (std::fabs(m_speed) > 0.0001f) {
        float flip = (m_speed > 0.f) ? 1.f : -1.f;
        if (m_controls->left)  m_angle += 0.03f * flip;
        if (m_controls->right) m_angle -= 0.03f * flip;
    }

    m_x -= std::sin(m_angle) * m_speed;
    m_y -= std::cos(m_angle) * m_speed;
}

void Car::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    sf::ConvexShape shape;
    shape.setPointCount(m_polygon.size());
    for (unsigned int i = 0; i < m_polygon.size(); i++) {
        shape.setPoint(i, m_polygon[i]);
    }

    if (m_damaged) {
        shape.setFillColor(sf::Color(100, 100, 100));
    } else {
        if (m_controlType == ControlType::KEYS) {
            shape.setFillColor(sf::Color::Blue);
        } else if (m_controlType == ControlType::AI) {
            if (m_isBest) {
                // Highlight the best car
                shape.setFillColor(sf::Color::Yellow);
            } else {
                shape.setFillColor(sf::Color::Green);
            }
        } else {
            shape.setFillColor(sf::Color::Red);
        }
    }
    target.draw(shape, states);

    if (m_sensor) {
        m_sensor->draw(target);
    }
}