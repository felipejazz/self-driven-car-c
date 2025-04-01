#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>

float lerp(float A, float B, float t);

bool getIntersection(const sf::Vector2f& A, const sf::Vector2f& B,
                     const sf::Vector2f& C, const sf::Vector2f& D,
                     sf::Vector2f& out, float& offset);

bool polysIntersect(const std::vector<sf::Vector2f>& poly1,
                    const std::vector<sf::Vector2f>& poly2);

float distance(const sf::Vector2f& a, const sf::Vector2f& b);