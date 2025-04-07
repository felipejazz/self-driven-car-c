#include "controls.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <iostream>

Controls::Controls()
    : forward(false),
      left(false),
      right(false),
      reverse(false)
{

}


void Controls::setKeyPressed(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::Up:
            forward = true;  
            break;
        case sf::Keyboard::Key::Down:
            reverse = true;  
            break;
        case sf::Keyboard::Key::Left:
            left = true;     
            break;
        case sf::Keyboard::Key::Right:
            right = true;    
            break;
        default:
            break;
    }
}


void Controls::setKeyReleased(sf::Keyboard::Key key) {
    switch(key) {
        case sf::Keyboard::Key::Up:
            forward = false;  break;
        case sf::Keyboard::Key::Down:
            reverse = false;  break;
        case sf::Keyboard::Key::Left:
            left = false;     break;
        case sf::Keyboard::Key::Right:
            right = false;    break;
        default:
            break;
    }
}
