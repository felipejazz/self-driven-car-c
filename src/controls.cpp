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
            std::cout << "Key Pressed: Up" << std::endl;
            forward = true;  
            break;
        case sf::Keyboard::Key::Down:
        std::cout << "Key Pressed: Down" << std::endl;
            reverse = true;  
            break;
        case sf::Keyboard::Key::Left:
            std::cout << "Key Pressed: Left" << std::endl;
            left = true;     
            break;
        case sf::Keyboard::Key::Right:
            std::cout << "Key Pressed: Right" << std::endl;
            right = true;    
            break;
        default:
            break;
    }
}


void Controls::setKeyReleased(sf::Keyboard::Key key) {
    switch(key) {
        case sf::Keyboard::Key::Up:
            std::cout << "Key Released: Up" << std::endl;
            forward = false;  break;
        case sf::Keyboard::Key::Down:
            std::cout << "Key Released: Down" << std::endl;
            reverse = false;  break;
        case sf::Keyboard::Key::Left:
            std::cout << "Key Released: Left" << std::endl;
            left = false;     break;
        case sf::Keyboard::Key::Right:
            std::cout << "Key Released: Right" << std::endl;
            right = false;    break;
        default:
            break;
    }
}
