#include "activation_softmax.hpp"
#include <Eigen/Dense>
#include <cmath>

void Activation_Softmax::forward(const Eigen::MatrixXd& inputs) {
    output.resize(inputs.rows(), inputs.cols());
    for (int i = 0; i < inputs.rows(); i++) {
        double row_max = inputs.row(i).maxCoeff();
        Eigen::VectorXd exps = (inputs.row(i).array() - row_max).exp();
        double sum_exps = exps.sum();
        output.row(i) = exps / sum_exps;
    }
}
