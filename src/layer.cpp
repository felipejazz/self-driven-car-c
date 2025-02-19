#include "layer.hpp"
#include <random>

Layer::Layer(int n_inputs, int n_neurons) {
    weights.resize(n_inputs, n_neurons);

    biases = Eigen::VectorXd::Zero(n_neurons);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> dist(0.0, 1.0);

    for (int i = 0; i < n_inputs; ++i) {
        for (int j = 0; j < n_neurons; ++j) {
            weights(i, j) = 0.1 * dist(gen);
        }
    }
}

void Layer::forward(const Eigen::MatrixXd& inputs) {
    output = (inputs * weights).rowwise() + biases.transpose();
}
