#ifndef UTILS_HPP
#define UTILS_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <random>
#include <cstdint>

struct IntersectionData {
    sf::Vector2f point;
    float offset; 
};

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
float getRandomFloat(float min, float max);
int getRandomInt(int min, int max);

#endif // UTILS_HPP