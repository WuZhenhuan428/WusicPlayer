#include "shortcuts_controller.hpp"

ShortcutsController::ShortcutsController(QObject* parent)
    : QObject(parent),
      m_viewModel(new ShortcutsViewModel(this))
{
    connect(m_viewModel, &ShortcutsViewModel::sgnBindingChanged, 
            this, &ShortcutsController::onBindingChanged);
}

void ShortcutsController::registerDescriptor(ShortcutDescriptor& desc) {
    if (desc.action_id.isEmpty()) return;
    m_descMap.insert(desc.action_id, desc);
}

void ShortcutsController::registerBinding(const ShortcutBinding& binding) {
    if (binding.action_id.isEmpty()) return;
    m_bindingMap.insert(binding.action_id, binding);
}

void ShortcutsController::mergeItems() {
    QVector<ShortcutItem> items;
    items.reserve(m_descMap.size());

    for (auto it = m_descMap.constBegin(); it != m_descMap.constEnd(); ++it) {
        ShortcutItem item;
        item.desc = it.value();

        if (m_bindingMap.contains(it.key())) {
            item.binding = m_bindingMap.value(it.key());
        } else {
            item.binding.action_id = item.desc.action_id;
            item.binding.current_key = item.desc.default_key;
            item.binding.enabled = true;
        }

        items.push_back(item);
    }
    m_viewModel->setItems(items);
}

ShortcutsViewModel* ShortcutsController::viewModel() const {
    return m_viewModel;
}

bool ShortcutsController::setShortcut(const QString& action_id, const QKeySequence& seq) {
    return m_viewModel->updateBinding(action_id, seq);
}

void ShortcutsController::onBindingChanged(const QString& action_id, const QKeySequence& new_key) {
    if (!m_bindingMap.contains(action_id)) {
        ShortcutBinding b;
        b.action_id = action_id;
        b.current_key = new_key;
        b.enabled = true;
        m_bindingMap.insert(action_id, b);
    } else {
        m_bindingMap[action_id].current_key = new_key;
    }
    emit sgnShortcutUpdated(action_id, new_key);
}
