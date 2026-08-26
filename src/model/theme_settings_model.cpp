#include "model/theme_settings_model.h"

ThemeSettingsModel::ThemeSettingsModel(QObject* parent) : QAbstractTableModel(parent) {}

void ThemeSettingsModel::set_entries(const QVector<ThemeEntry>& entries)
{
    beginResetModel();
    m_entries = entries;
    endResetModel();
}

int ThemeSettingsModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

int ThemeSettingsModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QVariant ThemeSettingsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const auto& e = m_entries.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:
            return e.name;
        case ColSource:
            return e.source;
        case ColAuthor:
            return e.author;
        case ColDark:
            return e.isDark ? tr("Dark") : tr("Light");
        }
    }

    if (role == RoleIsDark)
        return e.isDark;
    if (role == Qt::TextAlignmentRole)
        return int(Qt::AlignLeft | Qt::AlignVCenter);

    return {};
}

QVariant ThemeSettingsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColName:
        return tr("Name");
    case ColSource:
        return tr("Source");
    case ColAuthor:
        return tr("Author");
    case ColDark:
        return tr("Mode");
    }
    return {};
}
