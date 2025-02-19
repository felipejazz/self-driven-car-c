#include "loss.hpp"

Eigen::VectorXd Loss_CategoricalCrossEntropy::forward(const Eigen::MatrixXd& y_pred, const Eigen::VectorXi& y_true) {
    int samples = y_pred.rows();
    Eigen::MatrixXd y_pred_clipped = y_pred.unaryExpr([](double x) {
        return std::min(std::max(x, 1e-7), 1.0 - 1e-7);
    });
    
    Eigen::VectorXd correct_confidences(samples);
    for (int i = 0; i < samples; i++) {
        int index = y_true(i);
        correct_confidences(i) = y_pred_clipped(i, index);
    }
    
    Eigen::VectorXd negative_log_likelihoods = -correct_confidences.array().log();
    return negative_log_likelihoods;
}

Eigen::VectorXd Loss_CategoricalCrossEntropy::forward(const Eigen::MatrixXd& y_pred, const Eigen::MatrixXd& y_true) {
    int samples = y_pred.rows();
    Eigen::MatrixXd y_pred_clipped = y_pred.unaryExpr([](double x) {
        return std::min(std::max(x, 1e-7), 1.0 - 1e-7);
    });
    
    Eigen::VectorXd correct_confidences(samples);
    for (int i = 0; i < samples; i++) {
        correct_confidences(i) = (y_pred_clipped.row(i).array() * y_true.row(i).array()).sum();
    }
    
    Eigen::VectorXd negative_log_likelihoods = -correct_confidences.array().log();
    return negative_log_likelihoods;
}
