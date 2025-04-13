#ifndef OBSTACLE_HPP
#define OBSTACLE_HPP

#include <SFML/Graphics.hpp>
#include <vector>
#include <atomic> // Para ID atômico

class Obstacle {
public:
    sf::Vector2f position;
    float width;
    float height;
    sf::Color color;
    std::vector<sf::Vector2f> polygon; // Para detecção de colisão
    long long id; // Identificador único <<-- MEMBRO ID

    Obstacle(float x, float y, float w, float h, sf::Color col = sf::Color(128, 128, 128));

    void draw(sf::RenderTarget& target) const;
    std::vector<sf::Vector2f> getPolygon() const;

    // Opcional: Getter para o ID, se necessário externamente
    long long getId() const;

private:
    void updatePolygon();
    static std::atomic<long long> nextId; // <<-- DECLARAÇÃO ESTÁTICA
};

#endif // OBSTACLE_HPP