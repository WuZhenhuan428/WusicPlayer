#pragma once

#include "core/types.h"
#include "model/library/library_track.h"

#include <QAbstractItemModel>
#include <QVector>

class LibraryManager;

/**
 * @brief 媒体库浏览的分类方式(预设;DSL 自定义规则后续与主视图 SortRule 统一改造)。
 */
enum class LibraryGrouping
{
    none = 0, // 平铺(单组)
    artist,
    album,
    genre,
    folder,
    year
};

/**
 * @brief 媒体库浏览模型:分组树(分组节点 → 曲目行),支持 FTS5 实时搜索。
 *
 * 数据源:无关键字 → LibraryManager 内存索引(全量);有关键字 → FTS5 搜索结果。
 * 结果统一按当前分类构建分组树;分组节点默认折叠。
 *
 * 依赖 LibraryManager(非拥有,可空);监听 sgn_library_changed 自动刷新。
 */
class LibraryBrowseModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit LibraryBrowseModel(LibraryManager* lib = nullptr, QObject* parent = nullptr);

    // 切换分类 / 关键字(空 = 全量浏览),触发重建
    void set_grouping(LibraryGrouping grouping);
    LibraryGrouping grouping() const
    {
        return m_grouping;
    }
    void set_keyword(const QString& keyword);
    QString keyword() const
    {
        return m_keyword;
    }
    // 重新拉取库数据(库变更后调用)
    void refresh();

    // 后注入数据源(非拥有,可空);切换时保留当前分组/关键字并重建
    void set_library(LibraryManager* lib);

    // 非拥有:返回内部容器引用,仅本次调用内有效
    LibraryManager* library() const
    {
        return m_lib;
    }

    // 叶节点 → 库级曲目身份;分组节点 / 无效索引返回 nullopt
    std::optional<TrackId> track_id_at(const QModelIndex& index) const;

    // QAbstractItemModel 接口
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    struct Group
    {
        QString key; // 分组键(空 → "(Unknown)")
        QVector<LibraryTrack> tracks;
    };

    void rebuild();
    QString group_key(const LibraryTrack& lt) const;
    void sort_groups();

    LibraryManager* m_lib      = nullptr; // 非拥有
    LibraryGrouping m_grouping = LibraryGrouping::artist;
    QString m_keyword;
    QVector<Group> m_groups;
};
