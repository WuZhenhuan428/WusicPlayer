#pragma once

#include "model/ShortcutsViewModel/shortcuts_types.hpp"
#include "model/ShortcutsViewModel/shortcuts_view_model.hpp"
#include <QObject>
#include <QVector>
#include <QHash>

class ShortcutsController : public QObject
{
    Q_OBJECT
public:
    explicit ShortcutsController(QObject* parent = nullptr);
    ~ShortcutsController() override = default;

    void registerDescriptor(ShortcutDescriptor& desc);
    void registerBinding(const ShortcutBinding& binding);
    void mergeItems();

    ShortcutsViewModel* viewModel() const;

    bool setShortcut(const QString& action_id, const QKeySequence& seq);

signals:
    void sgnShortcutUpdated(const QString& action_id, const QKeySequence& seq);

private slots:
    void onBindingChanged(const QString& action_id, const QKeySequence& new_key);

private:
    QHash<QString, ShortcutDescriptor> m_descMap;
    QHash<QString, ShortcutBinding> m_bindingMap;

    ShortcutsViewModel* m_viewModel = nullptr;
};
