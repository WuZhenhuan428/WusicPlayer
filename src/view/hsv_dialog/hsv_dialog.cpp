#include "hsv_dialog.h"
#include "hsv_to_rgb.h"

#include <QStyle>

HSVDialog::HSVDialog(rgb_t curr_rgb, QWidget *parent)
    : QDialog(parent)
{

    m_current_color = curr_rgb;

    int outer_radius = 100;
    int label_width = 200;
    m_hsv_palette = new HSVPalette((int)(outer_radius * 0.75), outer_radius, this);
    m_color_preview = new ColorPreviewItem(60, outer_radius * 2, this);
    m_color_layout = new QVBoxLayout;
    m_color_layout->addWidget(m_hsv_palette);
    m_color_layout->addWidget(m_color_preview);
    
    m_label_matrix = new LabelMatrix(this);
    m_palette_layout = new QHBoxLayout;
    m_palette_layout->addLayout(m_color_layout);
    m_palette_layout->addStretch();
    m_palette_layout->addWidget(m_label_matrix);

    m_btn_apply = new QPushButton("Apply", this);
    m_btn_apply->setIcon((this->style()->standardIcon(QStyle::SP_DialogApplyButton)));
    m_btn_ok = new QPushButton("OK", this);
    m_btn_ok->setIcon((this->style()->standardIcon(QStyle::SP_DialogOkButton)));
    m_btn_cancel = new QPushButton("Cancel", this);
    m_btn_cancel->setIcon((this->style()->standardIcon(QStyle::SP_DialogCancelButton)));
    m_bottom_layout = new QHBoxLayout;
    m_bottom_layout->addStretch();
    m_bottom_layout->addWidget(m_btn_apply);
    m_bottom_layout->addWidget(m_btn_ok);
    m_bottom_layout->addWidget(m_btn_cancel);
    
    m_main_layout = new QVBoxLayout;
    m_main_layout->addLayout(m_palette_layout);
    m_main_layout->addLayout(m_bottom_layout);

    
    this->setLayout(m_main_layout);
    this->setMinimumSize(outer_radius*2 + label_width, outer_radius*2 + 60);

    m_hsv_palette->setHsv(hsv_t{0.0f, 50.0f, 50.0f});
    m_hsv_palette->setRgb(m_current_color);
    m_color_preview->setInitialColor(m_current_color);


    connect(m_hsv_palette, &HSVPalette::sgnHSVChanged, m_label_matrix, &LabelMatrix::setHSV);
    connect(m_label_matrix, &LabelMatrix::sgnEditColor, m_hsv_palette, &HSVPalette::setHsv);
    connect(m_hsv_palette, &HSVPalette::sgnMouseMovingColor, m_color_preview, &ColorPreviewItem::updateCurrColor);
    connect(m_hsv_palette, &HSVPalette::sgnMouseReleaseColor, m_color_preview, &ColorPreviewItem::updateColor);
    connect(m_btn_apply, &QPushButton::clicked, this, [this](){
        sgnSelectColor(m_hsv_palette->getRgb());
    });
    connect(m_btn_ok, &QPushButton::clicked, this, [this](){
        sgnSelectColor(m_hsv_palette->getRgb());
        this->close();
    });
    connect(m_btn_cancel, &QPushButton::clicked, this, [this](){
        this->close();
    });
}

HSVDialog::~HSVDialog() {}
