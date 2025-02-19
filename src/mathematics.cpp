#include "mathematics.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <tuple>

double lerp(double A, double B, double t) {
    return A + (B - A) * t;
}

std::optional<Intersection> getIntersection(const Point& A, const Point& B, const Point& C, const Point& D) {
    double t_top = (D.x - C.x) * (A.y - C.y) - (D.y - C.y) * (A.x - C.x);
    double u_top = (C.y - A.y) * (A.x - B.x) - (C.x - A.x) * (A.y - B.y);
    double bottom = (D.y - C.y) * (B.x - A.x) - (D.x - C.x) * (B.y - A.y);

    if (bottom != 0) {
        double t = t_top / bottom;
        double u = u_top / bottom;
        if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
            Intersection inter;
            inter.x = lerp(A.x, B.x, t);
            inter.y = lerp(A.y, B.y, t);
            inter.offset = t;
            return inter;
        }
    }
    return std::nullopt;
}

bool polysIntersect(const std::vector<Point>& poly1, const std::vector<Point>& poly2) {
    // Lambda para projetar um polígono em uma direção (normal)
    auto projectPolygon = [](const std::vector<Point>& poly, const Point& normal) -> std::pair<double, double> {
        double minProj = std::numeric_limits<double>::infinity();
        double maxProj = -std::numeric_limits<double>::infinity();
        for (const auto& p : poly) {
            double projection = normal.x * p.x + normal.y * p.y;
            minProj = std::min(minProj, projection);
            maxProj = std::max(maxProj, projection);
        }
        return {minProj, maxProj};
    };

    // Para cada aresta dos polígonos, verifica se existe um eixo separador.
    for (int polyIndex = 0; polyIndex < 2; ++polyIndex) {
        const std::vector<Point>& poly = (polyIndex == 0) ? poly1 : poly2;
        for (size_t i1 = 0; i1 < poly.size(); i1++) {
            size_t i2 = (i1 + 1) % poly.size();
            Point p1 = poly[i1];
            Point p2 = poly[i2];

            // Calcula a normal (vetor perpendicular à aresta)
            Point normal;
            normal.x = p2.y - p1.y;
            normal.y = p1.x - p2.x;

            auto [minA, maxA] = projectPolygon(poly1, normal);
            auto [minB, maxB] = projectPolygon(poly2, normal);

            if (maxA < minB || maxB < minA) {
                // Eixo separador encontrado: não há interseção
                return false;
            }
        }
    }
    return true;
}

bool aabbCollision(const std::vector<Point>& poly1, const std::vector<Point>& poly2) {
    auto getBounds = [](const std::vector<Point>& poly) {
        double minX = std::numeric_limits<double>::infinity();
        double maxX = -std::numeric_limits<double>::infinity();
        double minY = std::numeric_limits<double>::infinity();
        double maxY = -std::numeric_limits<double>::infinity();
        for (const auto& p : poly) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }
        return std::make_tuple(minX, maxX, minY, maxY);
    };

    auto [min_x1, max_x1, min_y1, max_y1] = getBounds(poly1);
    auto [min_x2, max_x2, min_y2, max_y2] = getBounds(poly2);

    return (min_x1 <= max_x2 && max_x1 >= min_x2 &&
            min_y1 <= max_y2 && max_y1 >= min_y2);
}
