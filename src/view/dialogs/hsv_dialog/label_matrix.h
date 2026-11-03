#pragma once

#include "core/hsv_types.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

class LabelMatrix : public QWidget
{
    Q_OBJECT
public:
    explicit LabelMatrix(QWidget* parent);

    void set_hsv(hsv_t hsv);

private:
    QLabel* lb_r_;
    QLabel* lb_g_;
    QLabel* lb_b_;
    QLabel* lb_h_;
    QLabel* lb_s_;
    QLabel* lb_v_;
    QLineEdit* le_r_;
    QLineEdit* le_g_;
    QLineEdit* le_b_;
    QLineEdit* le_h_;
    QLineEdit* le_s_;
    QLineEdit* le_v_;
    QHBoxLayout* hbl_hr_;
    QHBoxLayout* hbl_sg_;
    QHBoxLayout* hbl_vb_;
    QVBoxLayout* vbl_main_;

    rgb_t rgb_ = {};
    hsv_t hsv_ = {};

private:
    void update_rgb(); // when hsv changed
    void update_hsv(); // when rgb changed

signals:
    void sgn_color_edited(hsv_t hsv);
};
