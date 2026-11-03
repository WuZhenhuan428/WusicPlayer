#include "controller/shortcuts_controller.h"

#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>

ShortcutsController::ShortcutsController(QObject* parent) :
    QObject(parent), view_model_(new ShortcutsViewModel(this))
{
    connect(view_model_, &ShortcutsViewModel::sgn_binding_changed, this,
            &ShortcutsController::on_binding_changed);
}

void ShortcutsController::register_action(const ShortcutDescriptor& desc, ShortcutHandler handler,
                                          QObject* parent)
{
    desc_map_.insert(desc.action_id, desc);
    handle_map_.insert(desc.action_id, std::move(handler));
    owner_map_.insert(desc.action_id, parent);

    if (!binding_map_.contains(desc.action_id)) {
        ShortcutBinding binding;
        binding.action_id   = desc.action_id;
        binding.current_key = desc.default_key;
        binding.enabled     = true;
        binding_map_.insert(desc.action_id, binding);
    }

    if (pending_binding_map_.contains(desc.action_id)) {
        const ShortcutBinding pending = pending_binding_map_.take(desc.action_id);
        auto it                       = binding_map_.find(desc.action_id);
        if (it != binding_map_.end()) {
            it->current_key = pending.current_key;
            it->enabled     = pending.enabled;
        }
    }

    rebind(desc.action_id);
    rebuild_view_model();
}

void ShortcutsController::register_operation(ShortcutActionId action_id,
                                             const QString& display_name, ShortcutScope scope,
                                             const QKeySequence& default_key,
                                             ShortcutHandler handler, QObject* owner, bool editable)
{
    ShortcutDescriptor desc;
    desc.action_id    = action_id;
    desc.display_name = display_name;
    desc.scope        = scope;
    desc.default_key  = default_key;
    desc.editable     = editable;
    register_action(desc, std::move(handler), owner);
}

void ShortcutsController::unregister_action(ShortcutActionId action_id)
{
    if (runtime_map_.contains(action_id) && runtime_map_[action_id]) {
        runtime_map_[action_id]->deleteLater();
    }
    runtime_map_.remove(action_id);
    handle_map_.remove(action_id);
    binding_map_.remove(action_id);
    desc_map_.remove(action_id);
    owner_map_.remove(action_id);
    rebuild_view_model();
}

bool ShortcutsController::set_shortcut(ShortcutActionId action_id, const QKeySequence& seq)
{
    if (!desc_map_.contains(action_id)) {
        ShortcutBinding pending =
            pending_binding_map_.value(action_id, ShortcutBinding{action_id, QKeySequence(), true});
        pending.current_key = seq;
        pending_binding_map_.insert(action_id, pending);
        return false;
    }

    auto binding_it = binding_map_.find(action_id);
    if (binding_it == binding_map_.end()) {
        ShortcutBinding binding;
        binding.action_id   = action_id;
        binding.current_key = seq;
        binding.enabled     = true;
        binding_map_.insert(action_id, binding);
    } else {
        binding_it->current_key = seq;
    }

    rebind(action_id);
    rebuild_view_model();
    emit sgn_shortcut_updated(action_id, seq);
    return true;
}

void ShortcutsController::enable_action(ShortcutActionId action_id, bool enabled)
{
    if (!binding_map_.contains(action_id)) {
        ShortcutBinding pending =
            pending_binding_map_.value(action_id, ShortcutBinding{action_id, QKeySequence(), true});
        pending.enabled = enabled;
        pending_binding_map_.insert(action_id, pending);
        return;
    }

    binding_map_[action_id].enabled = enabled;
    rebind(action_id);
    rebuild_view_model();
}

void ShortcutsController::rebind(ShortcutActionId action_id)
{
    if (!desc_map_.contains(action_id)) {
        return;
    }

    if (runtime_map_.contains(action_id) && runtime_map_[action_id]) {
        runtime_map_[action_id]->deleteLater();
        runtime_map_[action_id] = nullptr;
    }

    const ShortcutBinding binding = binding_map_.value(action_id);
    if (!binding.enabled || binding.current_key.isEmpty()) {
        return;
    }

    const ShortcutDescriptor desc = desc_map_.value(action_id);
    QWidget* host                 = resolve_scope_host(desc.scope, owner_map_.value(action_id));
    if (host == nullptr) {
        return;
    }

    auto* shortcut                    = new QShortcut(binding.current_key, host);
    const Qt::ShortcutContext context = (desc.scope == ShortcutScope::Application)
                                            ? Qt::ApplicationShortcut
                                            : Qt::WidgetWithChildrenShortcut;
    shortcut->setContext(context);

    const auto handler = handle_map_.value(action_id);
    connect(shortcut, &QShortcut::activated, this, [handler]() {
        if (handler) {
            handler();
        }
    });
    runtime_map_.insert(action_id, shortcut);
}

void ShortcutsController::rebind_all()
{
    const auto action_ids = desc_map_.keys();
    for (ShortcutActionId action_id : action_ids) {
        rebind(action_id);
    }
    rebuild_view_model();
}

void ShortcutsController::reset_all_to_default()
{
    for (auto it = desc_map_.constBegin(); it != desc_map_.constEnd(); ++it) {
        set_shortcut(it.key(), it.value().default_key);
        enable_action(it.key(), true);
    }
    rebuild_view_model();
}

void ShortcutsController::set_scope_host(ShortcutScope scope, QWidget* host)
{
    scope_host_map_.insert(scope, host);
}

QVector<ShortcutBinding> ShortcutsController::bindings() const
{
    QVector<ShortcutBinding> result;
    result.reserve(binding_map_.size());
    for (auto it = binding_map_.constBegin(); it != binding_map_.constEnd(); ++it) {
        result.push_back(it.value());
    }
    return result;
}

void ShortcutsController::apply_bindings(const QVector<ShortcutBinding>& bindings)
{
    for (const ShortcutBinding& binding : bindings) {
        set_shortcut(binding.action_id, binding.current_key);
        enable_action(binding.action_id, binding.enabled);
    }
    rebuild_view_model();
}

ShortcutsViewModel* ShortcutsController::view_model() const
{
    return view_model_;
}

void ShortcutsController::on_binding_changed(ShortcutActionId action_id,
                                             const QKeySequence& new_key)
{
    set_shortcut(action_id, new_key);
}

void ShortcutsController::rebuild_view_model()
{
    QVector<ShortcutItem> items;
    items.reserve(desc_map_.size());
    for (auto it = desc_map_.constBegin(); it != desc_map_.constEnd(); ++it) {
        ShortcutItem item;
        item.desc = it.value();
        item.binding =
            binding_map_.value(it.key(), ShortcutBinding{it.key(), QKeySequence(), true});
        items.push_back(item);
    }
    view_model_->set_items(items);
}

QWidget* ShortcutsController::resolve_scope_host(ShortcutScope scope, QObject* owner) const
{
    if (scope_host_map_.contains(scope) && scope_host_map_.value(scope)) {
        return scope_host_map_.value(scope);
    }

    if (auto* widget_owner = qobject_cast<QWidget*>(owner)) {
        return widget_owner;
    }

    return QApplication::activeWindow();
}

void ShortcutsController::load_from_json(const QJsonObject& json)
{
    QJsonObject obj      = json.value(this->config_sub_key()).toObject();
    const QJsonArray arr = obj.value("bindings").toArray();
    QVector<ShortcutBinding> parsed;
    parsed.reserve(arr.size());

    for (const auto& value : arr) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject item     = value.toObject();
        ShortcutActionId action_id = ShortcutActionId::PlayPause;
        if (!shortcut_action_id_from_string(item.value("action_id").toString(), action_id)) {
            continue;
        }

        ShortcutBinding binding;
        binding.action_id = action_id;
        binding.current_key =
            QKeySequence::fromString(item.value("key").toString(), QKeySequence::PortableText);
        binding.enabled = item.value("enabled").toBool(true);
        parsed.push_back(binding);
    }

    apply_bindings(parsed);
}

QJsonObject ShortcutsController::save_to_json()
{
    QJsonArray arr;
    for (const ShortcutBinding& binding : bindings()) {
        QJsonObject item;
        item["action_id"] = shortcut_action_id_to_string(binding.action_id);
        item["key"]       = binding.current_key.toString(QKeySequence::PortableText);
        item["enabled"]   = binding.enabled;
        arr.append(item);
    }

    QJsonObject obj;
    obj["bindings"] = arr;
    return obj;
}

QString ShortcutsController::config_sub_key() const
{
    return "shortcuts";
}
