#include "label_matrix.h"
#include "hsv_to_rgb.h"

LabelMatrix::LabelMatrix(QWidget* parent) : QWidget(parent)
{
    lb_r_     = new QLabel(this);
    lb_g_     = new QLabel(this);
    lb_b_     = new QLabel(this);
    lb_h_     = new QLabel(this);
    lb_s_     = new QLabel(this);
    lb_v_     = new QLabel(this);
    le_r_     = new QLineEdit(this);
    le_g_     = new QLineEdit(this);
    le_b_     = new QLineEdit(this);
    le_h_     = new QLineEdit(this);
    le_s_     = new QLineEdit(this);
    le_v_     = new QLineEdit(this);

    hbl_hr_   = new QHBoxLayout;
    hbl_sg_   = new QHBoxLayout;
    hbl_vb_   = new QHBoxLayout;
    vbl_main_ = new QVBoxLayout;

    lb_r_->setText(tr("R: "));
    lb_g_->setText(tr("G: "));
    lb_b_->setText(tr("B: "));
    lb_h_->setText(tr("H: "));
    lb_s_->setText(tr("S: "));
    lb_v_->setText(tr("V: "));

    hbl_hr_->addWidget(lb_h_);
    hbl_hr_->addWidget(le_h_);
    hbl_hr_->addStretch();
    hbl_hr_->addWidget(lb_r_);
    hbl_hr_->addWidget(le_r_);

    hbl_sg_->addWidget(lb_s_);
    hbl_sg_->addWidget(le_s_);
    hbl_sg_->addStretch();
    hbl_sg_->addWidget(lb_g_);
    hbl_sg_->addWidget(le_g_);

    hbl_vb_->addWidget(lb_v_);
    hbl_vb_->addWidget(le_v_);
    hbl_vb_->addStretch();

    hbl_vb_->addWidget(lb_b_);
    hbl_vb_->addWidget(le_b_);

    vbl_main_->addLayout(hbl_hr_);
    vbl_main_->addLayout(hbl_sg_);
    vbl_main_->addLayout(hbl_vb_);
    vbl_main_->addStretch();

    this->setLayout(vbl_main_);

    connect(le_r_, &QLineEdit::textEdited, this, [this](const QString& str) {
        const int number = str.toInt();
        int num_clamped  = std::clamp(number, 0x00, 0xFF);
        rgb_.r           = num_clamped;
        update_hsv();
        emit sgn_color_edited(hsv_);
    });

    connect(le_g_, &QLineEdit::textEdited, this, [this](const QString& str) {
        const int number = str.toInt();
        int num_clamped  = std::clamp(number, 0x00, 0xFF);
        rgb_.g           = num_clamped;
        update_hsv();
        emit sgn_color_edited(hsv_);
    });

    connect(le_b_, &QLineEdit::textEdited, this, [this](const QString& str) {
        const int number = str.toInt();
        int num_clamped  = std::clamp(number, 0x00, 0xFF);
        rgb_.b           = num_clamped;
        update_hsv();
        emit sgn_color_edited(hsv_);
    });

    connect(le_h_, &QLineEdit::textEdited, this, [this](const QString& str) {
        const double number = str.toInt();
        double num_clamped  = std::clamp(number, 0.0, 360.0);
        hsv_.h              = num_clamped;
        update_rgb();
        emit sgn_color_edited(hsv_);
    });

    connect(le_s_, &QLineEdit::textEdited, this, [this](const QString& str) {
        const double number = str.toInt();
        double num_clamped  = std::clamp(number, 0.0, 100.0);
        hsv_.s              = num_clamped;
        update_rgb();
        emit sgn_color_edited(hsv_);
    });

    connect(le_v_, &QLineEdit::textEdited, this, [this](const QString& str) {
        const double number = str.toInt();
        double num_clamped  = std::clamp(number, 0.0, 100.0);
        hsv_.v              = num_clamped;
        update_rgb();
        emit sgn_color_edited(hsv_);
    });
}

void LabelMatrix::set_hsv(hsv_t hsv)
{
    le_h_->setText(QString::number(hsv.h));
    le_s_->setText(QString::number(hsv.s));
    le_v_->setText(QString::number(hsv.v));

    rgb_t rgb = hsv_to_rgb(hsv);
    le_r_->setText(QString::number(rgb.r));
    le_g_->setText(QString::number(rgb.g));
    le_b_->setText(QString::number(rgb.b));
}

void LabelMatrix::update_rgb()
{
    rgb_ = hsv_to_rgb(hsv_);
}

void LabelMatrix::update_hsv()
{
    hsv_ = rgb_to_hsv(rgb_);
}
