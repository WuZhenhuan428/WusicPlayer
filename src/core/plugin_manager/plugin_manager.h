#pragma once

#include "core/plugin_manager/plugin_types.h"
#include "plugin/i_basic_plugin.h"

#include <QJsonObject>
#include <QObject>
#include <QPluginLoader>
#include <QString>
#include <QVector>

#include <memory>
#include <unordered_map>

class PluginManager : public QObject
{
    Q_OBJECT

public:
    explicit PluginManager(QObject* parent = nullptr);

    /// 加载外部插件, PluginManager 持有资源
    bool load_plugin(const QString& path);

    /// 内建插件无需加载, 生命周期由 AppController(或其他创建位置)管理, 注册时仅获取实例指针(非持有)
    void register_builtin(IBasicPlugin* plugin);

    /// 检测输入目录下存在的插件, 默认递归
    void scan_directory(const QString& dir);

    /// 移除插件: 外部插件卸载(QPluginLoader 析构→unload);
    /// 内建插件仅从登记表移除, 对象仍由创建方管理。
    bool remove(const QString& id);

    /// 用于更新视图信息
    const QVector<PluginDescriptor> descriptors() const;

    /// 外部插件文件路径(用于持久化; 内建插件不在其中)
    QVector<QString> external_plugin_paths() const;

    /// 外部调用时仅传出指针, 不转移所有权
    QVector<IBasicPlugin*> all() const;

    template <typename T>
    QVector<T*> plugins() const
    {
        QVector<T*> out;
        for (const auto& [_, handle] : handles_) {
            if (auto* t = qobject_cast<T*>(handle->interface)) {
                out.push_back(t);
            }
        }
        return out;
    }

    template <typename T>
    T* plugin(const QString& id) const
    {
        const auto& res = handles_.find(id);
        if (res != handles_.end()) {
            return qobject_cast<T*>(res->second->interface);
        }
        return nullptr;
    }

private:
    PluginDescriptor resolve_descriptor(IBasicPlugin* plugin, const QJsonObject& json);

    // QHash 是 CoW, unique_ptr 不可拷贝, 故使用 std::unordered_map
    std::unordered_map<QString /*id*/, std::unique_ptr<PluginHandle>> handles_;
};
