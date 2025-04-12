#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include <SFML/Graphics.hpp>
#include "Network.hpp" // Include Network definition

class Visualizer {
public:
    // Static method to draw the neural network
    static void drawNetwork(sf::RenderTarget& ctx, const NeuralNetwork& network, const sf::Font& font,
                            float x, float y, float width, float height);

private:
    // Static helper to draw a single level
    static void drawLevel(sf::RenderTarget& ctx, const Level& level, const sf::Font& font,
                          float left, float top, float width, float height,
                          const std::vector<std::string>& outputLabels);

    // Static helper to get node x-coordinate
    static float getNodeX(int nodeCount, int index, float left, float right);
};

#endif // VISUALIZER_HPP