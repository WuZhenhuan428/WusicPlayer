#include "ConfigManager.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QJsonObject>
#include <QSaveFile>

namespace {
QJsonObject readRootFromFile(const QString& filepath)
{
    QFile file(filepath);
    if (!file.exists()) {
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[CONFIG] open failed:" << file.fileName() << file.errorString();
        return {};
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[CONFIG] parse failed:" << err.errorString();
        return {};
    }
    return doc.object();
}

void writeRootToFile(const QString& filepath, const QJsonObject& root)
{
    QSaveFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[CONFIG] write open failed:" << file.fileName() << file.errorString();
        return;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qWarning() << "[CONFIG] commit failed:" << file.fileName() << file.errorString();
    }
}
}

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

void ConfigManager::registerModule(IConfigurable* module) {
    m_modules.push_back(module);
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

    for (const auto& module : m_modules) {
        if (module) {
            module->loadFromJson(root);
        }
    }
}

QJsonObject ConfigManager::saveAll() const {
    const QString folder_path = getConfigPath();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QJsonObject root = readRootFromFile(getConfigFilepath());
    root.insert("filename", kFileName);
    root.insert("version", kConfigVersion);

    for (const auto& module : m_modules) {
        if (module) {
            root.insert(module->configSubKey(), module->saveToJson());
        }
    }

    writeRootToFile(getConfigFilepath(), root);
    return root;
}

QJsonObject ConfigManager::readSubConfig(const QString& key) const
{
    if (key.isEmpty()) {
        return {};
    }
    const QJsonObject root = readRootFromFile(getConfigFilepath());
    return root.value(key).toObject();
}

void ConfigManager::writeSubConfig(const QString& key, const QJsonObject &sub_obj)
{
    if (key.isEmpty()) {
        return;
    }

    const QString folder_path = getConfigPath();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QJsonObject root = readRootFromFile(getConfigFilepath());
    root.insert("filename", kFileName);
    root.insert("version", kConfigVersion);

    for (const auto& module : m_modules) {
        if (module) {
            root.insert(module->configSubKey(), module->saveToJson());
        }
    }

    root.insert(key, sub_obj);
    writeRootToFile(getConfigFilepath(), root);
}
