#ifndef LOSS_HPP
#define LOSS_HPP

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>

class Loss {
public:
    virtual ~Loss() = default;
    
    virtual Eigen::VectorXd forward(const Eigen::MatrixXd& y_pred, const Eigen::VectorXi& y_true) = 0;
    
    virtual Eigen::VectorXd forward(const Eigen::MatrixXd& y_pred, const Eigen::MatrixXd& y_true) = 0;
    
    double calculate(const Eigen::MatrixXd& output, const Eigen::VectorXi& y) {
        Eigen::VectorXd sample_losses = forward(output, y);
        return sample_losses.mean();
    }
    
    double calculate(const Eigen::MatrixXd& output, const Eigen::MatrixXd& y) {
        Eigen::VectorXd sample_losses = forward(output, y);
        return sample_losses.mean();
    }
};

class Loss_CategoricalCrossEntropy : public Loss {
public:
    Eigen::VectorXd forward(const Eigen::MatrixXd& y_pred, const Eigen::VectorXi& y_true) override;
    
    Eigen::VectorXd forward(const Eigen::MatrixXd& y_pred, const Eigen::MatrixXd& y_true) override;
};

#endif // LOSS_HPP
