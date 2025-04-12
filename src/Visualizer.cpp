#include "Visualizer.hpp"
#include "Utils.hpp" // For lerp, getValueColor
#include <string>   // For std::to_string
#include <iostream> // For debug

void Visualizer::drawNetwork(sf::RenderTarget& ctx, const NeuralNetwork& network, const sf::Font& font,
                             float x, float y, float width, float height)
{
    const float margin = 50.0f;
    const float left = x + margin;
    const float top = y + margin;
    const float drawWidth = width - margin * 2;
    const float drawHeight = height - margin * 2;

    if (network.levels.empty()) return;

    const float levelHeight = drawHeight / static_cast<float>(network.levels.size());

    for (int i = static_cast<int>(network.levels.size()) - 1; i >= 0; --i) { // Cast para int
        const float levelTop = top + lerp(
            drawHeight - levelHeight,
            0,
            network.levels.size() == 1 ? 0.5f : static_cast<float>(i) / (network.levels.size() - 1)
        );

        std::vector<std::string> labels;
        if (i == 0) {
             labels = {"F", "L", "R", "B"};
        }

        Visualizer::drawLevel(ctx, network.levels[i], font,
                              left, levelTop, drawWidth, levelHeight, labels);
    }
}

void Visualizer::drawLevel(sf::RenderTarget& ctx, const Level& level, const sf::Font& font,
                           float left, float top, float width, float height,
                           const std::vector<std::string>& outputLabels)
{
    const float right = left + width;
    const float bottom = top + height;

    const auto& inputs = level.inputs;
    const auto& outputs = level.outputs;
    const auto& weights = level.weights;
    const auto& biases = level.biases;

    const float nodeRadius = 18.0f;
    const float lineWidth = 2.0f; // Note: lineWidth in SFML VertexArray Lines is not directly settable
    const float nodeOutlineThickness = 2.0f;

    // --- Draw connections (weights) ---
    sf::VertexArray lines(sf::PrimitiveType::Lines);
    for (size_t i = 0; i < inputs.size(); ++i) {
        float inputX = getNodeX(inputs.size(), i, left, right);
        for (size_t j = 0; j < outputs.size(); ++j) {
             float outputX = getNodeX(outputs.size(), j, left, right);
             sf::Color lineColor = getValueColor(weights[i][j]); // Color based on weight value

             // *** ADEQUAÇÃO DA SINTAXE ***
             // Use a inicialização por chaves {} para sf::Vertex
             // e construa sf::Vector2f explicitamente DENTRO das chaves.
             lines.append(sf::Vertex{sf::Vector2f(inputX, bottom), lineColor}); // Ponto inicial
             lines.append(sf::Vertex{sf::Vector2f(outputX, top), lineColor});   // Ponto final
        }
    }
     if (lines.getVertexCount() > 0) {
         ctx.draw(lines);
     }

    // --- Draw Input Nodes (at the bottom) ---
    for (size_t i = 0; i < inputs.size(); ++i) {
        float x = getNodeX(inputs.size(), i, left, right);

        sf::CircleShape nodeBg(nodeRadius);
        nodeBg.setFillColor(sf::Color::Black);
        nodeBg.setOrigin({nodeRadius, nodeRadius});
        nodeBg.setPosition({x, bottom});
        ctx.draw(nodeBg);

        sf::CircleShape nodeValue(nodeRadius * 0.6f);
        nodeValue.setFillColor(getValueColor(inputs[i]));
        nodeValue.setOrigin({nodeRadius * 0.6f, nodeRadius * 0.6f});
        nodeValue.setPosition({x, bottom});
        ctx.draw(nodeValue);
    }

    // --- Draw Output Nodes (at the top) ---
    for (size_t i = 0; i < outputs.size(); ++i) {
        float x = getNodeX(outputs.size(), i, left, right);

        sf::CircleShape nodeBg(nodeRadius);
        nodeBg.setFillColor(sf::Color::Black);
        nodeBg.setOrigin({nodeRadius, nodeRadius});
        nodeBg.setPosition({x, top});
        ctx.draw(nodeBg);

        sf::CircleShape nodeValue(nodeRadius * 0.6f);
        nodeValue.setFillColor(getValueColor(outputs[i]));
        nodeValue.setOrigin({nodeRadius * 0.6f, nodeRadius * 0.6f});
        nodeValue.setPosition({x, top});
        ctx.draw(nodeValue);

        sf::CircleShape biasCircle(nodeRadius * 0.8f);
        biasCircle.setOutlineThickness(nodeOutlineThickness);
        biasCircle.setOutlineColor(getValueColor(biases[i]));
        biasCircle.setFillColor(sf::Color::Transparent);
        biasCircle.setOrigin({nodeRadius * 0.8f, nodeRadius * 0.8f});
        biasCircle.setPosition({x, top});
        ctx.draw(biasCircle);

        if (i < outputLabels.size() && !outputLabels[i].empty()) {
            sf::Text labelText(font, outputLabels[i], static_cast<unsigned int>(nodeRadius * 1.1));
            labelText.setFillColor(sf::Color::Black);
             sf::FloatRect textBounds = labelText.getLocalBounds();
             labelText.setOrigin({textBounds.position.x + textBounds.size.x / 2.0f, textBounds.position.y + textBounds.size.y / 2.0f});
             labelText.setPosition({x, top});
            ctx.draw(labelText);
        }
    }
}


float Visualizer::getNodeX(int nodeCount, int index, float left, float right) {
    if (nodeCount <= 1) {
        return lerp(left, right, 0.5f);
    }
    return lerp(left, right, static_cast<float>(index) / (nodeCount - 1));
}