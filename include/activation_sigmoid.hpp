#ifndef ACTIVATION_SIGMOID_HPP
#define ACTIVATION_SIGMOID_HPP

#include "activation.hpp"

class Activation_Sigmoid : public Activation {
public:
    void forward(const Eigen::MatrixXd& inputs) override;
};

#endif // ACTIVATION_SIGMOID_HPP
