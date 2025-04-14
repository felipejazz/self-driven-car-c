#include "Utils.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

std::mt19937 rng(std::random_device{}());
std::uniform_real_distribution<float> dist_01(0.0f, 1.0f);
std::uniform_real_distribution<float> dist_signed(-1.0f, 1.0f);

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float getRandom() {
    return dist_01(rng);
}

float getRandomSigned() {
    return dist_signed(rng);
}

float getRandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

int getRandomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

std::optional<IntersectionData> getIntersection(
    const sf::Vector2f& A, const sf::Vector2f& B,
    const sf::Vector2f& C, const sf::Vector2f& D)
{
    float bottom = (D.y - C.y) * (B.x - A.x) - (D.x - C.x) * (B.y - A.y);

    if (std::abs(bottom) < std::numeric_limits<float>::epsilon()) {
        return std::nullopt;
    }

    float tTop = (D.x - C.x) * (A.y - C.y) - (D.y - C.y) * (A.x - C.x);
    float uTop = (C.y - A.y) * (A.x - B.x) - (C.x - A.x) * (A.y - B.y);
    float t = tTop / bottom;
    float u = uTop / bottom;

    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
        return IntersectionData{
            { lerp(A.x, B.x, t), lerp(A.y, B.y, t) },
            t
        };
    }

    return std::nullopt;
}

bool polysIntersect(
    const std::vector<sf::Vector2f>& poly1,
    const std::vector<sf::Vector2f>& poly2)
{
    if (poly1.empty() || poly2.empty()) {
        return false;
    }
    for (size_t i = 0; i < poly1.size(); ++i) {
        for (size_t j = 0; j < poly2.size(); ++j) {
            const sf::Vector2f& p1a = poly1[i];
            const sf::Vector2f& p1b = poly1[(i + 1) % poly1.size()];
            const sf::Vector2f& p2a = poly2[j];
            const sf::Vector2f& p2b = poly2[(j + 1) % poly2.size()];
            if (getIntersection(p1a, p1b, p2a, p2b)) {
                return true;
            }
        }
    }
    return false;
}

sf::Color hslToRgb(float h, float s, float l) {
    float r, g, b;
    if (s == 0.0f) {
        r = g = b = l;
    } else {
        auto hue2rgb = [](float p, float q, float t) {
            if (t < 0.0f) t += 1.0f;
            if (t > 1.0f) t -= 1.0f;
            if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
            if (t < 1.0f/2.0f) return q;
            if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
            return p;
        };
        float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        h /= 360.0f;
        r = hue2rgb(p, q, h + 1.0f/3.0f);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0f/3.0f);
    }
    return sf::Color(
        static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, r)) * 255),
        static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, g)) * 255),
        static_cast<uint8_t>(std::max(0.0f, std::min(1.0f, b)) * 255)
    );
}

sf::Color getRandomColor() {
    float randomHue = 200.0f + getRandom() * 160.0f;
    return hslToRgb(randomHue, 0.8f, 0.6f);
}

sf::Color getValueColor(float value) {
    float alpha = std::max(0.0f, std::min(1.0f, std::abs(value)));
    uint8_t R = (value < 0) ? 0 : 255;
    uint8_t G = R;
    uint8_t B = (value > 0) ? 0 : 255;
    uint8_t A = static_cast<uint8_t>(alpha * 255);
    return sf::Color(R, G, B, A);
}