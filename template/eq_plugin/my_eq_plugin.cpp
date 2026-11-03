#include "my_eq_plugin.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <numbers>

namespace
{
constexpr int kRange100 = 1200; // ±12dB, 单位 0.01dB
} // namespace

QString MyEqPlugin::id() const
{
    return QStringLiteral("com.wusicplayer.eq.myeq");
}

QString MyEqPlugin::name() const
{
    return QStringLiteral("My EQ");
}

QString MyEqPlugin::version() const
{
    return QStringLiteral("1.0");
}

QString MyEqPlugin::description() const
{
    return QStringLiteral("A sample EQ plugin (3 bands)");
}

QString MyEqPlugin::author() const
{
    return QStringLiteral("Your Name");
}

QVector<QString> MyEqPlugin::categories() const
{
    return {QStringLiteral("eq")};
}

QWidget* MyEqPlugin::create_eq_widget(QWidget* parent)
{
    auto* w   = new QWidget(parent);
    auto* hbl = new QHBoxLayout(w);
    hbl->setContentsMargins(4, 4, 4, 4);

    for (int i = 0; i < kBandCount; ++i) {
        auto* vbl     = new QVBoxLayout;
        auto* lb_freq = new QLabel(QString::number(kFreqs[i]), w);
        lb_freq->setAlignment(Qt::AlignHCenter);
        auto* lb_val = new QLabel(QStringLiteral("0 dB"), w);
        lb_val->setAlignment(Qt::AlignHCenter);
        auto* slider = new QSlider(Qt::Vertical, w);
        slider->setRange(-kRange100, kRange100);
        slider->setValue(static_cast<int>(gains_[i] * 100.0f));

        vbl->addWidget(lb_val);
        vbl->addWidget(slider, 1);
        vbl->addWidget(lb_freq);
        hbl->addLayout(vbl);

        sliders_[i]   = slider;
        lb_values_[i] = lb_val;

        connect(slider, &QSlider::valueChanged, this, [this, i](int v) {
            gains_[i] = static_cast<float>(v) / 100.0f;
            lb_values_[i]->setText(QStringLiteral("%1 dB").arg(v / 100.0));
            emit sgn_config_changed(); // 即时模式下固定窗口据此推送后端
        });
    }
    return w;
}

EqConfig MyEqPlugin::eq_config() const
{
    EqConfig cfg;
    cfg.enabled = true;
    for (int i = 0; i < kBandCount; ++i) {
        // 示例: 三段 Parametric, Q≈1.414(1 octave)
        cfg.bands.push_back(
            EqBand{EqFilterType::Parametric, kFreqs[i], std::numbers::sqrt2, gains_[i]});
    }
    return cfg;
}

void MyEqPlugin::apply_current()
{
    gains_applied_ = gains_;
}

void MyEqPlugin::revert()
{
    gains_ = gains_applied_;
    this->sync_sliders();
}

void MyEqPlugin::reset()
{
    gains_.fill(0.0f);
    gains_applied_.fill(0.0f);
    this->sync_sliders();
}

void MyEqPlugin::restore_from_config(const EqConfig& cfg)
{
    gains_.fill(0.0f);
    for (int i = 0; i < kBandCount && i < cfg.bands.size(); ++i) {
        gains_[i] = static_cast<float>(cfg.bands[i].gain_db);
    }
    gains_applied_ = gains_; // 恢复值视为已应用, Cancel 不回退到全 0
    this->sync_sliders();
}

void MyEqPlugin::sync_sliders()
{
    for (int i = 0; i < kBandCount; ++i) {
        if (sliders_[i]) {
            sliders_[i]->setValue(static_cast<int>(gains_[i] * 100.0f));
        }
    }
}
