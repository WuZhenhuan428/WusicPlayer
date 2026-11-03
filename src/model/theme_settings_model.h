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
    bool is_dark = false;
};

/// 主题浏览模型——展示所有可用主题（含系统/内置/外部）。
/// 数据由 ThemeService::scan_themes() 填充。
class ThemeSettingsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column // NOLINT(cppcoreguidelines-use-enum-class)
    {
        ColName = 0,
        ColSource,
        ColAuthor,
        ColDark,
        ColCount,
    };
    enum UserRole // NOLINT(cppcoreguidelines-use-enum-class)
    {
        RoleIsDark = Qt::UserRole + 1,
    };

    explicit ThemeSettingsModel(QObject* parent = nullptr);

    void set_entries(const QVector<ThemeEntry>& entries);
    [[nodiscard]] const QVector<ThemeEntry>& entries() const
    {
        return entries_;
    }

    // QAbstractTableModel interface
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role) const override;

private:
    QVector<ThemeEntry> entries_;
};
