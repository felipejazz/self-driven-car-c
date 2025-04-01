#include "util.hpp"
#include <cmath>
#include <algorithm>

float lerp(float A, float B, float t) {
    return A + (B - A) * t;
}

bool getIntersection(const sf::Vector2f& A, const sf::Vector2f& B,
                     const sf::Vector2f& C, const sf::Vector2f& D,
                     sf::Vector2f& out, float& offset) {
    
    float tTop = (D.x - C.x) * (A.y - C.y) - (D.y - C.y) * (A.x - C.x);
    float uTop = (C.y - A.y) * (A.x - B.x) - (C.x - A.x) * (A.y - B.y);
    float bottom = (D.y - C.y) * (B.x - A.x) - (D.x - C.x) * (B.y - A.y);

    if (std::fabs(bottom) > 0.0001f) {
        float t = tTop / bottom;
        float u = uTop / bottom;
        if (t >= 0.f && t <= 1.f && u >= 0.f && u <= 1.f) {
            out.x = lerp(A.x, B.x, t);
            out.y = lerp(A.y, B.y, t);
            offset = t;
            return true;
        }
    }
    return false;
}

bool polysIntersect(const std::vector<sf::Vector2f>& poly1,
                    const std::vector<sf::Vector2f>& poly2) {
    for (size_t i = 0; i < poly1.size(); i++) {
        sf::Vector2f A = poly1[i];
        sf::Vector2f B = poly1[(i + 1) % poly1.size()];
        for (size_t j = 0; j < poly2.size(); j++) {
            sf::Vector2f C = poly2[j];
            sf::Vector2f D = poly2[(j + 1) % poly2.size()];
            sf::Vector2f intersect;
            float offset;
            if (getIntersection(A, B, C, D, intersect, offset)) {
                return true;
            }
        }
    }
    return false;
}

float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
    return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}
