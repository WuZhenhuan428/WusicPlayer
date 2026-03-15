#pragma once

#include "model/ShortcutsViewModel/shortcuts_types.hpp"
#include "model/ShortcutsViewModel/shortcuts_view_model.hpp"
#include <QObject>
#include <QVector>
#include <QHash>
#include <QShortcut>
#include <QPointer>
#include <QWidget>

#include <functional>

class ShortcutsController : public QObject
{
    Q_OBJECT
public:
    using ShortcutHandler = std::function<void()>;

    explicit ShortcutsController(QObject* parent = nullptr);
    ~ShortcutsController() override = default;
    
    void registerAction(const ShortcutDescriptor& desc, ShortcutHandler handler, QObject* parent = nullptr);
    void registerOperation(ShortcutActionId action_id,
                           const QString& display_name,
                           ShortcutScope scope,
                           const QKeySequence& default_key,
                           ShortcutHandler handler,
                           QObject* owner = nullptr,
                           bool editable = true);
    void unregisterAction(ShortcutActionId action_id);
    bool setShortcut(ShortcutActionId action_id, const QKeySequence& seq);
    void enableAction(ShortcutActionId action_id, bool enabled);
    void rebind(ShortcutActionId action_id);
    void rebindAll();
    void resetAllToDefault();
    void setScopeHost(ShortcutScope scope, QWidget* host);
    QVector<ShortcutBinding> bindings() const;
    void applyBindings(const QVector<ShortcutBinding>& bindings);
    
    ShortcutsViewModel* viewModel() const;

signals:
    void sgnShortcutUpdated(ShortcutActionId action_id, const QKeySequence& seq);

private slots:
    void onBindingChanged(ShortcutActionId action_id, const QKeySequence& new_key);

private:
    void rebuildViewModel();
    QWidget* resolveScopeHost(ShortcutScope scope, QObject* owner) const;

    QHash<ShortcutActionId, ShortcutDescriptor> m_descMap;
    QHash<ShortcutActionId, ShortcutHandler> m_handleMap;
    QHash<ShortcutActionId, ShortcutBinding> m_bindingMap;
    QHash<ShortcutActionId, ShortcutBinding> m_pendingBindingMap;
    QHash<ShortcutActionId, QPointer<QShortcut>> m_runtimeMap;
    QHash<ShortcutActionId, QPointer<QObject>> m_ownerMap;
    QHash<ShortcutScope, QPointer<QWidget>> m_scopeHostMap;

    ShortcutsViewModel* m_viewModel = nullptr;
};
