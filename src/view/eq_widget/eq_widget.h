#pragma once

#include "core/player_types.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <array>

class QSlider;
class QLabel;

class EQWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EQWidget(gains_t old_gains, bool enabled, bool instant, QWidget* parent = nullptr);
    ~EQWidget();

signals:
    void sgnGainChanged(gains_t gains);
    void sgnEqEnabledChanged(bool enabled);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void init_ui();
    void init_connections();

    gains_t array_to_gaint_t(std::array<float, 10> gains);

private:
    std::array<QSlider*, 10> m_sliders;
    std::array<QLabel*, 10> m_lb_values;
    std::array<QVBoxLayout*, 10> m_vbl_slider;
    std::array<QLabel*, 10> m_lb_freq;
    QHBoxLayout* m_hbl_sliders;

    QCheckBox* m_cb_enable_eq;
    QCheckBox* m_cb_instant_apply;
    QPushButton* m_btn_apply;
    QPushButton* m_btn_close;
    QPushButton* m_btn_reset;
    QHBoxLayout* m_hbl_control_bar;

    QVBoxLayout* m_vbl_main;

    std::array<float, 10> m_gains;

    bool m_enable_eq     = false;
    bool m_instant_apply = false;
    bool m_timer_hold    = false;
};
