#pragma once
#include <SFML/Window/Keyboard.hpp>


enum class ControlType {
    KEYS,
    AI,
    DUMMY
};


class Controls {
public:
    bool forward;
    bool left;
    bool right;
    bool reverse;

    Controls();
    void setKeyPressed(sf::Keyboard::Key key);
    void setKeyReleased(sf::Keyboard::Key key);
};
