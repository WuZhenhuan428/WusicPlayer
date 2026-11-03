#include "view/settings_panel/language_settings_page.h"

#include "core/config_manager/language_settings.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

LanguageSettingsPage::LanguageSettingsPage(LanguageSettings* settings, QWidget* parent) :
    QWidget(parent), settings_(settings)
{
    cb_language_ = new QComboBox(this);
    cb_language_->addItem(QStringLiteral("English"), QStringLiteral("en_US"));
    cb_language_->addItem(QStringLiteral("简体中文"), QStringLiteral("zh_CN"));
    const int idx = cb_language_->findData(settings_->locale());
    if (idx >= 0) {
        cb_language_->setCurrentIndex(idx);
    }

    lb_hint_ = new QLabel(tr("Language change takes effect after restart."), this);
    lb_hint_->setWordWrap(true);

    auto* hbl = new QHBoxLayout;
    hbl->addWidget(new QLabel(tr("Language:"), this));
    hbl->addWidget(cb_language_);
    hbl->addStretch();

    auto* vbl = new QVBoxLayout(this);
    vbl->addLayout(hbl);
    vbl->addWidget(lb_hint_);
    vbl->addStretch();

    connect(cb_language_, &QComboBox::currentIndexChanged, this, [this](int) {
        settings_->set_locale(cb_language_->currentData().toString());
        QMessageBox::information(this, tr("Language"),
                                 tr("Language change takes effect after restart."));
    });
}

QListWidgetItem* LanguageSettingsPage::get_title_item()
{
    if (!list_title_) {
        list_title_ = new QListWidgetItem(tr("Language"));
    }
    return list_title_;
}
