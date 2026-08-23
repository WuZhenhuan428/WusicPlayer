#include "plugin_manager.h"

#include "core/utils/path.hpp"
#include "plugin/i_basic_plugin.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include "core/logger/logger_manager.h"

namespace
{
auto logger = LoggerManager::instance().file_logger("plugin_manager", {"gui", "console"});
}

PluginManager::PluginManager(QObject* parent) : QObject(parent) {}

bool PluginManager::load_plugin(const QString& path)
{
    // 检查路径
    if (!QFileInfo::exists(path)) {
        logger->warn("load_plugin: Unexist path '{}'", path);
        return false;
    }

    // 创建 handle
    auto handle      = std::make_unique<PluginHandle>();
    handle->loader   = std::make_unique<QPluginLoader>(path);

    // 加载插件 & 缓存对象
    handle->instance = handle->loader->instance();
    if (!handle->instance) {
        logger->warn("load_plugin: failed to load plugin: {}", path);
        return false;
    }

    // 缓存接口
    handle->interface = qobject_cast<IBasicPlugin*>(handle->instance);
    if (!handle->interface) {
        logger->warn("plugin '{}' does not implement IBasicPlugin", path);
        return false;
    }

    // 缓存元数据
    handle->iid        = handle->loader->metaData().value("IID").toString();
    handle->metadata   = handle->loader->metaData().value("MetaData").toObject();
    handle->descriptor = this->resolve_descriptor(handle->interface, handle->metadata);

    // 文件路径
    handle->file_path  = QDir(path).absolutePath();

    // 重复检查
    if (handles_.contains(handle->descriptor.id)) {
        logger->warn("plugin id '{}' already loaded, skip", handle->descriptor.id);
        return false;
    }

    handles_.emplace(handle->descriptor.id, std::move(handle));
    return true;
}

void PluginManager::scan_directory(const QString& dir)
{
    QDir plugin_dir(dir);
    if (!plugin_dir.exists()) {
        return;
    }

    QStringList suffixes =
#if defined(Q_OS_WIN)
        {"dll"}
#elif defined(Q_OS_MACOS)
        {"so", "dylib", "bundle"} /* 用于加载插件时三者均可用 */
#elif defined(Q_OS_UNIX)
        {"so"}
#endif
    ;

    QDirIterator dir_it(dir, QDir::Files, QDirIterator::Subdirectories);
    while (dir_it.hasNext()) {
        dir_it.next();
        const QString path = dir_it.fileInfo().absoluteFilePath();
        if (utils::path::contains_suffix(path, suffixes) ||
            utils::path::parse_versioned_unix_so(path).has_value()) {
            this->load_plugin(path);
        }
    }
}

void PluginManager::register_builtin(IBasicPlugin* plugin)
{
    if (!plugin) {
        return;
    }

    if (handles_.contains(plugin->id())) {
        logger->warn("plugin id '{}' already registered, skip", plugin->id());
        return;
    }

    auto handle        = std::make_unique<PluginHandle>();
    handle->instance   = plugin;
    handle->interface  = plugin;

    // 不存在 json 元数据, 直接获取信息, 不调用函数
    handle->descriptor = {
        .id          = plugin->id(),
        .name        = plugin->name(),
        .version     = plugin->version(),
        .description = plugin->description(),
        .author      = plugin->author(),
        .categories  = plugin->categories(),
    };

    handles_.emplace(plugin->id(), std::move(handle));
}

PluginDescriptor PluginManager::resolve_descriptor(IBasicPlugin* plugin, const QJsonObject& json)
{
    if (!plugin) {
        return {};
    }
    PluginDescriptor desc;
    // clang-format off
    desc.id          = json["id"].isString()          ? json["id"].toString()                         : plugin->id();
    desc.name        = json["name"].isString()        ? json["name"].toString()                       : plugin->name();
    desc.version     = json["version"].isString()     ? json["version"].toString()                    : plugin->version();
    desc.description = json["description"].isString() ? json["description"].toString()                : plugin->description();
    desc.author      = json["author"].isString()      ? json["author"].toString()                     : plugin->author();
    desc.categories  = json["categories"].isArray()   ? json["categories"].toVariant().toStringList() : plugin->categories();
    // clang-format on
    return desc;
}

QVector<IBasicPlugin*> PluginManager::all() const
{
    QVector<IBasicPlugin*> vec;
    for (const auto& [_, handle] : this->handles_) {
        vec.push_back(handle->interface);
    }
    return vec;
}
