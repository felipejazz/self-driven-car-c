#include "Road.hpp"
#include "Utils.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Color.hpp>
#include <algorithm>
#include <cmath>

Road::Road(float centerX, float roadWidth, int lanes)
    : x(centerX), width(roadWidth), laneCount(lanes)
{
    left = x - width / 2.0f;
    right = x + width / 2.0f;
    const float infinity = 1000000.0f;
    top = -infinity;
    bottom = infinity;

    sf::Vector2f topLeft = {left, top};
    sf::Vector2f topRight = {right, top};
    sf::Vector2f bottomLeft = {left, bottom};
    sf::Vector2f bottomRight = {right, bottom};

    borders.push_back({topLeft, bottomLeft});
    borders.push_back({topRight, bottomRight});
}

float Road::getLaneCenter(int laneIndex) const {
    if (laneCount == 0) return x;
    const float laneWidth = width / static_cast<float>(laneCount);
    int clampedIndex = std::max(0, std::min(laneIndex, laneCount - 1));
    return left + laneWidth / 2.0f + static_cast<float>(clampedIndex) * laneWidth;
}

int Road::getLaneIndex(float xPos){
    int existingLaneIndex = -1;
    if (this->laneCount > 0 && xPos >= this->left && xPos <= this->right) {
        const float laneWidth = this->width / static_cast<float>(this->laneCount);
        existingLaneIndex = static_cast<int>((xPos - this->left) / laneWidth);
        existingLaneIndex = std::max(0, std::min(existingLaneIndex, this->laneCount - 1)); 
        return existingLaneIndex;
    }

}
void Road::draw(sf::RenderTarget& target) {
    if (laneCount > 1) {
        sf::VertexArray laneLines(sf::PrimitiveType::Lines);
        const float dashLength = 20.0f;
        const float gapLength = 15.0f;
        for (int i = 1; i < laneCount; ++i) {
            float laneX = lerp(left, right, static_cast<float>(i) / static_cast<float>(laneCount));
            float currentY = top;
            while (currentY < bottom) {
                float endOfDashY = std::min(currentY + dashLength, bottom);
                laneLines.append(sf::Vertex{{laneX, currentY}, sf::Color::Yellow});
                laneLines.append(sf::Vertex{{laneX, endOfDashY}, sf::Color::Yellow});
                currentY += dashLength + gapLength;
            }
        }
        if (laneLines.getVertexCount() > 0) {
            target.draw(laneLines);
        }
    }

    sf::VertexArray borderLines(sf::PrimitiveType::Lines);
    for (const auto& border : borders) {
        borderLines.append(sf::Vertex{border.first, sf::Color::White});
        borderLines.append(sf::Vertex{border.second, sf::Color::White});
    }
    if (borderLines.getVertexCount() > 0) {
        target.draw(borderLines);
    }
}