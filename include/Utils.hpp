#ifndef UTILS_HPP
#define UTILS_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <random>
#include <cstdint> // <--- Make sure this is included

// Structure for intersection results
struct IntersectionData {
    sf::Vector2f point;
    float offset; // 't' value for the first segment (A, B)
};

// Function Declarations (Prototypes)
float lerp(float a, float b, float t);
float getRandom();
float getRandomSigned();
std::optional<IntersectionData> getIntersection(
    const sf::Vector2f& A, const sf::Vector2f& B,
    const sf::Vector2f& C, const sf::Vector2f& D);
bool polysIntersect(
    const std::vector<sf::Vector2f>& poly1,
    const std::vector<sf::Vector2f>& poly2);
sf::Color hslToRgb(float h, float s, float l);
sf::Color getValueColor(float value);
sf::Color getRandomColor();

#endif // UTILS_HPP