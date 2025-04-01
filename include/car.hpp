#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "controls.hpp"
#include "sensor.hpp"

class Road;

class Car : public sf::Drawable {
private:
    float m_x;
    float m_y;
    float m_width;
    float m_height;

    float m_speed;
    float m_acceleration;
    float m_maxSpeed;
    float m_angle;

    bool m_damaged;
    bool m_isAi;
    std::vector<sf::Vector2f> m_polygon;

    Controls* m_controls;
    Sensor* m_sensor;

    void move(Road* road);
    bool assessDamage(const std::vector<std::vector<sf::Vector2f>>& roadBorders);
    void createPolygon();
    

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

public:
    Car(int x, int y, ControlType ctype);
    ~Car();

    void update(Road* road);
    void updateMovement(float deltaTime, float friction, bool hitBoundary);
    float getSpeed() const;

    float m_dummySpeed;
    ControlType m_controlType;

    float getX() const { return m_x; }
    float getY() const { return m_y; }
    float getAngle() const { return m_angle; }
    Controls* getControls() const { return m_controls; }
    ControlType getControlType() const {
        return m_controlType;
    }
    void updateSensor(
        const std::vector<std::vector<sf::Vector2f>>& roadBorders,
        const std::vector<Car*>& otherCars
    );
    

    void stop();
    void setDamage();
    const std::vector<sf::Vector2f>& getPolygon() const;
    bool isAi()       const { return (m_controlType == ControlType::AI); }
    bool isDummy()    const { return (m_controlType == ControlType::DUMMY); }
    bool isKeyboard() const { return (m_controlType == ControlType::KEYS); }
    friend class Sensor;
};
