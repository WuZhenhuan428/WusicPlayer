#pragma once

#include "shortcuts_types.hpp"

#include <QObject>
#include <QAbstractTableModel>
#include <QModelIndex>
#include <QString>
#include <QVector>
#include <QHash>

class ShortcutsViewModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        ColActionName = 0,
        ColKeySequence,
        ColScope,
        ColConflict,
        ColCount
    };

    enum UserRole {
        RoleActionId = Qt::UserRole + 1,
        RoleConflict,
        RoleConflictWith,
        RoleEnabled
    };

    explicit ShortcutsViewModel(QObject* parent = nullptr);
    ~ShortcutsViewModel();

    void setItems(const QVector<ShortcutItem>& items);
    const QVector<ShortcutItem>& items();

    bool updateBinding(ShortcutActionId action_id, const QKeySequence& new_key);
    void recomputeConflicts();

signals:
    void sgnBindingChanged(ShortcutActionId action_id, const QKeySequence& new_key);
    void sgnConflictUpdated();

protected:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

private:
    int findRowbyActionId(ShortcutActionId action_id) const;
    // static QStrign scopeToString(ShortcutScope s);  // replace with libmagicenum
    QVector<ShortcutItem> m_items;
};