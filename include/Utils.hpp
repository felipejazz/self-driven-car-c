#ifndef UTILS_HPP
#define UTILS_HPP

#include <SFML/Graphics.hpp> // <<--- ESTE É O PONTO CHAVE
#include <vector>
#include <optional>
#include <random> // Include necessary headers here too
#include <cstdint> // For uint8_t

// Structure for intersection results
struct IntersectionData {
    sf::Vector2f point;
    float offset; // 't' value for the first segment (A, B)
};

// Function Declarations (Prototypes)
float lerp(float a, float b, float t);
float getRandom();
float getRandomSigned(); // <<< --- ADD THIS DECLARATION
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