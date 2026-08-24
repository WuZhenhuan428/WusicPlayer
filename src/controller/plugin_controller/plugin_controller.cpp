#include "plugin_controller.h"

#include "core/logger/logger_manager.h"

#include <QFileDialog>
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

const QVector<PluginDescriptor> PluginController::descriptors()
{
    return plugin_manager_->descriptors();
}
