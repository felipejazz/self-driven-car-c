#include "Utils.hpp"
#include <algorithm> // for std::min
#include <cstdint>

// Mersenne Twister random number generator
std::mt19937 rng(std::random_device{}());
std::uniform_real_distribution<float> dist(0.0f, 1.0f);
std::uniform_real_distribution<float> dist_signed(-1.0f, 1.0f);
std::uniform_int_distribution<int> dist_hue(290, 290 + 260 -1); // For HSL

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

float getRandom() {
    return dist(rng);
}

float getRandomSigned() {
    return dist_signed(rng);
}

std::optional<IntersectionData> getIntersection(
    const sf::Vector2f& A, const sf::Vector2f& B,
    const sf::Vector2f& C, const sf::Vector2f& D)
{
    // Line AB represented as A + t * (B - A)
    // Line CD represented as C + u * (D - C)
    // We want to find t and u such that A + t*(B-A) = C + u*(D-C)
    // Rearranging gives: t*(B-A) - u*(D-C) = C - A

    // Let vector r = B - A
    // Let vector s = D - C
    // Let vector q = C - A

    // We have the equations:
    // t*rx - u*sx = qx
    // t*ry - u*sy = qy

    // Using Cramer's rule or substitution to solve for t and u.
    // Using the formula from the JS code (cross product based):
    const float bottom = (D.y - C.y) * (B.x - A.x) - (D.x - C.x) * (B.y - A.y);

    if (std::abs(bottom) < 1e-6) { // Lines are parallel or collinear
        return std::nullopt;
    }

    const float tTop = (D.x - C.x) * (A.y - C.y) - (D.y - C.y) * (A.x - C.x);
    const float uTop = (C.y - A.y) * (A.x - B.x) - (C.x - A.x) * (A.y - B.y);

    const float t = tTop / bottom;
    const float u = uTop / bottom;

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

// Helper to convert HSL to RGB
// Inputs H=0-360, S=0-1, L=0-1
// Outputs R, G, B=0-255
sf::Color hslToRgb(float h, float s, float l) {
    float r, g, b;

    // ... (rest of the logic remains the same)

    if (s == 0) {
        r = g = b = l; // achromatic
    } else {
        auto hue2rgb = [](float p, float q, float t) {
            if (t < 0.f) t += 1.f;
            if (t > 1.f) t -= 1.f;
            if (t < 1.f/6.f) return p + (q - p) * 6.f * t;
            if (t < 1.f/2.f) return q;
            if (t < 2.f/3.f) return p + (q - p) * (2.f/3.f - t) * 6.f;
            return p;
        };

        float q = l < 0.5f ? l * (1.f + s) : l + s - l * s;
        float p = 2.f * l - q;
        h /= 360.f; // Normalize H to 0-1
        r = hue2rgb(p, q, h + 1.f/3.f);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.f/3.f);
    }

    // Use uint8_t here
    return sf::Color(
        static_cast<uint8_t>(r * 255),
        static_cast<uint8_t>(g * 255),
        static_cast<uint8_t>(b * 255)
    );
}

sf::Color getRandomColor() {
    // Example: Generate a random hue in a pleasant range (e.g., blues/purples)
    // You already have dist_hue in your code, but it seems unused. Let's use getRandom.
    float randomHue = 200.0f + getRandom() * 160.0f; // Hue from 200 to 360
    // You can adjust the hue range, saturation (s), and lightness (l)
    return hslToRgb(randomHue, 0.8f, 0.6f); // Example: Sat=0.8, Lightness=0.6
}
sf::Color getValueColor(float value) {
    // Clamping alpha to [0, 1]
    float alpha = std::max(0.0f, std::min(1.0f, std::abs(value)));

    // Use uint8_t here
    uint8_t R = (value < 0) ? 0 : 255;
    uint8_t G = R; // Same as R in the original JS
    uint8_t B = (value > 0) ? 0 : 255;
    uint8_t A = static_cast<uint8_t>(alpha * 255); // Use uint8_t here

    return sf::Color(R, G, B, A);
}