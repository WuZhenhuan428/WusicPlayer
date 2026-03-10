#pragma once

#include <QByteArray>
#include <QList>
#include <QVector>
#include <QString>
#include <QUuid>
#include "core/types.h"
#include "view/ConfigBinder/IConfigSection.hpp"

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

    void registerSection(IConfigSection* s);
    void loadAll();
    QJsonObject saveAll() const;

    int m_version;
    QString m_filename;
private:
    ConfigManager();
    ~ConfigManager() = default;
    
    QString getConfigPath() const;
    QString getConfigFilepath() const;

    QVector<IConfigSection*> m_sections;
};