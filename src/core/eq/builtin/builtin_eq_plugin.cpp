#include "core/eq/builtin/builtin_eq_plugin.h"

#include "core/player/config.h" // kEqUpperLimitDB / kEqLowerLimitDB

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include <numbers>

namespace
{
constexpr double kEqFreqs[10]  = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
const char* kEqFreqStrings[10] = {"31", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"};
} // namespace

QString BuiltinEqPlugin::id() const
{
    return QStringLiteral("wusic.eq.builtin");
}

QString BuiltinEqPlugin::name() const
{
    return QStringLiteral("Builtin EQ (10-band)");
}

QString BuiltinEqPlugin::version() const
{
    return QStringLiteral("1.0.0");
}

QString BuiltinEqPlugin::description() const
{
    return QStringLiteral("10-band equalizer, ±12 dB per band");
}

QString BuiltinEqPlugin::author() const
{
    return QStringLiteral("WusicPlayer");
}

QVector<QString> BuiltinEqPlugin::categories() const
{
    return {QStringLiteral("eq")};
}

QWidget* BuiltinEqPlugin::create_eq_widget(QWidget* parent)
{
    auto* w   = new QWidget(parent);
    auto* hbl = new QHBoxLayout(w);
    hbl->setContentsMargins(4, 4, 4, 4);

    const int range = static_cast<int>(kEqUpperLimitDB * 100.0); // ±120 单位 = ±12dB

    for (int i = 0; i < 10; ++i) {
        auto* vbl     = new QVBoxLayout;
        auto* lb_freq = new QLabel(kEqFreqStrings[i], w);
        lb_freq->setAlignment(Qt::AlignHCenter);
        auto* lb_val = new QLabel(QStringLiteral("0 dB"), w);
        lb_val->setAlignment(Qt::AlignHCenter);
        auto* slider = new QSlider(Qt::Vertical, w);
        slider->setRange(-range, range);
        slider->setValue(static_cast<int>(gains_[i] * 100.0f));

        vbl->addWidget(lb_val, 0, Qt::AlignHCenter);
        vbl->addWidget(slider, 1, Qt::AlignHCenter);
        vbl->addWidget(lb_freq, 0, Qt::AlignHCenter);
        hbl->addLayout(vbl);

        sliders_[i]   = slider;
        lb_values_[i] = lb_val;

        connect(slider, &QSlider::valueChanged, this, [this, i](int v) {
            gains_[i] = static_cast<float>(v) / 100.0f;
            lb_values_[i]->setText(QStringLiteral("%1 dB").arg(v / 100.0));
            emit sgn_config_changed();
        });
    }
    return w;
}

EqConfig BuiltinEqPlugin::eq_config() const
{
    EqConfig cfg;
    cfg.enabled = true;
    for (int i = 0; i < 10; ++i) {
        // q≈1.414 对应 1 octave(与原 width_type=o:width=1 听感一致)
        cfg.bands.push_back(
            EqBand{EqFilterType::Parametric, kEqFreqs[i], std::numbers::sqrt2, gains_[i]});
    }
    return cfg;
}

void BuiltinEqPlugin::apply_current()
{
    gains_applied_ = gains_;
}

void BuiltinEqPlugin::revert()
{
    gains_ = gains_applied_;
    this->sync_sliders();
}

void BuiltinEqPlugin::reset()
{
    gains_.fill(0.0f);
    gains_applied_.fill(0.0f);
    this->sync_sliders();
}

void BuiltinEqPlugin::restore_from_config(const EqConfig& cfg)
{
    gains_.fill(0.0f);
    for (int i = 0; i < 10 && i < cfg.bands.size(); ++i) {
        gains_[i] = static_cast<float>(cfg.bands[i].gain_db);
    }
    gains_applied_ = gains_; // 恢复值视为已应用, Cancel 不回退到全 0
    this->sync_sliders();
}

void BuiltinEqPlugin::sync_sliders()
{
    for (int i = 0; i < 10; ++i) {
        if (sliders_[i]) {
            sliders_[i]->setValue(static_cast<int>(gains_[i] * 100.0f));
        }
    }
}
