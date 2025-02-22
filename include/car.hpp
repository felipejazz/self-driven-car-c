#ifndef CAR_HPP
#define CAR_HPP

#include <SFML/Graphics.hpp>

class Car : public sf::RectangleShape {
public:
    
    Car();      
    double speed;           
    void updateMovement();
    void render();
};

#endif 
