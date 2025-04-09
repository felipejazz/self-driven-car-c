#pragma once
#include <vector>
#include <Eigen/Dense>
#include "layer.hpp"
#include <SFML/Graphics.hpp>

class NeuralNetwork : public sf::Drawable {
private:
    std::vector<Layer> layers;
  
    // For visualization
    float margin = 50.0f;
    float nodeRadius = 18.0f;
  
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    float getNodeX(int nodeCount, int index, float left, float right) const;
    void drawLevel(sf::RenderTarget& target, const Layer& level, float left, float top, 
                  float width, float height, const std::vector<std::string>& outputLabels) const;
  
public:
    NeuralNetwork(const std::vector<int>& neuronCounts);
    NeuralNetwork(const NeuralNetwork& network); // Copy constructor for cloning
  
    Eigen::VectorXd feedForward(const Eigen::VectorXd& inputs);
  
    // Evolution methods
    void mutate(float rate);
    NeuralNetwork* clone() const;
  
    // Getters
    const std::vector<Layer>& getLayers() const { return layers; }
};

// Helper function for visualization
sf::Color getRGBA(double value);