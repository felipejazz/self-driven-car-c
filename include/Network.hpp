#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <vector>
#include <cmath> // For std::tanh, std::exp if using different activation
#include <fstream>
#include "Utils.hpp" // For lerp, getRandomSigned

// Represents one layer of the neural network
class Level {
public:
    std::vector<float> inputs;
    std::vector<float> outputs;
    std::vector<float> biases;
    std::vector<std::vector<float>> weights; // weights[input][output]

    Level(int inputCount, int outputCount);

    // Feed forward through this level
    static const std::vector<float>& feedForward(const std::vector<float>& givenInputs, Level& level);

    // Save/Load Level data
    void save(std::ofstream& file) const;
    void load(std::ifstream& file);

private:
    // Initialize weights and biases randomly
    void randomize();
};

// Represents the entire neural network
class NeuralNetwork {
public:
    std::vector<Level> levels;

    NeuralNetwork(const std::vector<int>& neuronCounts);

    // Feed forward through the entire network
    static std::vector<float> feedForward(std::vector<float> givenInputs, const NeuralNetwork& network);

    // Mutate the network's weights and biases
    static void mutate(NeuralNetwork& network, float amount = 1.0f);

    // Save/Load network
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
};

#endif // NETWORK_HPP