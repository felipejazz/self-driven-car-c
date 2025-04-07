#include "neural_network.hpp"
#include <random>
#include <algorithm>
#include <cmath>

static float randomRange(float minVal, float maxVal) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(gen);
}

Level::Level(int inputCount, int outputCount)
{
    inputs.resize(inputCount, 0.f);
    outputs.resize(outputCount, 0.f);
    biases.resize(outputCount, 0.f);

    weights.resize(inputCount);
    for(int i=0; i<inputCount; i++){
        weights[i].resize(outputCount, 0.f);
    }

    Level::randomize(*this);
}

void Level::randomize(Level &level){
    for (int i=0; i<(int)level.weights.size(); i++){
        for(int j=0; j<(int)level.weights[i].size(); j++){
            level.weights[i][j] = randomRange(-1.f, 1.f);
        }
    }
    for(int i=0; i<(int)level.biases.size(); i++){
        level.biases[i] = randomRange(-1.f, 1.f);
    }
}

void Level::feedForward(Level &level, const std::vector<float> &givenInputs, std::vector<float> &givenOutputs){
    // Copia inputs
    for(int i=0; i<(int)level.inputs.size(); i++){
        level.inputs[i] = givenInputs[i];
    }

    for(int i=0; i<(int)level.outputs.size(); i++){
        float sum = 0.f;
        // Soma weighted inputs
        for(int j=0; j<(int)level.inputs.size(); j++){
            sum += level.inputs[j] * level.weights[j][i];
        }
        // Comparação com bias (igual no JS)
        if(sum > level.biases[i]){
            level.outputs[i] = 1.f;
        } else {
            level.outputs[i] = 0.f;
        }
    }

    // "Retorna" (copia) a saída para givenOutputs
    for(int i=0; i<(int)level.outputs.size(); i++){
        givenOutputs[i] = level.outputs[i];
    }
}

/* ----------------- CLASS NeuralNetwork ----------------- */

NeuralNetwork::NeuralNetwork(const std::vector<int> &neuronCounts)
{
    // Cria cada Level
    for(size_t i=0; i<neuronCounts.size()-1; i++){
        levels.emplace_back(Level(neuronCounts[i], neuronCounts[i+1]));
    }
}

std::vector<float> NeuralNetwork::feedForward(const std::vector<float> &givenInputs, NeuralNetwork &network)
{
    // Primeira camada
    std::vector<float> outputs = givenInputs;
    std::vector<float> temp;

    for(size_t i=0; i<network.levels.size(); i++){
        Level &level = network.levels[i];
        temp.resize(level.outputs.size(), 0.f);
        Level::feedForward(level, outputs, temp);
        outputs = temp;
    }
    return outputs;
}

void NeuralNetwork::copy(NeuralNetwork &dest, const NeuralNetwork &src)
{
    dest.levels.resize(src.levels.size());
    for(size_t i=0; i<src.levels.size(); i++){
        const Level &sL = src.levels[i];
        Level &dL = dest.levels[i];

        // redimensiona
        dL.inputs.resize(sL.inputs.size());
        dL.outputs.resize(sL.outputs.size());
        dL.biases.resize(sL.biases.size());
        dL.weights.resize(sL.weights.size());

        for(size_t w=0; w<sL.weights.size(); w++){
            dL.weights[w].resize(sL.weights[w].size());
        }

        // copia
        dL.inputs   = sL.inputs;
        dL.outputs  = sL.outputs;
        dL.biases   = sL.biases;
        for(size_t w=0; w<sL.weights.size(); w++){
            for(size_t c=0; c<sL.weights[w].size(); c++){
                dL.weights[w][c] = sL.weights[w][c];
            }
        }
    }
}

void NeuralNetwork::mutate(NeuralNetwork &network, float amount)
{
    // amount ~ 0.1 => 10% de perturbação
    for(auto &level : network.levels){
        // Pesos
        for(size_t i=0; i<level.weights.size(); i++){
            for(size_t j=0; j<level.weights[i].size(); j++){
                // chance de alterar
                if(randomRange(0.f, 1.f) < 0.5f){
                    float delta = randomRange(-amount, amount);
                    level.weights[i][j] += delta;
                    // clipping simples
                    if(level.weights[i][j] > 1.f)  level.weights[i][j] = 1.f;
                    if(level.weights[i][j] < -1.f) level.weights[i][j] = -1.f;
                }
            }
        }
        // Biases
        for(size_t b=0; b<level.biases.size(); b++){
            if(randomRange(0.f, 1.f) < 0.5f){
                float delta = randomRange(-amount, amount);
                level.biases[b] += delta;
                if(level.biases[b] > 1.f)   level.biases[b] = 1.f;
                if(level.biases[b] < -1.f)  level.biases[b] = -1.f;
            }
        }
    }
}
