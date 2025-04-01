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
    m_dummySpeed(0.f)  // inicializa 0
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
}

Car::~Car() {
    delete m_controls;
    delete m_sensor;
}

void Car::update(Road* road) {
    if (!m_damaged) {
        move(road);
        createPolygon();
        m_damaged = assessDamage(road->getBorders());
    }
    // Atualiza sensor (se existir)
    if (m_sensor) {
        m_sensor->update(road->getBorders(), {}); 
        // Se quiser, passe a lista de Car* para enxergar também carros.
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
        m_sensor->update(roadBorders, otherCars);
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
            m_speed += (1.2f * m_acceleration * deltaTime * 60.f);
        }
        if (m_controls->reverse) {
            m_speed -= (1.2f * m_acceleration * deltaTime * 60.f);
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
            shape.setFillColor(sf::Color::Green);
        } else {
            shape.setFillColor(sf::Color::Red);
        }
    }
    target.draw(shape, states);

    if (m_sensor) {
        m_sensor->draw(target);
    }
}