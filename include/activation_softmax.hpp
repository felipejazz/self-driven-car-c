#ifndef ACTIVATION_SOFTMAX_HPP
#define ACTIVATION_SOFTMAX_HPP

#include "activation.hpp"

class Activation_Softmax : public Activation {
public:
    void forward(const Eigen::MatrixXd& inputs) override;
};

#endif // ACTIVATION_SOFTMAX_HPP
