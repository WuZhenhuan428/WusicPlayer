#pragma once

#include "core/config_manager/i_configurable.h"
#include "model/shortcuts_view_model/shortcuts_types.hpp"
#include "model/shortcuts_view_model/shortcuts_view_model.hpp"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QShortcut>
#include <QVector>
#include <QWidget>
#include <functional>

class ShortcutsController : public QObject, public IConfigurable
{
    Q_OBJECT
public:
    using ShortcutHandler = std::function<void()>;

    explicit ShortcutsController(QObject* parent = nullptr);
    ~ShortcutsController() override = default;

    void register_action(const ShortcutDescriptor& desc, ShortcutHandler handler,
                         QObject* parent = nullptr);
    void register_operation(ShortcutActionId action_id, const QString& display_name,
                            ShortcutScope scope, const QKeySequence& default_key,
                            ShortcutHandler handler, QObject* owner = nullptr,
                            bool editable = true);
    void unregister_action(ShortcutActionId action_id);
    bool set_shortcut(ShortcutActionId action_id, const QKeySequence& seq);
    void enable_action(ShortcutActionId action_id, bool enabled);
    void rebind(ShortcutActionId action_id);
    void rebind_all();
    void reset_all_to_default();
    void set_scope_host(ShortcutScope scope, QWidget* host);
    QVector<ShortcutBinding> bindings() const;
    void apply_bindings(const QVector<ShortcutBinding>& bindings);

    ShortcutsViewModel* view_model() const;

    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

signals:
    void sgn_shortcut_updated(ShortcutActionId action_id, const QKeySequence& seq);

private slots:
    void on_binding_changed(ShortcutActionId action_id, const QKeySequence& new_key);

private:
    void rebuild_view_model();
    QWidget* resolve_scope_host(ShortcutScope scope, QObject* owner) const;

    QHash<ShortcutActionId, ShortcutDescriptor> m_descMap;
    QHash<ShortcutActionId, ShortcutHandler> m_handleMap;
    QHash<ShortcutActionId, ShortcutBinding> m_bindingMap;
    QHash<ShortcutActionId, ShortcutBinding> m_pendingBindingMap;
    QHash<ShortcutActionId, QPointer<QShortcut>> m_runtimeMap;
    QHash<ShortcutActionId, QPointer<QObject>> m_ownerMap;
    QHash<ShortcutScope, QPointer<QWidget>> m_scopeHostMap;

    ShortcutsViewModel* m_viewModel = nullptr;
};
