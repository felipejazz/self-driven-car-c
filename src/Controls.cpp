#include "Controls.hpp"

Controls::Controls(ControlType t) : type(t) {
    if (type == ControlType::DUMMY) {
        forward = true;
    }
    // brake já é inicializado como false por padrão
}

void Controls::update() {
    if (type == ControlType::KEYS) {
        forward = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
        reverse = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
        left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
        right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
        brake = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space); // <-- ADICIONADO: Verifica a tecla Space
    }
    // Para AI e DUMMY, o estado é definido em outro lugar.
}