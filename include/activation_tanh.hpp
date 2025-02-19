#ifndef ACTIVATION_TANH_HPP
#define ACTIVATION_TANH_HPP

#include "activation.hpp"

class Activation_Tanh : public Activation {
public:
    void forward(const Eigen::MatrixXd& inputs) override;
};

#endif // ACTIVATION_TANH_HPP
