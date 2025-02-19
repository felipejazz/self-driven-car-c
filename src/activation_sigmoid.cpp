#include "activation_sigmoid.hpp"
#include <cmath>

void Activation_Sigmoid::forward(const Eigen::MatrixXd& inputs) {
    output = inputs.unaryExpr([](double x) { return 1.0 / (1.0 + std::exp(-x)); });
}
