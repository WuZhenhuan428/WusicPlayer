#pragma once

#include "model/shortcuts_view_model/shortcuts_types.hpp"

#include <QAbstractTableModel>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVector>

class ShortcutsViewModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column
    {
        ColActionName = 0,
        ColKeySequence,
        ColScope,
        ColConflict,
        ColCount
    };

    enum UserRole
    {
        RoleActionId = Qt::UserRole + 1,
        RoleConflict,
        RoleConflictWith,
        RoleEnabled
    };

    explicit ShortcutsViewModel(QObject* parent = nullptr);
    ~ShortcutsViewModel();

    void set_items(const QVector<ShortcutItem>& items);
    const QVector<ShortcutItem>& items();

    bool update_binding(ShortcutActionId action_id, const QKeySequence& new_key);
    void recompute_conflicts();

signals:
    void sgn_binding_changed(ShortcutActionId action_id, const QKeySequence& new_key);
    void sgn_conflict_updated();

protected:
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

private:
    int find_row_by_action_id(ShortcutActionId action_id) const;
    // static QStrign scopeToString(ShortcutScope s);  // replace with libmagicenum
    QVector<ShortcutItem> m_items;
};
