#include "activation_relu.hpp"

void Activation_ReLU::forward(const Eigen::MatrixXd& inputs) {
    output = inputs.unaryExpr([](double x) { return x > 0 ? x : 0; });
}
