#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#include <SFML/Window/Keyboard.hpp>

enum class ControlType {
    KEYS,   // Manual keyboard control
    DUMMY,  // Always moves forward
    AI      // Controlled by neural network
};

class Controls {
public:
    bool forward = false;
    bool left = false;
    bool right = false;
    bool reverse = false;
    ControlType type;

    Controls(ControlType t = ControlType::KEYS);

    // Update controls based on keyboard input (only if type is KEYS)
    void update();
};

#endif // CONTROLS_HPP