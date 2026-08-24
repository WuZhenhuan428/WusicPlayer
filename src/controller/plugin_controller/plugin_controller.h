#pragma once

#include "core/plugin_manager/plugin_manager.h"
#include "core/plugin_manager/plugin_types.h"

#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

class PluginController : public QObject
{
    Q_OBJECT

public:
    explicit PluginController(QObject* parent = nullptr);

    void import();
    void remove(const QString& id);
    const QVector<PluginDescriptor> descriptors();

signals:
    void sgn_data_updated();

private:
    std::unique_ptr<PluginManager> plugin_manager_ = nullptr;
};
