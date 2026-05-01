#pragma once

#include <QByteArray>
#include <QList>
#include <QVector>
#include <QString>
#include <QUuid>
#include <QJsonObject>
#include "./IConfigurable.h"

namespace {
    const int kConfigVersion = 1;  // use after officially release
    static QString kFileName = "WusicPlayer.json";
};

class ConfigManager
{
public:
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static ConfigManager& getInstance();

    void registerModule(IConfigurable* module);
    void loadAll();
    QJsonObject saveAll() const;

    QJsonObject readSubConfig(const QString& key) const;
    void writeSubConfig(const QString& key, const QJsonObject &sub_obj);

    int m_version;
    QString m_filename;
private:
    ConfigManager();
    ~ConfigManager() = default;
    
    QString getConfigPath() const;
    QString getConfigFilepath() const;

    QVector<IConfigurable*> m_modules;
};