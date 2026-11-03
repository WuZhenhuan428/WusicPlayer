#include "model/shortcuts_view_model/shortcuts_view_model.hpp"

#include <magic_enum/magic_enum.hpp>
#include <string>

ShortcutsViewModel::ShortcutsViewModel(QObject* parent) : QAbstractTableModel(parent) {}

void ShortcutsViewModel::set_items(const QVector<ShortcutItem>& items)
{
    beginResetModel();
    items_ = items;
    endResetModel();
    recompute_conflicts();
}

const QVector<ShortcutItem>& ShortcutsViewModel::items()
{
    return items_;
}

bool ShortcutsViewModel::update_binding(ShortcutActionId action_id, const QKeySequence& new_key)
{
    const int row = find_row_by_action_id(action_id);
    if (row < 0) {
        return false;
    }

    items_[row].binding.current_key = new_key;
    emit dataChanged(index(row, ColKeySequence), index(row, ColConflict),
                     {Qt::DisplayRole, RoleConflict, RoleConflictWith});
    recompute_conflicts();
    emit sgn_binding_changed(action_id, new_key);
    return true;
}

void ShortcutsViewModel::recompute_conflicts()
{
    QHash<QString, int> key_first_row; // key: `scope|seq`
    for (auto& item : items_) {
        item.conflict = false;
        item.conflict_with_action_id.clear();
    }

    for (int i = 0; i < items_.size(); ++i) {
        const auto& it = items_[i];
        if (!it.binding.enabled || it.binding.current_key.isEmpty()) {
            continue;
        }
        const QString key = QString::number(magic_enum::enum_integer(it.desc.scope)) + "|" +
                            it.binding.current_key.toString(QKeySequence::PortableText);
        if (!key_first_row.contains(key)) {
            key_first_row.insert(key, i);
        } else {
            const int j        = key_first_row.value(key);
            items_[i].conflict = true;
            items_[i].conflict_with_action_id =
                shortcut_action_id_to_string(items_[j].desc.action_id);
        }
    }

    if (!items_.isEmpty()) {
        emit dataChanged(index(0, ColConflict),
                         index(static_cast<int>(items_.size() - 1), ColConflict),
                         {Qt::DisplayRole, RoleConflict, RoleConflictWith});
    }
    emit sgn_conflict_updated();
}

QModelIndex ShortcutsViewModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }
    if (row < 0 || row >= items_.size()) {
        return {};
    }
    return createIndex(row, column); // <-
}

int ShortcutsViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(items_.size());
}

int ShortcutsViewModel::columnCount(const QModelIndex& parent) const
{
    // 固定column: desc(action_id作为内部属性不显示), shortcut, scope, conflict
    if (parent.isValid()) {
        return 0;
    }
    return ColCount;
}

QVariant ShortcutsViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= items_.size()) {
        return {};
    }
    const auto& item = items_[index.row()];

    if (role == Qt::TextAlignmentRole) {
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColActionName:
            return item.desc.display_name;
        case ColKeySequence:
            return item.binding.current_key.toString();
        case ColScope:
            return QString::fromStdString(std::string(magic_enum::enum_name(item.desc.scope)));
        case ColConflict:
            return item.conflict ? "Conflict" : "";
        default:
            return {};
        }
    }

    if (role == RoleActionId) {
        return shortcut_action_id_to_string(item.desc.action_id);
    }
    if (role == RoleConflict) {
        return item.conflict;
    }
    if (role == RoleConflictWith) {
        return item.conflict_with_action_id;
    }
    if (role == RoleEnabled) {
        return item.binding.enabled;
    }

    return {};
}

QVariant ShortcutsViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
    case ColActionName:
        return tr("Action");
    case ColKeySequence:
        return tr("Shortcuts");
    case ColScope:
        return tr("Scope");
    case ColConflict:
        return tr("Status");
    default:
        return {};
    }
}

Qt::ItemFlags ShortcutsViewModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    auto f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == ColKeySequence) {
        if (index.row() >= 0 && index.row() < items_.size() && items_[index.row()].desc.editable) {
            f |= Qt::ItemIsEditable;
        }
    }
    return f;
}

bool ShortcutsViewModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    if (index.column() != ColKeySequence) {
        return false;
    }
    if (role != Qt::EditRole) {
        return false;
    }
    if (index.row() < 0 || index.row() >= items_.size()) {
        return false;
    }
    if (!items_[index.row()].desc.editable) {
        return false;
    }

    const QString action_id_text = data(this->index(index.row(), 0), RoleActionId).toString();
    ShortcutActionId action_id   = ShortcutActionId::PlayPause;
    if (!shortcut_action_id_from_string(action_id_text, action_id)) {
        return false;
    }
    const QKeySequence seq(value.toString());
    return update_binding(action_id, seq);
}

int ShortcutsViewModel::find_row_by_action_id(ShortcutActionId action_id) const
{
    for (int i = 0; i < items_.size(); ++i) {
        if (items_[i].desc.action_id == action_id) {
            return i;
        }
    }
    return -1;
}
