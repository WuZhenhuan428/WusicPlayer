#include "eq_widget.h"
#include "core/player/config.h"

#include <QSlider>
#include <QLabel>
#include <QString>
#include <QTimer>

namespace
{
    const int SLIDER_NUMBER = 10;
} // namespace

EQWidget::EQWidget(gains_t old_gains, bool enabled, bool instant, QWidget* parent)
    : QWidget(parent)
{
    m_enable_eq = enabled;
    m_instant_apply = instant;
    m_gains = {old_gains._31, old_gains._63, old_gains._125, old_gains._250,
               old_gains._500, old_gains._1k, old_gains._2k, old_gains._4k,
               old_gains._8k, old_gains._16k};

    this->init_ui();
    this->init_connections();
}

EQWidget::~EQWidget()
{
}

/**
 * set slider range with [-1200, 1200], map to [-12, 12]
 * single step: 0.01, page step: 0.10
 */
void EQWidget::init_ui()
{
    m_hbl_sliders = new QHBoxLayout;

    for (int i = 0; i < SLIDER_NUMBER; ++i) {
        m_sliders[i] = nullptr;
        m_lb_values[i] = nullptr;
        m_vbl_slider[i] = nullptr;
        m_lb_freq[i] = nullptr;
    }

    m_lb_freq[0] = new QLabel("31Hz", this);
    m_lb_freq[1] = new QLabel("63Hz", this);
    m_lb_freq[2] = new QLabel("125Hz", this);
    m_lb_freq[3] = new QLabel("250Hz", this);
    m_lb_freq[4] = new QLabel("500Hz", this);
    m_lb_freq[5] = new QLabel("1kHz", this);
    m_lb_freq[6] = new QLabel("2kHz", this);
    m_lb_freq[7] = new QLabel("4kHz", this);
    m_lb_freq[8] = new QLabel("8kHz", this);
    m_lb_freq[9] = new QLabel("16kHz", this);

    for (int i = 0; i < SLIDER_NUMBER; ++i) {
        m_sliders[i] = new QSlider(Qt::Orientation::Vertical, this);
        m_sliders[i]->setMaximum((int)(EQ_UPPER_LIMIT_DB * 100));
        m_sliders[i]->setMinimum((int)(EQ_LOWER_LIMIT_DB * 100));
        m_sliders[i]->setSingleStep(1);
        m_sliders[i]->setPageStep(10);
        m_sliders[i]->setValue((int)(m_gains[i] * 100));
        m_sliders[i]->setTracking(true);

        m_lb_values[i] = new QLabel(QString::number(m_gains[i], 'f', 2), this);
        m_vbl_slider[i] = new QVBoxLayout;

        m_vbl_slider[i]->addWidget(m_lb_values[i], 0, Qt::AlignHCenter);
        m_vbl_slider[i]->addWidget(m_sliders[i], 0, Qt::AlignHCenter);
        m_vbl_slider[i]->addWidget(m_lb_freq[i], 0, Qt::AlignHCenter);

        m_hbl_sliders->addLayout(m_vbl_slider[i]);

        connect(m_sliders[i], &QSlider::valueChanged, this, [this, i](int value){
            m_gains[i] = value / 100.0;
            m_lb_values[i]->setText(QString::number(m_gains[i], 'f', 2));
            if (m_instant_apply && m_enable_eq && !m_timer_hold) {
                emit this->sgnGainChanged(array_to_gaint_t(m_gains));
                m_timer_hold = true;
                QTimer::singleShot(30, [this](){
                    m_timer_hold = false;
                });
            }
        });
    }

    m_cb_enable_eq = new QCheckBox("Enable EQ", this);
    m_cb_instant_apply = new QCheckBox("Instant apply", this);
    m_btn_reset = new QPushButton("Reset", this);
    m_btn_apply = new QPushButton("Apply", this);
    m_btn_close = new QPushButton("Close", this);
    m_hbl_control_bar = new QHBoxLayout;

    m_cb_enable_eq->setChecked(m_enable_eq);
    m_cb_instant_apply->setChecked(m_instant_apply);

    m_hbl_control_bar->addWidget(m_cb_enable_eq);
    m_hbl_control_bar->addWidget(m_cb_instant_apply);
    m_hbl_control_bar->addStretch();
    m_hbl_control_bar->addWidget(m_btn_reset);
    m_hbl_control_bar->addWidget(m_btn_apply);
    m_hbl_control_bar->addWidget(m_btn_close);

    m_vbl_main = new QVBoxLayout;
    m_vbl_main->addLayout(m_hbl_sliders);
    m_vbl_main->addLayout(m_hbl_control_bar);

    this->setLayout(m_vbl_main);
}

void EQWidget::init_connections()
{
    connect(m_cb_enable_eq, &QCheckBox::toggled, this, [this](bool checked){
        m_enable_eq = checked;
        emit sgnEqEnabledChanged(checked);
        if (checked) {
            emit sgnGainChanged(array_to_gaint_t(m_gains));
        } else {
            emit sgnGainChanged(gains_t{});
        }
    });
    connect(m_cb_instant_apply, &QCheckBox::toggled, this, [this](bool checked){
        m_instant_apply = checked;
    });
    connect(m_btn_reset, &QPushButton::clicked, this, [this](){
        for (int i = 0; i < SLIDER_NUMBER; i++) {
            m_sliders[i]->setValue(0);
            m_gains[i] = 0.0f;
            m_lb_values[i]->setText("0.00");
        }
        emit sgnGainChanged(array_to_gaint_t(m_gains));
    });
    connect(m_btn_apply, &QPushButton::clicked, this, [this](){
        for (int i = 0; i < SLIDER_NUMBER; ++i) {
            m_gains[i] = m_sliders[i]->value() / 100.0f;
        }
        emit sgnGainChanged(array_to_gaint_t(m_gains));
    });
    connect(m_btn_close, &QPushButton::clicked, this, [this](){
        this->close();
    });
}

gains_t EQWidget::array_to_gaint_t(std::array<float, 10> gains)
{
    gains_t gain = {gains[0], gains[1], gains[2],
                    gains[3], gains[4], gains[5],
                    gains[6], gains[7], gains[8], gains[9]};
    return gain;
}