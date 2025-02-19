#ifndef ACTIVATION_HPP
#define ACTIVATION_HPP

#include <Eigen/Dense>

class Activation {
public:
    Eigen::MatrixXd output;
    virtual void forward(const Eigen::MatrixXd& inputs) = 0;
    virtual ~Activation() = default;
};

#endif // ACTIVATION_HPP
