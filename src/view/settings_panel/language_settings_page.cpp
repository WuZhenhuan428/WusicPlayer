#include "view/settings_panel/language_settings_page.h"

#include "core/config_manager/language_settings.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

LanguageSettingsPage::LanguageSettingsPage(LanguageSettings* settings, QWidget* parent) :
    QWidget(parent), m_settings(settings)
{
    m_cb_language = new QComboBox(this);
    m_cb_language->addItem(QStringLiteral("English"), QStringLiteral("en_US"));
    m_cb_language->addItem(QStringLiteral("简体中文"), QStringLiteral("zh_CN"));
    const int idx = m_cb_language->findData(m_settings->locale());
    if (idx >= 0) {
        m_cb_language->setCurrentIndex(idx);
    }

    m_lb_hint = new QLabel(tr("Language change takes effect after restart."), this);
    m_lb_hint->setWordWrap(true);

    auto* hbl = new QHBoxLayout;
    hbl->addWidget(new QLabel(tr("Language:"), this));
    hbl->addWidget(m_cb_language);
    hbl->addStretch();

    auto* vbl = new QVBoxLayout(this);
    vbl->addLayout(hbl);
    vbl->addWidget(m_lb_hint);
    vbl->addStretch();

    connect(m_cb_language, &QComboBox::currentIndexChanged, this, [this](int) {
        m_settings->set_locale(m_cb_language->currentData().toString());
        QMessageBox::information(this, tr("Language"),
                                 tr("Language change takes effect after restart."));
    });
}

QListWidgetItem* LanguageSettingsPage::get_title_item()
{
    if (!m_title) {
        m_title = new QListWidgetItem(tr("Language"));
    }
    return m_title;
}
