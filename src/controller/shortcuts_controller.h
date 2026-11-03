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
    [[nodiscard]] QVector<ShortcutBinding> bindings() const;
    void apply_bindings(const QVector<ShortcutBinding>& bindings);

    [[nodiscard]] ShortcutsViewModel* view_model() const;

    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    [[nodiscard]] QString config_sub_key() const override;

signals:
    void sgn_shortcut_updated(ShortcutActionId action_id, const QKeySequence& seq);

private slots:
    void on_binding_changed(ShortcutActionId action_id, const QKeySequence& new_key);

private:
    void rebuild_view_model();
    QWidget* resolve_scope_host(ShortcutScope scope, QObject* owner) const;

    QHash<ShortcutActionId, ShortcutDescriptor> desc_map_;
    QHash<ShortcutActionId, ShortcutHandler> handle_map_;
    QHash<ShortcutActionId, ShortcutBinding> binding_map_;
    QHash<ShortcutActionId, ShortcutBinding> pending_binding_map_;
    QHash<ShortcutActionId, QPointer<QShortcut>> runtime_map_;
    QHash<ShortcutActionId, QPointer<QObject>> owner_map_;
    QHash<ShortcutScope, QPointer<QWidget>> scope_host_map_;

    ShortcutsViewModel* view_model_ = nullptr;
};
