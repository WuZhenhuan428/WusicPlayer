#include "shortcuts_view_model.hpp"

#include <string>
#include <magic_enum/magic_enum.hpp>

ShortcutsViewModel::ShortcutsViewModel(QObject* parent)
    : QAbstractTableModel(parent)
{}

ShortcutsViewModel::~ShortcutsViewModel() {}

void ShortcutsViewModel::setItems(const QVector<ShortcutItem>& items) {
    beginResetModel();
    m_items = items;
    endResetModel();
    recomputeConflicts();
}

const QVector<ShortcutItem>& ShortcutsViewModel::items() {
    return m_items;
}

bool ShortcutsViewModel::updateBinding(const QString& action_id, const QKeySequence& new_key) {
    const int row = findRowbyActionId(action_id);
    if (row < 0) return false;

    m_items[row].binding.current_key = new_key;
    emit dataChanged(index(row, ColKeySequence), index(row, ColConflict),
                     {Qt::DisplayRole, RoleConflict, RoleConflictWith});
    recomputeConflicts();
    emit sgnBindingChanged(action_id, new_key);
    return true;
}

void ShortcutsViewModel::recomputeConflicts() {
    QHash<QString, int> key_first_row;  // key: `scope|seq`
    for (auto& item : m_items) {
        item.conflict = false;
        item.conflict_with_action_id.clear();
    }

    for (int i = 0; i < m_items.size(); ++i) {
        const auto& it = m_items[i];
        if (!it.binding.enabled || it.binding.current_key.isEmpty()) {
            continue;
        }
        const QString key = QString::number(magic_enum::enum_integer(it.desc.scope)) + "|" +
                            it.binding.current_key.toString(QKeySequence::PortableText);
        if (!key_first_row.contains(key)) {
            key_first_row.insert(key, i);
        } else {
            const int j = key_first_row.value(key);
            m_items[i].conflict = true;
            m_items[i].conflict_with_action_id = m_items[j].desc.action_id;
        }
    }

    if (!m_items.isEmpty()) {
        emit dataChanged(index(0, ColConflict), index(m_items.size()-1, ColConflict),
                         {Qt::DisplayRole, RoleConflict, RoleConflictWith});
    }
    emit sgnConflictUpdated();
}


QModelIndex ShortcutsViewModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();
    if (row < 0 && row >= m_items.size()) return QModelIndex();
    return createIndex(row, column);    // <-
}

int ShortcutsViewModel::rowCount(const QModelIndex &parent) const {
    if (!parent.isValid()) return 0;
    return m_items.size();
}

int ShortcutsViewModel::columnCount(const QModelIndex &parent) const {
    // 固定column: desc(action_id作为内部属性不显示), shortcut, scope, conflict
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant ShortcutsViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();
    const auto& item = m_items[index.row()];

    if (role == Qt::TextAlignmentRole) {
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColActionName: return item.desc.display_name;
        case ColKeySequence: return item.binding.current_key.toString();
        case ColScope: return QString::fromStdString(std::string(magic_enum::enum_name(item.desc.scope)));
        case ColConflict: return item.conflict ? "Conflict" : "";
        default: return {};
        }
    }

    if (role == RoleActionId) return item.desc.action_id;
    if (role == RoleConflict) return item.conflict;
    if (role == RoleConflictWith) return item.conflict_with_action_id;
    if (role == RoleEnabled) return item.binding.enabled;

    return {};
}

QVariant ShortcutsViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case ColActionName: return "Action";
    case ColKeySequence: return "Shortcutr";
    case ColScope: return "Scope";
    case ColConflict: return "Status";
    default: return {};
    }
}

Qt::ItemFlags ShortcutsViewModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    auto f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == ColKeySequence) {
        f |= Qt::ItemIsSelectable;
    }
    return f;
}

bool ShortcutsViewModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid()) return false;
    if (index.column() != ColKeySequence) return false;
    if (role != Qt::EditRole) return false;

    const QString action_id = data(this->index(index.row(), 0), RoleActionId).toString();
    const QKeySequence seq(value.toString());
    return updateBinding(action_id, seq);
}

int ShortcutsViewModel::findRowbyActionId(const QString& action_id) const {
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].desc.action_id == action_id) {
            return i;
        }
    }
    return -1;
}