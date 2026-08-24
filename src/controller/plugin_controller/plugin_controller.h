#pragma once

#include "core/config_manager/i_configurable.h"
#include "core/plugin_manager/plugin_manager.h"
#include "core/plugin_manager/plugin_types.h"

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

class PluginController : public QObject, public IConfigurable
{
    Q_OBJECT

public:
    explicit PluginController(QObject* parent = nullptr);

    void import();
    void remove(const QString& id);

    /// 扫描目录下的插件(递归, 经 PluginManager)
    void scan_directory(const QString& dir);

    /// 注册内建插件(生命周期由调用方管理, 仅登记)
    void register_builtin(IBasicPlugin* plugin);

    /// 暴露底层 PluginManager(供组合根注入 ThemeManager 等)
    PluginManager* plugin_manager() const;

    const QVector<PluginDescriptor> descriptors();

    // ---- IConfigurable: 持久化外部插件路径, 启动时恢复 ----
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

signals:
    void sgn_data_updated();

private:
    std::unique_ptr<PluginManager> plugin_manager_ = nullptr;
};
