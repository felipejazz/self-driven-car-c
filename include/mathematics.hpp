#ifndef MATHEMATICS_HPP
#define MATHEMATICS_HPP

#include <vector>
#include <optional>


// A point in 2D space.
struct Point {
    double x;
    double y;
};

// Verify if a point is inside a polygon.
struct Intersection {
    double x;
    double y;
    double offset;
};

double lerp(double A, double B, double t);

// Returns the distance between two points.
std::optional<Intersection> getIntersection(const Point& A, const Point& B, const Point& C, const Point& D);

// Verify if a point is inside a polygon.
bool polysIntersect(const std::vector<Point>& poly1, const std::vector<Point>& poly2);

// Verify if two polygons are colliding.
bool aabbCollision(const std::vector<Point>& poly1, const std::vector<Point>& poly2);

#endif // MATHEMATICS_HPP
