#include "Obstacle.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath> // Para std::hypot, std::atan2, std::sin, std::cos, M_PI
#include <atomic> // Certifique-se de incluir <atomic>

// Definição e inicialização do contador de IDs estático
std::atomic<long long> Obstacle::nextId(0); // <<-- DEFINIÇÃO E INICIALIZAÇÃO

Obstacle::Obstacle(float x, float y, float w, float h, sf::Color col)
    : position(x, y), width(w), height(h), color(col),
      id(nextId.fetch_add(1, std::memory_order_relaxed)) // <<-- ATRIBUI E INCREMENTA O ID
{
    updatePolygon(); // Calcula o polígono inicial
}
long long Obstacle::getId() const {
    return id;
}
void Obstacle::updatePolygon() {
    polygon.resize(4);
    const float rad = std::hypot(width, height) / 2.0f;
    // Alpha é 0 porque obstáculos não rotacionam
    const float alpha = std::atan2(width, height); // Ainda útil para os cantos

    // Polígono não rotacionado (ângulo = 0)
    polygon[0] = { position.x + std::sin(alpha) * rad, position.y - std::cos(alpha) * rad };        // Top-rightish
    polygon[1] = { position.x + std::sin(-alpha) * rad, position.y - std::cos(-alpha) * rad };       // Bottom-rightish
    polygon[2] = { position.x + std::sin(static_cast<float>(M_PI) + alpha) * rad, position.y - std::cos(static_cast<float>(M_PI) + alpha) * rad }; // Bottom-leftish
    polygon[3] = { position.x + std::sin(static_cast<float>(M_PI) - alpha) * rad, position.y - std::cos(static_cast<float>(M_PI) - alpha) * rad }; // Top-leftish
}

std::vector<sf::Vector2f> Obstacle::getPolygon() const {
    // Obstáculos são fixos, então o polígono só precisa ser calculado uma vez.
    // Se precisassem mover/rotacionar, chamaríamos updatePolygon() aqui ou no update().
    return polygon;
}

void Obstacle::draw(sf::RenderTarget& target) const {
    // Desenha um retângulo simples para o obstáculo
    sf::RectangleShape rectShape({width, height});
    rectShape.setOrigin({width / 2.0f, height / 2.0f}); // Centraliza a origem
    rectShape.setPosition(position);
    rectShape.setFillColor(color);
    rectShape.setOutlineColor(sf::Color::Black);
    rectShape.setOutlineThickness(1.f);
    target.draw(rectShape);

    // // Opcional: Desenhar o polígono para debug
    // /*
    // if (!polygon.empty()) {
    //     sf::VertexArray lines(sf::PrimitiveType::LineStrip, polygon.size() + 1);
    //     for(size_t i = 0; i < polygon.size(); ++i) {
    //         lines[i].position = polygon[i];
    //         lines[i].color = sf::Color::Red;
    //     }
    //     lines[polygon.size()].position = polygon[0]; // Fecha o loop
    //     lines[polygon.size()].color = sf::Color::Red;
    //     target.draw(lines);
    // }
    // */
}