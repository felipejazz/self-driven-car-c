#include "Network.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

Level::Level(int inputCount, int outputCount)
    : inputs(inputCount), outputs(outputCount), biases(outputCount), weights(inputCount, std::vector<float>(outputCount))
{
    randomize();
}

void Level::randomize() {
    for (int i = 0; i < inputs.size(); ++i) {
        for (int j = 0; j < outputs.size(); ++j) {
            weights[i][j] = getRandomSigned();
        }
    }
    for (int i = 0; i < biases.size(); ++i) {
        biases[i] = getRandomSigned();
    }
}

float activate(float sum, float bias) {
    return (sum > bias) ? 1.0f : 0.0f;
}

const std::vector<float>& Level::feedForward(const std::vector<float>& givenInputs, Level& level) {
    if (givenInputs.size() != level.inputs.size()) {
        throw std::runtime_error("Input size mismatch in Level::feedForward");
    }
    for(size_t i = 0; i < level.inputs.size(); ++i) {
        level.inputs[i] = givenInputs[i];
    }
    for (int i = 0; i < level.outputs.size(); ++i) {
        float sum = 0.0f;
        for (int j = 0; j < level.inputs.size(); ++j) {
            sum += level.inputs[j] * level.weights[j][i];
        }
        level.outputs[i] = activate(sum, level.biases[i]);
    }
    return level.outputs;
}

void Level::save(std::ofstream& file) const {
    size_t biasCount = biases.size();
    file.write(reinterpret_cast<const char*>(&biasCount), sizeof(biasCount));
    file.write(reinterpret_cast<const char*>(biases.data()), biasCount * sizeof(float));
    size_t inputCount = inputs.size();
    size_t outputCount = outputs.size();
    file.write(reinterpret_cast<const char*>(&inputCount), sizeof(inputCount));
    if (inputCount > 0) {
        for(size_t i = 0; i < inputCount; ++i) {
            file.write(reinterpret_cast<const char*>(weights[i].data()), outputCount * sizeof(float));
        }
    }
}

void Level::load(std::ifstream& file) {
    size_t biasCount;
    file.read(reinterpret_cast<char*>(&biasCount), sizeof(biasCount));
    biases.resize(biasCount);
    file.read(reinterpret_cast<char*>(biases.data()), biasCount * sizeof(float));
    size_t inputCount;
    size_t outputCount = biasCount;
    file.read(reinterpret_cast<char*>(&inputCount), sizeof(inputCount));
    inputs.resize(inputCount);
    outputs.resize(outputCount);
    weights.resize(inputCount);
    for(size_t i = 0; i < inputCount; ++i) {
        weights[i].resize(outputCount);
        file.read(reinterpret_cast<char*>(weights[i].data()), outputCount * sizeof(float));
    }
}

NeuralNetwork::NeuralNetwork(const std::vector<int>& neuronCounts) {
    if (neuronCounts.size() < 2) {
        throw std::runtime_error("Need at least an input and output layer count");
    }
    levels.reserve(neuronCounts.size() - 1);
    for (size_t i = 0; i < neuronCounts.size() - 1; ++i) {
        levels.emplace_back(neuronCounts[i], neuronCounts[i+1]);
    }
}

std::vector<float> NeuralNetwork::feedForward(std::vector<float> givenInputs, const NeuralNetwork& network) {
    std::vector<float> outputs = givenInputs;
    NeuralNetwork mutableNetwork = network;
    for (size_t i = 0; i < mutableNetwork.levels.size(); ++i) {
        outputs = Level::feedForward(outputs, mutableNetwork.levels[i]);
    }
    return outputs;
}

void NeuralNetwork::mutate(NeuralNetwork& network, float amount) {
    if (amount == 0.0f) return;
    for (Level& level : network.levels) {
        for (float& bias : level.biases) {
            bias = lerp(bias, getRandomSigned(), amount);
        }
        for (std::vector<float>& inputWeights : level.weights) {
            for (float& weight : inputWeights) {
                weight = lerp(weight, getRandomSigned(), amount);
            }
        }
    }
}

bool NeuralNetwork::saveToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for saving network: " << filename << std::endl;
        return false;
    }
    size_t numLevels = levels.size();
    file.write(reinterpret_cast<const char*>(&numLevels), sizeof(numLevels));
    for (const auto& level : levels) {
        level.save(file);
    }
    file.close();
    return !file.fail();
}

bool NeuralNetwork::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    size_t numLevels;
    file.read(reinterpret_cast<char*>(&numLevels), sizeof(numLevels));
    if (file.fail() || numLevels == 0) {
        std::cerr << "Error: Invalid network file format (level count)." << std::endl;
        return false;
    }
    levels.clear();
    levels.reserve(numLevels);
    try {
        for(size_t i = 0; i < numLevels; ++i) {
            Level tempLevel(1, 1);
            tempLevel.load(file);
            if (file.fail()) {
                std::cerr << "Error: Failed reading level " << i << " from file." << std::endl;
                return false;
            }
            levels.push_back(std::move(tempLevel));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during network load: " << e.what() << std::endl;
        levels.clear();
        return false;
    }
    file.close();
    return !file.fail();
}