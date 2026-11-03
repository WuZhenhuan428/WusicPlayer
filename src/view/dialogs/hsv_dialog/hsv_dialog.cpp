#include "view/dialogs/hsv_dialog/hsv_dialog.h"

#include <QStyle>

HSVDialog::HSVDialog(rgb_t curr_rgb, QWidget* parent) : QDialog(parent)
{
    current_color_   = curr_rgb;

    int outer_radius = 100;
    int label_width  = 200;
    hsv_palette_     = new HSVPalette(static_cast<int>(outer_radius * 0.75), outer_radius, this);
    color_preview_   = new ColorPreviewItem(60, (outer_radius * 2), this);
    vbl_color_       = new QVBoxLayout;
    vbl_color_->addWidget(hsv_palette_);
    vbl_color_->addWidget(color_preview_);

    lb_matrix_   = new LabelMatrix(this);
    hbl_palette_ = new QHBoxLayout;
    hbl_palette_->addLayout(vbl_color_);
    hbl_palette_->addStretch();
    hbl_palette_->addWidget(lb_matrix_);

    btn_apply_ = new QPushButton("Apply", this);
    btn_apply_->setIcon((this->style()->standardIcon(QStyle::SP_DialogApplyButton)));
    btn_ok_ = new QPushButton("OK", this);
    btn_ok_->setIcon((this->style()->standardIcon(QStyle::SP_DialogOkButton)));
    btn_cancel_ = new QPushButton("Cancel", this);
    btn_cancel_->setIcon((this->style()->standardIcon(QStyle::SP_DialogCancelButton)));
    bottom_layout_ = new QHBoxLayout;
    bottom_layout_->addStretch();
    bottom_layout_->addWidget(btn_apply_);
    bottom_layout_->addWidget(btn_ok_);
    bottom_layout_->addWidget(btn_cancel_);

    main_layout_ = new QVBoxLayout;
    main_layout_->addLayout(hbl_palette_);
    main_layout_->addLayout(bottom_layout_);

    this->setLayout(main_layout_);
    this->setMinimumSize((outer_radius * 2) + label_width, (outer_radius * 2) + 60);

    hsv_palette_->set_hsv(hsv_t{0.0f, 50.0f, 50.0f});
    hsv_palette_->set_rgb(current_color_);
    color_preview_->set_initial_color(current_color_);

    connect(hsv_palette_, &HSVPalette::sgn_hsv_changed, lb_matrix_, &LabelMatrix::set_hsv);
    connect(lb_matrix_, &LabelMatrix::sgn_color_edited, hsv_palette_, &HSVPalette::set_hsv);
    connect(hsv_palette_, &HSVPalette::sgn_mouse_moving_color, color_preview_,
            &ColorPreviewItem::update_curr_color);
    connect(hsv_palette_, &HSVPalette::sgn_mouse_release_color, color_preview_,
            &ColorPreviewItem::update_color);

    connect(btn_apply_, &QPushButton::clicked, this,
            [this]() { current_color_ = hsv_palette_->get_rgb(); });
    connect(btn_ok_, &QPushButton::clicked, this, [this]() {
        current_color_ = hsv_palette_->get_rgb();
        this->accept();
    });
    connect(btn_cancel_, &QPushButton::clicked, this, [this]() { this->reject(); });
}

rgb_t HSVDialog::get_color()
{
    return current_color_;
}
