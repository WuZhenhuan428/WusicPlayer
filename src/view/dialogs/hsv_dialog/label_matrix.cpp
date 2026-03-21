#include "label_matrix.h"
#include "hsv_to_rgb.h"

LabelMatrix::LabelMatrix(QWidget* parent)
    : QWidget(parent)
{
    m_lbR = new QLabel(this);
    m_lbG = new QLabel(this);
    m_lbB = new QLabel(this);
    m_lbH = new QLabel(this);
    m_lbS = new QLabel(this);
    m_lbV = new QLabel(this);
    m_leR = new QLineEdit(this);
    m_leG = new QLineEdit(this);
    m_leB = new QLineEdit(this);
    m_leH = new QLineEdit(this);
    m_leS = new QLineEdit(this);
    m_leV = new QLineEdit(this);
    
    m_blHR = new QHBoxLayout;
    m_blSG = new QHBoxLayout;
    m_blVB = new QHBoxLayout;
    m_blMain = new QVBoxLayout;
    
    m_lbR->setText("R: ");
    m_lbG->setText("G: ");
    m_lbB->setText("B: ");
    m_lbH->setText("H: ");
    m_lbS->setText("S: ");
    m_lbV->setText("V: ");

    m_blHR->addWidget(m_lbH);
    m_blHR->addWidget(m_leH);
    m_blHR->addStretch();
    m_blHR->addWidget(m_lbR);
    m_blHR->addWidget(m_leR);

    m_blSG->addWidget(m_lbS);
    m_blSG->addWidget(m_leS);
    m_blSG->addStretch();
    m_blSG->addWidget(m_lbG);
    m_blSG->addWidget(m_leG);

    m_blVB->addWidget(m_lbV);
    m_blVB->addWidget(m_leV);
    m_blVB->addStretch();

    m_blVB->addWidget(m_lbB);
    m_blVB->addWidget(m_leB);

    m_blMain->addLayout(m_blHR);
    m_blMain->addLayout(m_blSG);
    m_blMain->addLayout(m_blVB);
    m_blMain->addStretch();

    this->setLayout(m_blMain);

    connect(m_leR, &QLineEdit::textEdited, this, [this](const QString& str){
        const int number = str.toInt();
        int num_clamped = std::clamp(number, 0x00, 0xFF);
        m_rgb.r = num_clamped;
        updateHSV();
        emit sgnEditColor(m_hsv);
    });

    connect(m_leG, &QLineEdit::textEdited, this, [this](const QString& str){
        const int number = str.toInt();
        int num_clamped = std::clamp(number, 0x00, 0xFF);
        m_rgb.g = num_clamped;
        updateHSV();
        emit sgnEditColor(m_hsv);
    });

    connect(m_leB, &QLineEdit::textEdited, this, [this](const QString& str){
        const int number = str.toInt();
        int num_clamped = std::clamp(number, 0x00, 0xFF);
        m_rgb.b = num_clamped;
        updateHSV();
        emit sgnEditColor(m_hsv);
    });

    connect(m_leH, &QLineEdit::textEdited, this, [this](const QString& str){
        const double number = str.toInt();
        double num_clamped = std::clamp(number, 0.0, 360.0);
        m_hsv.h = num_clamped;
        updateRGB();
        emit sgnEditColor(m_hsv);
    });

    connect(m_leS, &QLineEdit::textEdited, this, [this](const QString& str){
        const double number = str.toInt();
        double num_clamped = std::clamp(number, 0.0, 100.0);
        m_hsv.s = num_clamped;
        updateRGB();
        emit sgnEditColor(m_hsv);
    });

    connect(m_leV, &QLineEdit::textEdited, this, [this](const QString& str){
        const double number = str.toInt();
        double num_clamped = std::clamp(number, 0.0, 100.0);
        m_hsv.v = num_clamped;
        updateRGB();
        emit sgnEditColor(m_hsv);
    });

}

void LabelMatrix::setHSV(hsv_t hsv) {
    m_leH->setText(QString::number((hsv.h)));
    m_leS->setText(QString::number((hsv.s)));
    m_leV->setText(QString::number((hsv.v)));

    rgb_t rgb = hsv_to_rgb(hsv);
    m_leR->setText(QString::number((rgb.r)));
    m_leG->setText(QString::number((rgb.g)));
    m_leB->setText(QString::number((rgb.b)));
}



void LabelMatrix::updateRGB() {
    m_rgb = hsv_to_rgb(m_hsv);
}

void LabelMatrix::updateHSV() {
    m_hsv = rgb_to_hsv(m_rgb);
}
