#ifndef LAYER_HPP
#define LAYER_HPP

#include <Eigen/Dense>

class Layer {
public:
    Eigen::MatrixXd weights;
    Eigen::VectorXd biases;
    Eigen::MatrixXd output;

    Layer(int n_inputs, int n_neurons);

    void forward(const Eigen::MatrixXd& inputs);
};

#endif // LAYER_HPP
