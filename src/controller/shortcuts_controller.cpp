#include "shortcuts_controller.h"

#include <QApplication>

ShortcutsController::ShortcutsController(QObject* parent)
    : QObject(parent),
      m_viewModel(new ShortcutsViewModel(this))
{
    connect(m_viewModel, &ShortcutsViewModel::sgnBindingChanged,
            this, &ShortcutsController::onBindingChanged);
}

void ShortcutsController::registerAction(const ShortcutDescriptor& desc, ShortcutHandler handler, QObject* parent) {
    m_descMap.insert(desc.action_id, desc);
    m_handleMap.insert(desc.action_id, std::move(handler));
    m_ownerMap.insert(desc.action_id, parent);

    if (!m_bindingMap.contains(desc.action_id)) {
        ShortcutBinding binding;
        binding.action_id = desc.action_id;
        binding.current_key = desc.default_key;
        binding.enabled = true;
        m_bindingMap.insert(desc.action_id, binding);
    }

    if (m_pendingBindingMap.contains(desc.action_id)) {
        const ShortcutBinding pending = m_pendingBindingMap.take(desc.action_id);
        auto it = m_bindingMap.find(desc.action_id);
        if (it != m_bindingMap.end()) {
            it->current_key = pending.current_key;
            it->enabled = pending.enabled;
        }
    }

    rebind(desc.action_id);
    rebuildViewModel();
}

void ShortcutsController::registerOperation(ShortcutActionId action_id,
                                            const QString& display_name,
                                            ShortcutScope scope,
                                            const QKeySequence& default_key,
                                            ShortcutHandler handler,
                                            QObject* owner,
                                            bool editable)
{
    ShortcutDescriptor desc;
    desc.action_id = action_id;
    desc.display_name = display_name;
    desc.scope = scope;
    desc.default_key = default_key;
    desc.editable = editable;
    registerAction(desc, std::move(handler), owner);
}

void ShortcutsController::unregisterAction(ShortcutActionId action_id) {
    if (m_runtimeMap.contains(action_id) && m_runtimeMap[action_id]) {
        m_runtimeMap[action_id]->deleteLater();
    }
    m_runtimeMap.remove(action_id);
    m_handleMap.remove(action_id);
    m_bindingMap.remove(action_id);
    m_descMap.remove(action_id);
    m_ownerMap.remove(action_id);
    rebuildViewModel();
}

bool ShortcutsController::setShortcut(ShortcutActionId action_id, const QKeySequence& seq) {
    if (!m_descMap.contains(action_id)) {
        ShortcutBinding pending = m_pendingBindingMap.value(action_id, ShortcutBinding{action_id, QKeySequence(), true});
        pending.current_key = seq;
        m_pendingBindingMap.insert(action_id, pending);
        return false;
    }

    auto binding_it = m_bindingMap.find(action_id);
    if (binding_it == m_bindingMap.end()) {
        ShortcutBinding binding;
        binding.action_id = action_id;
        binding.current_key = seq;
        binding.enabled = true;
        m_bindingMap.insert(action_id, binding);
    } else {
        binding_it->current_key = seq;
    }

    rebind(action_id);
    rebuildViewModel();
    emit sgnShortcutUpdated(action_id, seq);
    return true;
}

void ShortcutsController::enableAction(ShortcutActionId action_id, bool enabled) {
    if (!m_bindingMap.contains(action_id)) {
        ShortcutBinding pending = m_pendingBindingMap.value(action_id, ShortcutBinding{action_id, QKeySequence(), true});
        pending.enabled = enabled;
        m_pendingBindingMap.insert(action_id, pending);
        return;
    }

    m_bindingMap[action_id].enabled = enabled;
    rebind(action_id);
    rebuildViewModel();
}

void ShortcutsController::rebind(ShortcutActionId action_id)
{
    if (!m_descMap.contains(action_id)) {
        return;
    }

    if (m_runtimeMap.contains(action_id) && m_runtimeMap[action_id]) {
        m_runtimeMap[action_id]->deleteLater();
        m_runtimeMap[action_id] = nullptr;
    }

    const ShortcutBinding binding = m_bindingMap.value(action_id);
    if (!binding.enabled || binding.current_key.isEmpty()) {
        return;
    }

    const ShortcutDescriptor desc = m_descMap.value(action_id);
    QWidget* host = resolveScopeHost(desc.scope, m_ownerMap.value(action_id));
    if (!host) {
        return;
    }

    auto* shortcut = new QShortcut(binding.current_key, host);
    const Qt::ShortcutContext context =
        (desc.scope == ShortcutScope::Application) ? Qt::ApplicationShortcut : Qt::WidgetWithChildrenShortcut;
    shortcut->setContext(context);

    const auto handler = m_handleMap.value(action_id);
    connect(shortcut, &QShortcut::activated, this, [handler]() {
        if (handler) {
            handler();
        }
    });
    m_runtimeMap.insert(action_id, shortcut);
}

void ShortcutsController::rebindAll()
{
    const auto action_ids = m_descMap.keys();
    for (ShortcutActionId action_id : action_ids) {
        rebind(action_id);
    }
    rebuildViewModel();
}

void ShortcutsController::resetAllToDefault()
{
    for (auto it = m_descMap.constBegin(); it != m_descMap.constEnd(); ++it) {
        setShortcut(it.key(), it.value().default_key);
        enableAction(it.key(), true);
    }
    rebuildViewModel();
}

void ShortcutsController::setScopeHost(ShortcutScope scope, QWidget* host)
{
    m_scopeHostMap.insert(scope, host);
}

QVector<ShortcutBinding> ShortcutsController::bindings() const
{
    QVector<ShortcutBinding> result;
    result.reserve(m_bindingMap.size());
    for (auto it = m_bindingMap.constBegin(); it != m_bindingMap.constEnd(); ++it) {
        result.push_back(it.value());
    }
    return result;
}

void ShortcutsController::applyBindings(const QVector<ShortcutBinding>& bindings)
{
    for (const ShortcutBinding& binding : bindings) {
        setShortcut(binding.action_id, binding.current_key);
        enableAction(binding.action_id, binding.enabled);
    }
    rebuildViewModel();
}

ShortcutsViewModel* ShortcutsController::viewModel() const
{
    return m_viewModel;
}

void ShortcutsController::onBindingChanged(ShortcutActionId action_id, const QKeySequence& new_key)
{
    setShortcut(action_id, new_key);
}

void ShortcutsController::rebuildViewModel()
{
    QVector<ShortcutItem> items;
    items.reserve(m_descMap.size());
    for (auto it = m_descMap.constBegin(); it != m_descMap.constEnd(); ++it) {
        ShortcutItem item;
        item.desc = it.value();
        item.binding = m_bindingMap.value(it.key(), ShortcutBinding{it.key(), QKeySequence(), true});
        items.push_back(item);
    }
    m_viewModel->setItems(items);
}

QWidget* ShortcutsController::resolveScopeHost(ShortcutScope scope, QObject* owner) const
{
    if (m_scopeHostMap.contains(scope) && m_scopeHostMap.value(scope)) {
        return m_scopeHostMap.value(scope);
    }

    if (auto* widgetOwner = qobject_cast<QWidget*>(owner)) {
        return widgetOwner;
    }

    return QApplication::activeWindow();
}
