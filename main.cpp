#include "game.hpp"
#include "controls.hpp"
int main() {
    Game game(3, ControlType::KEYS);
    game.run();
    return 0;
}
