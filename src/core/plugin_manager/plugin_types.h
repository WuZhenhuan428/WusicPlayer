#pragma once

#include "plugin/i_basic_plugin.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

#include <QObject>
#include <QPluginLoader>

#include <memory>

struct PluginDescriptor
{
    QString id;                  // 唯一标识符
    QString name;                // 显示名称
    QString version;             // 版本号
    QString description;         // 描述
    QString author;              // 作者
    QVector<QString> categories; // 分类标签
};

/// - QPluginLoader::metaData() 返回的 Json 可能包含以下键值:
///   "IID", "MetaData", "className", "debug", "version"
/// - 此处提取 iid 与 metadata 作为缓存使用, 并解析 & 保存为 PluginDescriptor
/// - 不存在 metaData() 时, 通过基类插件的接口获取信息, 例如用于内置插件,
///   此时 iid 与 metadata 为空
/// - 外部调用时, 仅通过 instance / interface / descriptor
/// - (待定) 约定 iid 与 id 相同, 不同时的校验机制?
struct PluginHandle
{
    /// - 内建时为空, 直接调用 instance / interface
    /// - 加载外部插件时非空, 用于 RAII 管理资源, 使用时仍直接调用 instance / interface
    std::unique_ptr<QPluginLoader> loader;

    /// 实例指针缓存
    QObject* instance       = nullptr;

    /// 接口指针缓存
    IBasicPlugin* interface = nullptr;

    /// 插件文件路径
    QString file_path;

    /// 元数据缓存
    QString iid;
    QJsonObject metadata;
    PluginDescriptor descriptor;
};
