#include "core/eq/builtin/builtin_eq_plugin.h"

#include "core/player/config.h" // EQ_UPPER_LIMIT_DB / EQ_LOWER_LIMIT_DB

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
constexpr double kEqFreqs[10] = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
} // namespace

BuiltinEqPlugin::BuiltinEqPlugin() = default;

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

    const int range = static_cast<int>(EQ_UPPER_LIMIT_DB * 100.0); // ±120 单位 = ±12dB

    for (int i = 0; i < 10; ++i) {
        auto* vbl     = new QVBoxLayout;
        auto* lb_freq = new QLabel(QString::number(kEqFreqs[i]), w);
        lb_freq->setAlignment(Qt::AlignHCenter);
        auto* lb_val = new QLabel(QStringLiteral("0 dB"), w);
        lb_val->setAlignment(Qt::AlignHCenter);
        auto* slider = new QSlider(Qt::Vertical, w);
        slider->setRange(-range, range);
        slider->setValue(static_cast<int>(m_gains[i] * 100.0f));

        vbl->addWidget(lb_freq);
        vbl->addWidget(lb_val);
        vbl->addWidget(slider, 1);
        hbl->addLayout(vbl);

        m_sliders[i]   = slider;
        m_lb_values[i] = lb_val;

        connect(slider, &QSlider::valueChanged, this, [this, i](int v) {
            m_gains[i] = static_cast<float>(v) / 100.0f;
            m_lb_values[i]->setText(QStringLiteral("%1 dB").arg(v / 100.0));
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
        cfg.bands.push_back(EqBand{EqFilterType::Parametric, kEqFreqs[i], 1.414, m_gains[i]});
    }
    return cfg;
}

void BuiltinEqPlugin::apply_current()
{
    m_gains_applied = m_gains;
}

void BuiltinEqPlugin::revert()
{
    m_gains = m_gains_applied;
    this->sync_sliders();
}

void BuiltinEqPlugin::sync_sliders()
{
    for (int i = 0; i < 10; ++i) {
        if (m_sliders[i]) {
            m_sliders[i]->setValue(static_cast<int>(m_gains[i] * 100.0f));
        }
    }
}
