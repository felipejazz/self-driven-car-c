#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#include <SFML/Window/Keyboard.hpp>

enum class ControlType {
    KEYS,
    DUMMY,
    AI
};

class Controls {
public:
    bool forward = false;
    bool left = false;
    bool right = false;
    bool reverse = false;
    ControlType type;

    Controls(ControlType t = ControlType::KEYS);
    void update();
};

#endif