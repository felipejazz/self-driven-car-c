#ifndef MATHEMATICS_HPP
#define MATHEMATICS_HPP

#include <vector>
#include <optional>


struct Point {
    double x;
    double y;
};

struct Intersection {
    double x;
    double y;
    double offset;
};

double lerp(double A, double B, double t);

std::optional<Intersection> getIntersection(const Point& A, const Point& B, const Point& C, const Point& D);

bool polysIntersect(const std::vector<Point>& poly1, const std::vector<Point>& poly2);

bool aabbCollision(const std::vector<Point>& poly1, const std::vector<Point>& poly2);

#endif // MATHEMATICS_HPP
