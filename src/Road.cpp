#include "Road.hpp"
#include "Utils.hpp" // For lerp
#include <SFML/Graphics/RectangleShape.hpp> // For drawing lines if needed

Road::Road(float centerX, float roadWidth, int lanes)
    : x(centerX), width(roadWidth), laneCount(lanes)
{
    left = x - width / 2.0f;
    right = x + width / 2.0f;

    // Using a large number instead of infinity for practical rendering
    // Adjust if needed based on your view range
    const float infinity = 1000000.0f;
    top = -infinity;
    bottom = infinity;

    // Define the road borders
    sf::Vector2f topLeft = {left, top};
    sf::Vector2f topRight = {right, top};
    sf::Vector2f bottomLeft = {left, bottom};
    sf::Vector2f bottomRight = {right, bottom};

    borders.push_back({topLeft, bottomLeft});   // Left border
    borders.push_back({topRight, bottomRight}); // Right border
}

float Road::getLaneCenter(int laneIndex) const {
    if (laneCount == 0) return x; // Avoid division by zero
    const float laneWidth = width / static_cast<float>(laneCount);
    // Clamp lane index to be within valid range [0, laneCount - 1]
    int clampedIndex = std::max(0, std::min(laneIndex, laneCount - 1));
    return left + laneWidth / 2.0f + static_cast<float>(clampedIndex) * laneWidth;
}

void Road::draw(sf::RenderTarget& target) {
    // --- Draw Lane Lines ---
    if (laneCount > 1) {
         sf::VertexArray laneLines(sf::PrimitiveType::Lines);
        const float dashLength = 20.0f; // Length of a dash/gap

        for (int i = 1; i < laneCount; ++i) {
            float laneX = lerp(left, right, static_cast<float>(i) / static_cast<float>(laneCount));

            // Draw dashed lines (simulate setLineDash)
             // Iterate from top to bottom, adding segments for dashes
             // Note: This is a simplified approach. True dashed lines might require shaders or more complex geometry.
             // Here we just draw a solid line for simplicity, as SFML doesn't have built-in dashed lines.
             // For a visual cue, we can make them slightly thinner or a different shade.
             laneLines.append(sf::Vertex{{laneX, top}, sf::Color(255, 255, 255, 100)}); // Correto: usa {{pos}, color}
             laneLines.append(sf::Vertex{{laneX, bottom}, sf::Color(255, 255, 255, 100)}); // Correto: usa {{pos}, color}
        }
         if (laneLines.getVertexCount() > 0) {
             // You might need to set a different thickness or draw differently if needed.
             // For now, standard line drawing.
            target.draw(laneLines);
         }
    }


    // --- Draw Road Borders ---
    sf::VertexArray borderLines(sf::PrimitiveType::Lines);
    for (const auto& border : borders) {
        borderLines.append(sf::Vertex{border.first, sf::Color::White}); // Correto: usa {pos_vector, color}
        borderLines.append(sf::Vertex{border.second, sf::Color::White}); // Correto: usa {pos_vector, color}
        }
    if (borderLines.getVertexCount() > 0) {
        // Optionally set thickness for borders here if needed
        // Example using shapes (less efficient for many lines):
        // sf::RectangleShape lineShape;
        // lineShape.setFillColor(sf::Color::White); ... calculate position/rotation/size ...
        target.draw(borderLines);
    }
}