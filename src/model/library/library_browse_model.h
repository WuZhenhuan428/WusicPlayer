#pragma once

#include "core/dsl/evaluator.h"
#include "core/types.h"
#include "model/library/library_track.h"

#include <QAbstractItemModel>
#include <QMimeData>
#include <QStringList>
#include <QVector>

#include <memory>

class LibraryManager;

/**
 * @brief 媒体库浏览的分类方式(预设;DSL 自定义规则后续与主视图 SortRule 统一改造)。
 */
enum class LibraryGrouping
{
    None = 0, // 平铺(单组)
    Artist,
    Album,
    Genre,
    Folder,
    Year,
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
    [[nodiscard]] LibraryGrouping grouping() const
    {
        return grouping_;
    }
    /**
     * @brief DSL 自定义分类(与预设 set_grouping 互斥; 设置成功后优先)。
     * 单级视图: bucket 全量生效; group 取第一级键。
     * @return 解析 + 校验是否成功; 失败保留原状态, 错误见 dsl_error()
     */
    bool set_dsl_grouping(const QString& expression);
    void clear_dsl_grouping();
    [[nodiscard]] const QString& dsl_error() const
    {
        return dsl_error_;
    }
    [[nodiscard]] bool has_dsl() const
    {
        return dsl_ != nullptr;
    }
    /// 当前生效的 DSL 源文本(空 = 未启用 DSL)。
    [[nodiscard]] const QString& dsl_source() const
    {
        return dsl_source_;
    }
    void set_keyword(const QString& keyword);
    [[nodiscard]] QString keyword() const
    {
        return keyword_;
    }
    // 重新拉取库数据(库变更后调用)
    void refresh();

    // 后注入数据源(非拥有,可空);切换时保留当前分组/关键字并重建
    void set_library(LibraryManager* lib);

    // 非拥有:返回内部容器引用,仅本次调用内有效
    [[nodiscard]] LibraryManager* library() const
    {
        return lib_;
    }

    // 叶节点 → 库级曲目身份;分组节点 / 无效索引返回 nullopt
    [[nodiscard]] std::optional<TrackId> track_id_at(const QModelIndex& index) const;
    // 收集索引集合对应的库曲目身份:曲目行直接取;分组行展开为该组全部曲目
    [[nodiscard]] QVector<TrackId> collect_track_ids(const QModelIndexList& indexes) const;

    // QAbstractItemModel 接口
    [[nodiscard]] QModelIndex index(int row, int column,
                                    const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& child) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;

    // 拖拽源:对外提供库曲目 TrackId 列表(JSON 序列化)
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role) const override;

private:
    struct Group
    {
        QString key; // 分组键(空 → "(Unknown)")
        // 共享引用(指向 Library 权威副本), 避免整份复制
        QVector<std::shared_ptr<const LibraryTrack>> tracks;
    };

    void rebuild();
    [[nodiscard]] QString group_key(const LibraryTrack& lt) const;
    void sort_tracks(QVector<std::shared_ptr<const LibraryTrack>>& tracks);
    void sort_groups();

    LibraryManager* lib_      = nullptr; // 非拥有
    LibraryGrouping grouping_ = LibraryGrouping::Artist;
    QString keyword_;
    QVector<Group> groups_;

    // DSL 自定义分类(非空时优先于 m_grouping)
    std::unique_ptr<dsl::Evaluator> dsl_;
    QString dsl_error_;
    QString dsl_source_; // 当前生效的 DSL 源文本
};
