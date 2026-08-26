#pragma once

#include "core/config_manager/i_configurable.h"

#include <QJsonObject>
#include <QObject>
#include <QString>

/// 界面语言设置(单例, 持久化)。
/// locale 取值如 "en_US"(默认) / "zh_CN"; 切换后需重启生效。
class LanguageSettings : public QObject, public IConfigurable
{
    Q_OBJECT
public:
    static LanguageSettings& instance();

    QString locale() const;
    void set_locale(const QString& locale);

    // ---- IConfigurable ----
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

private:
    explicit LanguageSettings(QObject* parent = nullptr);

    QString m_locale;
};
