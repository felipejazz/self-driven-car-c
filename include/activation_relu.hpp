#ifndef ACTIVATION_RELU_HPP
#define ACTIVATION_RELU_HPP

#include "activation.hpp"

class Activation_ReLU : public Activation {
public:
    void forward(const Eigen::MatrixXd& inputs) override;
};

#endif // ACTIVATION_RELU_HPP
