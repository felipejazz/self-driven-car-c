#pragma once
#include <vector>

class Level {
public:
    std::vector<float> inputs;
    std::vector<float> outputs;
    std::vector<float> biases;
    std::vector<std::vector<float>> weights;
    Level() = default;
    Level(int inputCount, int outputCount);
    static void feedForward(Level &level, const std::vector<float> &givenInputs, std::vector<float> &givenOutputs);

    static void randomize(Level &level);
};

class NeuralNetwork {
public:
    std::vector<Level> levels;
    NeuralNetwork(const std::vector<int> &neuronCounts);

    static std::vector<float> feedForward(const std::vector<float> &givenInputs, NeuralNetwork &network);

    static void copy(NeuralNetwork &dest, const NeuralNetwork &src);
    static void mutate(NeuralNetwork &network, float amount);
};
