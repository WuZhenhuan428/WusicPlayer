#pragma once

#include "core/config_manager/i_configurable.h"
#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUuid>
#include <QVector>

class ConfigManager
{
public:
    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&)                 = default;
    ConfigManager& operator=(ConfigManager&&)      = default;

    static ConfigManager& get_instance();

    void register_module(IConfigurable* module);
    void load_all();

    // note: save_all 改变了外部文件但是不改变对象自身, 同时返回配置文件的 json 根
    //       因此不适用 [[nodiscard]]
    QJsonObject save_all() const; // NOLINT

    [[nodiscard]] static QJsonObject read_sub_config(const QString& key);
    void write_sub_config(const QString& key, const QJsonObject& sub_obj);

private:
    static constexpr int kConfigVersion = 1; // use after officially release
    ConfigManager()                     = default;
    ~ConfigManager()                    = default;

    [[nodiscard]] static QString get_config_path();
    [[nodiscard]] static QString get_config_filepath();

    QString filename_;
    int version_ = kConfigVersion;
    QVector<IConfigurable*> modules_;
};
