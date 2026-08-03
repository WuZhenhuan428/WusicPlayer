#pragma once

#include "core/config_manager/i_configurable.h"
#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUuid>
#include <QVector>

namespace
{
const int kConfigVersion = 1; // use after officially release
static QString kFileName = "WusicPlayer.json";
}; // namespace

class ConfigManager
{
public:
    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static ConfigManager& get_instance();

    void register_module(IConfigurable* module);
    void load_all();
    QJsonObject save_all() const;

    QJsonObject read_sub_config(const QString& key) const;
    void write_sub_config(const QString& key, const QJsonObject& sub_obj);

    int m_version;
    QString m_filename;

private:
    ConfigManager();
    ~ConfigManager() = default;

    QString get_config_path() const;
    QString get_config_filepath() const;

    QVector<IConfigurable*> m_modules;
};
