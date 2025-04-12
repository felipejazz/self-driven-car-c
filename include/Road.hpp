#ifndef ROAD_HPP // <<< CORRIGIDO Guarda de Inclusão
#define ROAD_HPP // <<< CORRIGIDO Guarda de Inclusão

#include <SFML/Graphics.hpp>
#include <vector>
#include <limits>    // Para numeric_limits
#include <algorithm> // Para std::max, std::min
#include <cmath>     // Para M_PI (se usado)
#include "Utils.hpp" // <<< ADICIONADO Para lerp

class Road {
public:
    float x;        // Centro horizontal da estrada
    float width;    // Largura total da estrada
    int laneCount;  // Número de faixas

    // Bordas verticais (simplificado, ajusta se necessário)
    float top;
    float bottom;
    // Bordas laterais
    float left;
    float right;

    // Vetor com os segmentos de reta que definem as bordas físicas da estrada
    // Cada par representa um segmento de reta (ponto inicial, ponto final)
    std::vector<std::pair<sf::Vector2f, sf::Vector2f>> borders;

    // Construtor: x é o centro horizontal, width é a largura total, lanes é o número de faixas.
    Road(float centerX, float roadWidth, int lanes = 3);

    // Retorna a coordenada X do centro de uma faixa específica (0-indexada).
    float getLaneCenter(int laneIndex) const;

    // Desenha a estrada (linhas e bordas) na janela fornecida.
    void draw(sf::RenderTarget& target); // Alterado para RenderTarget para flexibilidade

private:
    // Define a representação visual das bordas para desenho
    void setupBorders();
};

#endif // ROAD_HPP // <<< CORRIGIDO Guarda de Inclusão