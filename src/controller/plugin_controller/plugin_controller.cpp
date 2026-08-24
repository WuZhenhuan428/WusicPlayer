#include "plugin_controller.h"

#include "core/logger/logger_manager.h"

#include <QFileDialog>
#include <QJsonArray>
#include <QStringList>

namespace
{
auto* logger = LoggerManager::instance().file_logger("plugin_controller", {"gui", "console"});
}

PluginController::PluginController(QObject* parent) :
    QObject(parent), plugin_manager_(std::make_unique<PluginManager>())
{}

void PluginController::import()
{
    const QStringList files =
        QFileDialog::getOpenFileNames(nullptr, tr("Import plugin(s)"), QString(),
                                      tr("Plugins (*.so *.dll *.dylib *.bundle);;All files (*)"));
    if (files.isEmpty()) {
        return;
    }

    int loaded = 0;
    for (const QString& file : files) {
        if (plugin_manager_->load_plugin(file)) {
            ++loaded;
        }
    }
    logger->info("plugin import: {} / {} loaded", loaded, files.size());
    if (loaded > 0) {
        emit sgn_data_updated();
    }
}

void PluginController::remove(const QString& id)
{
    if (plugin_manager_->remove(id)) {
        emit sgn_data_updated();
    }
}

void PluginController::scan_directory(const QString& dir)
{
    plugin_manager_->scan_directory(dir);
}

void PluginController::load_from_json(const QJsonObject& json)
{
    const QJsonArray arr = json.value("external_plugins").toArray();
    if (arr.isEmpty()) {
        return;
    }
    int restored = 0;
    for (const auto& v : arr) {
        const QString path = v.toString();
        if (!path.isEmpty() && plugin_manager_->load_plugin(path)) {
            ++restored;
        }
    }
    logger->info("plugin restore: {} plugin(s) from config", restored);
}

QJsonObject PluginController::save_to_json()
{
    QJsonObject obj;
    QJsonArray arr;
    const auto paths = plugin_manager_->external_plugin_paths();
    for (const QString& p : paths) {
        arr.append(p);
    }
    obj["external_plugins"] = arr;
    return obj;
}

QString PluginController::config_sub_key() const
{
    return QStringLiteral("plugins");
}

const QVector<PluginDescriptor> PluginController::descriptors()
{
    return plugin_manager_->descriptors();
}
