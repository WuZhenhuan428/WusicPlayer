#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <QVector>

/// 主题列表中的一行
struct ThemeEntry
{
    QString name;
    QString source; // "System" / "Builtin" / "External"
    QString author;
    bool isDark = false;
};

/// 主题浏览模型——展示所有可用主题（含系统/内置/外部）。
/// 数据由 ThemeService::scanThemes() 填充。
class ThemeSettingsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column
    {
        ColName = 0,
        ColSource,
        ColAuthor,
        ColDark,
        ColCount
    };
    enum UserRole
    {
        RoleIsDark = Qt::UserRole + 1
    };

    explicit ThemeSettingsModel(QObject* parent = nullptr);

    void setEntries(const QVector<ThemeEntry>& entries);
    const QVector<ThemeEntry>& entries() const
    {
        return m_entries;
    }

    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    QVector<ThemeEntry> m_entries;
};
