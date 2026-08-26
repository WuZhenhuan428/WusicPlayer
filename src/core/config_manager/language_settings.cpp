#include "core/config_manager/language_settings.h"

#include <QLocale>

namespace
{
/// 按系统语言推断默认 locale(仅支持中文/英文, 其余回退英文)
QString detect_system_locale()
{
    const QLocale sys = QLocale::system();
    if (sys.language() == QLocale::Chinese) {
        return QStringLiteral("zh_CN");
    }
    return QStringLiteral("en_US");
}
} // namespace

LanguageSettings& LanguageSettings::instance()
{
    static LanguageSettings s_instance;
    return s_instance;
}

LanguageSettings::LanguageSettings(QObject* parent) :
    QObject(parent), m_locale(detect_system_locale())
{}

QString LanguageSettings::locale() const
{
    return m_locale;
}

void LanguageSettings::set_locale(const QString& locale)
{
    if (!locale.isEmpty() && locale != m_locale) {
        m_locale = locale;
    }
}

void LanguageSettings::load_from_json(const QJsonObject& json)
{
    const QJsonObject obj = json.value(this->config_sub_key()).toObject();
    const QString loc     = obj.value("locale").toString();
    if (!loc.isEmpty()) {
        m_locale = loc;
    }
}

QJsonObject LanguageSettings::save_to_json()
{
    QJsonObject obj;
    QJsonObject sub;
    sub["locale"]               = m_locale;
    obj[this->config_sub_key()] = sub;
    return obj;
}

QString LanguageSettings::config_sub_key() const
{
    return QStringLiteral("language");
}
