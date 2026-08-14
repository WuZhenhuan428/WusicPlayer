#include "core/config_manager/config_manager.h"

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("config", {"console", "gui"});
}

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
QJsonObject readRootFromFile(const QString& filepath)
{
    QFile file(filepath);
    if (!file.exists()) {
        return {};
    }
    if (!file.open(QIODevice::ReadOnly)) {
        logger->warn("open failed: {} {}", file.fileName(), file.errorString());
        return {};
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        logger->warn("parse failed: {}", err.errorString());
        return {};
    }
    return doc.object();
}

void writeRootToFile(const QString& filepath, const QJsonObject& root)
{
    QSaveFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        logger->warn("write open failed: {} {}", file.fileName(), file.errorString());
        return;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        logger->warn("commit failed: {} {}", file.fileName(), file.errorString());
    }
}
} // namespace

ConfigManager::ConfigManager() {}

ConfigManager& ConfigManager::get_instance()
{
    static ConfigManager instance;
    return instance;
}

QString ConfigManager::get_config_path() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString ConfigManager::get_config_filepath() const
{
    QDir dir(get_config_path());
    return dir.filePath(kFileName);
}

void ConfigManager::register_module(IConfigurable* module)
{
    m_modules.push_back(module);
}

void ConfigManager::load_all()
{
    // check file
    const QString folder_path = get_config_path();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(get_config_filepath());
    if (!file.exists()) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        logger->warn("open failed: {} {}", file.fileName(), file.errorString());
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        logger->warn("parse failed: {}", err.errorString());
        return;
    }

    const QJsonObject root = doc.object();

    this->m_version        = root.value("version").toInt(kConfigVersion);

    for (const auto& module : m_modules) {
        if (module) {
            module->load_from_json(root);
        }
    }
}

QJsonObject ConfigManager::save_all() const
{
    const QString folder_path = get_config_path();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QJsonObject root = readRootFromFile(get_config_filepath());
    root.insert("filename", kFileName);
    root.insert("version", kConfigVersion);

    for (const auto& module : m_modules) {
        if (module) {
            root.insert(module->config_sub_key(), module->save_to_json());
        }
    }

    writeRootToFile(get_config_filepath(), root);
    return root;
}

QJsonObject ConfigManager::read_sub_config(const QString& key) const
{
    if (key.isEmpty()) {
        return {};
    }
    const QJsonObject root = readRootFromFile(get_config_filepath());
    return root.value(key).toObject();
}

void ConfigManager::write_sub_config(const QString& key, const QJsonObject& sub_obj)
{
    if (key.isEmpty()) {
        return;
    }

    const QString folder_path = get_config_path();
    QDir dir(folder_path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QJsonObject root = readRootFromFile(get_config_filepath());
    root.insert("filename", kFileName);
    root.insert("version", kConfigVersion);

    for (const auto& module : m_modules) {
        if (module) {
            root.insert(module->config_sub_key(), module->save_to_json());
        }
    }

    root.insert(key, sub_obj);
    writeRootToFile(get_config_filepath(), root);
}
