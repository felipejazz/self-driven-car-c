#include "activation_tanh.hpp"
#include <cmath>

void Activation_Tanh::forward(const Eigen::MatrixXd& inputs) {
    output = inputs.unaryExpr([](double x) { return std::tanh(x); });
}
