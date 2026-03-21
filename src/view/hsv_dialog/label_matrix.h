#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QString>
#include "core/hsv_types.h"

class LabelMatrix : public QWidget
{
    Q_OBJECT
public:
    explicit LabelMatrix(QWidget* parent);
    ~LabelMatrix() = default;

    void setHSV(hsv_t hsv);

private:
    QLabel* m_lbR;
    QLabel* m_lbG;
    QLabel* m_lbB;
    QLabel* m_lbH;
    QLabel* m_lbS;
    QLabel* m_lbV;
    QLineEdit* m_leR;
    QLineEdit* m_leG;
    QLineEdit* m_leB;
    QLineEdit* m_leH;
    QLineEdit* m_leS;
    QLineEdit* m_leV;
    QHBoxLayout* m_blHR;
    QHBoxLayout* m_blSG;
    QHBoxLayout* m_blVB;
    QVBoxLayout* m_blMain;

    rgb_t m_rgb;
    hsv_t m_hsv;

private:
    void updateRGB();   // when hsv changed
    void updateHSV();   // when rgb changed

signals:
    void sgnEditColor(hsv_t hsv);
};