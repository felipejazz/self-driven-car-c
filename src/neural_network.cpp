#include "neural_network.hpp"
#include "util.hpp"
#include <random>
#include <cmath>
#include <iostream>


// Implementação da classe Layer
Layer::Layer(int inputCount, int outputCount) {
    // Inicializa os pesos com valores aleatórios entre -1 e 1
    weights = Eigen::MatrixXd::Random(inputCount, outputCount);
    
    // Inicializa os vieses com zeros
    biases = Eigen::VectorXd::Zero(outputCount);
}

void Layer::forward(const Eigen::MatrixXd& inputs) {
    // Calcula a saída: output = inputs * weights + biases
    output = inputs * weights;
    
    // Adiciona os vieses
    for (int i = 0; i < output.rows(); i++) {
        for (int j = 0; j < output.cols(); j++) {
            output(i, j) += biases(j);
        }
    }
}

// Implementação da classe NeuralNetwork
NeuralNetwork::NeuralNetwork(const std::vector<int>& neuronCounts) {
    for (size_t i = 0; i < neuronCounts.size() - 1; i++) {
        layers.emplace_back(neuronCounts[i], neuronCounts[i + 1]);
    }
}

NeuralNetwork::NeuralNetwork(const NeuralNetwork& network) {
    for (const auto& layer : network.layers) {
        Layer newLayer(layer.weights.rows(), layer.weights.cols());
        newLayer.weights = layer.weights;
        newLayer.biases = layer.biases;
        layers.push_back(newLayer);
    }
}

Eigen::VectorXd NeuralNetwork::feedForward(const Eigen::VectorXd& inputs) {
    Eigen::MatrixXd outputs = inputs.transpose();
  
    for (auto& layer : layers) {
        layer.forward(outputs);
      
        // Apply ReLU activation to all except the last layer
        if (&layer != &layers.back()) {
            layer.output = layer.output.unaryExpr([](double x) { return std::max(0.0, x); });
        }
      
        outputs = layer.output;
    }
  
    // Apply sigmoid to the final layer output
    outputs = outputs.unaryExpr([](double x) { return 1.0 / (1.0 + std::exp(-x)); });
  
    return outputs.row(0);
}

void NeuralNetwork::mutate(float rate) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(0.0, 1.0);
  
    for (auto& layer : layers) {
        for (int i = 0; i < layer.weights.rows(); i++) {
            for (int j = 0; j < layer.weights.cols(); j++) {
                if (static_cast<float>(dist(gen)) < rate) {
                    layer.weights(i, j) += 0.1 * dist(gen);
                }
            }
        }
      
        for (int i = 0; i < layer.biases.size(); i++) {
            if (static_cast<float>(dist(gen)) < rate) {
                layer.biases(i) += 0.1 * dist(gen);
            }
        }
    }
}

NeuralNetwork* NeuralNetwork::clone() const {
    return new NeuralNetwork(*this);
}

float NeuralNetwork::getNodeX(int nodeCount, int index, float left, float right) const {
    if (nodeCount == 1) {
        return (left + right) / 2.0f;
    }
    return left + (right - left) * (static_cast<float>(index) / (nodeCount - 1));
}

sf::Color getRGBA(double value) {
    // Map value from [-1, 1] to color
    int r = 0, g = 0, b = 0;
    int alpha = 255;
  
    if (value < 0) {
        r = 255;
        g = b = static_cast<int>(255 * (1 + value));
    } else {
        g = 255;
        r = b = static_cast<int>(255 * (1 - value));
    }
  
    return sf::Color(r, g, b, alpha);
}

void NeuralNetwork::drawLevel(sf::RenderTarget& target, const Layer& level, float left, float top, 
                             float width, float height, const std::vector<std::string>& outputLabels) const {
    float right = left + width;
    float bottom = top + height;
  
    int inputCount = level.weights.rows();
    int outputCount = level.weights.cols();
  
    // Draw connections between nodes
    for (int i = 0; i < inputCount; i++) {
        for (int j = 0; j < outputCount; j++) {
            float startX = getNodeX(inputCount, i, left, right);
            float startY = bottom;
            float endX = getNodeX(outputCount, j, left, right);
            float endY = top;
          
            // Draw line
            sf::Vertex line[] = {
                sf::Vertex(),
                sf::Vertex()
            };
            line[0].position = sf::Vector2f(startX, startY);
            line[0].color = getRGBA(level.weights(i, j));
            line[1].position = sf::Vector2f(endX, endY);
            line[1].color = getRGBA(level.weights(i, j));
          
            target.draw(line, 2, sf::PrimitiveType::Lines);        
        }
    }
  
    // Draw input nodes
    for (int i = 0; i < inputCount; i++) {
        float x = getNodeX(inputCount, i, left, right);
      
        // Outer circle
        sf::CircleShape outerCircle(nodeRadius);
        outerCircle.setFillColor(sf::Color::Black);
        outerCircle.setPosition(sf::Vector2f(x - nodeRadius, bottom - nodeRadius));
        target.draw(outerCircle);
      
        // Inner circle
        sf::CircleShape innerCircle(nodeRadius * 0.6f);
        innerCircle.setFillColor(sf::Color::White); // Default color when no input
        innerCircle.setPosition(sf::Vector2f(x - nodeRadius * 0.6f, bottom - nodeRadius * 0.6f));
        target.draw(innerCircle);
    }
  
    // Draw output nodes
    for (int i = 0; i < outputCount; i++) {
        float x = getNodeX(outputCount, i, left, right);
      
        // Outer circle
        sf::CircleShape outerCircle(nodeRadius);
        outerCircle.setFillColor(sf::Color::Black);
        outerCircle.setPosition(sf::Vector2f(x - nodeRadius, top - nodeRadius));
        target.draw(outerCircle);
      
        // Inner circle
        sf::CircleShape innerCircle(nodeRadius * 0.6f);
        innerCircle.setFillColor(sf::Color::White); // Default color when no output
        innerCircle.setPosition(sf::Vector2f(x - nodeRadius * 0.6f, top - nodeRadius * 0.6f));
        target.draw(innerCircle);
      
        // Bias circle
        sf::CircleShape biasCircle(nodeRadius * 0.8f);
        biasCircle.setFillColor(sf::Color::Transparent);
        biasCircle.setOutlineColor(getRGBA(level.biases(i)));
        biasCircle.setOutlineThickness(2.0f);
        biasCircle.setPosition(sf::Vector2f(x - nodeRadius * 0.8f, top - nodeRadius * 0.8f));
        target.draw(biasCircle);
        static bool s_fontLoaded = false;
        static sf::Font s_font;

        // Draw labels if provided
        if (i < outputLabels.size()) {
            // Tente carregar a fonte se ainda não foi carregada
            if (!s_fontLoaded) {
                // Tente vários caminhos possíveis para a fonte
                if (s_font.openFromFile("../resources/LiberationSans-Regular.ttf")) {
                    s_fontLoaded = true;
                } else if (s_font.openFromFile("resources/LiberationSans-Regular.ttf")) {
                    s_fontLoaded = true;
                } else if (s_font.openFromFile("/usr/share/fonts/TTF/LiberationSans-Regular.ttf")) {
                    s_fontLoaded = true;
                } else if (s_font.openFromFile("/usr/share/fonts/liberation/LiberationSans-Regular.ttf")) {
                    s_fontLoaded = true;
                } else {
                    std::cout << "Não foi possível carregar nenhuma fonte. Informações de geração não serão exibidas." << std::endl;
                    std::cout << "Procurando em: " << std::endl;
                    std::cout << "  - ../resources/LiberationSans-Regular.ttf" << std::endl;
                    std::cout << "  - resources/LiberationSans-Regular.ttf" << std::endl;
                    std::cout << "  - /usr/share/fonts/TTF/LiberationSans-Regular.ttf" << std::endl;
                    std::cout << "  - /usr/share/fonts/liberation/LiberationSans-Regular.ttf" << std::endl;
                }
            }
            
            if (s_fontLoaded) {
                // Crie o texto com o construtor correto
                sf::Text text(s_font, outputLabels[i], static_cast<unsigned int>(nodeRadius * 1.5f));
                text.setFillColor(sf::Color::Black);
                text.setOutlineColor(sf::Color::White);
                text.setOutlineThickness(0.5f);
                
                // Centralize o texto
                sf::FloatRect textBounds = text.getLocalBounds();
                text.setPosition(sf::Vector2f(x, top));
                
                target.draw(text);
            }
        }
    }
}

void NeuralNetwork::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    float left = margin;
    float top = margin;
    float width = target.getSize().x - margin * 2;
    float height = target.getSize().y - margin * 2;
  
    float levelHeight = height / layers.size();
  
    for (int i = layers.size() - 1; i >= 0; i--) {
        float levelTop = top + lerp(height - levelHeight, 0.0f, 
                                   layers.size() == 1 ? 0.5f : static_cast<float>(i) / (layers.size() - 1));
      
        std::vector<std::string> outputLabels;
        if (i == static_cast<int>(layers.size()) - 1) {
            outputLabels = {"↑", "←", "→", "↓"};
        }
      
        drawLevel(target, layers[i], left, levelTop, width, levelHeight, outputLabels);
    }
}