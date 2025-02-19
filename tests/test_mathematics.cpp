#include "mathematics.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main() {
    double a = 0.0;
    double b = 10.0;
    double t = 0.5;
    double result = lerp(a, b, t);
    assert(std::abs(result - 5.0) < 1e-6);

    Point A{0, 0};
    Point B{10, 10};
    Point C{0, 10};
    Point D{10, 0};
    auto intersection = getIntersection(A, B, C, D);
    assert(intersection.has_value());
    if (intersection) {
        assert(std::abs(intersection->x - 5.0) < 1e-6);
        assert(std::abs(intersection->y - 5.0) < 1e-6);
    }

    std::vector<Point> poly1 = { {0, 0}, {10, 0}, {10, 10}, {0, 10} };
    std::vector<Point> poly2 = { {5, 5}, {15, 5}, {15, 15}, {5, 15} };
    std::vector<Point> poly3 = { {20, 20}, {30, 20}, {30, 30}, {20, 30} };

    assert(polysIntersect(poly1, poly2) == true);
    assert(polysIntersect(poly1, poly3) == false);

    assert(aabbCollision(poly1, poly2) == true);
    assert(aabbCollision(poly1, poly3) == false);

    std::cout << "All Mathematics tests passed!" << std::endl;
    return 0;
}
