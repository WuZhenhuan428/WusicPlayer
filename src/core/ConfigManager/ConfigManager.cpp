#include "ConfigManager.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QJsonObject>
#include <QSaveFile>

ConfigManager::ConfigManager() {}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

QString ConfigManager::getConfigPath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString ConfigManager::getConfigFilepath() const {
    QDir dir(getConfigPath());
    return dir.filePath(kFileName);
}

void ConfigManager::registerSection(IConfigSection* s) {
    m_sections.push_back(s);
}

void ConfigManager::loadAll() {
// check file
    const QString folder_path = getConfigPath();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QFile file(getConfigFilepath());
    if (!file.exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[CONFIG] open failed:" << file.fileName() << file.errorString();
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[CONFIG] parse failed:" << err.errorString();
        return;
    }

    const QJsonObject root = doc.object();

    this->m_version = root.value("version").toInt(kConfigVersion);

    for (const auto& section : m_sections) {
        section->load(root);
    }
}

QJsonObject ConfigManager::saveAll() const {
    const QString folder_path = getConfigPath();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QJsonObject root;
    // meta QJsonObject
    root.insert("filename", kFileName);
    root.insert("version", kConfigVersion);

    for (const auto& sec : m_sections) {
        root.insert(sec->key(), sec->save());
    }

    QSaveFile file(getConfigFilepath());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[CONFIG] write open failed:" << file.fileName() << file.errorString();
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qWarning() << "[CONFIG] commit failed:" << file.fileName() << file.errorString();
    }
    return root;
}
