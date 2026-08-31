#pragma once

#include "core/dsl/evaluator.h"
#include "core/types.h"
#include "model/playlist/playlist.h"

#include <QString>
#include <QUuid>
#include <QVector>

#include <memory>

struct Node
{
    EntryId id; // Track UUID. If null, it's a group node.
    TrackMetaData meta;
    QString group_name;
    bool missing = false; // 文件缺失标记(库引用条目)
    Node* parent = nullptr;
    QVector<Node*> children;

    int row() const
    {
        if (parent) {
            return parent->children.indexOf(const_cast<Node*>(this));
        }
        return 0;
    }

    explicit Node(Node* p = nullptr) : parent(p) {}
    ~Node()
    {
        qDeleteAll(children);
    }
};

struct TrackEntry
{
    EntryId id;
    TrackMetaData meta;
};

struct GroupEntry
{
    QString name;
    QVector<TrackEntry> tracks;
};

struct PlaylistLayout
{
    QVector<GroupEntry> groups;
};

struct LayoutResult
{
    Node* root;
    QVector<EntryId> playback_queue;
    QVector<TrackEntry> updated_meta;
};

class PlaylistLayoutBuilder
{
public:
    LayoutResult build(const Playlist& playlist);

    void set_group_rule(const QVector<SortRule>& group_rule);
    void set_sort_rule(const QVector<SortRule>& sort_rule);
    const QVector<SortRule> sort_rules() const;
    const QVector<SortRule> group_rules() const;

    /**
     * @brief DSL 路径(与 SortRule 二选一; 设置成功后 build 优先使用 DSL)。
     * @param expression DSL 源文本; 空串 → 清除 DSL
     * @return 解析 + 静态校验是否成功; 失败时保留原 DSL 并可通过 dsl_error() 取错误
     */
    bool set_dsl(const QString& expression);
    void clear_dsl();
    bool has_dsl() const
    {
        return m_dsl != nullptr;
    }
    const QString& dsl_error() const
    {
        return m_dsl_error;
    }

    /**
     * @brief Auxiliary: used to handle event such as table header was clicked
     * @param overrideExisting true=override, false=append
     * @note only one SortRule for group is supported
     */
    void update_sort(SortRule rule, bool overrideExisting = false);

    static QVariant get_metadata_value(const TrackMetaData& mata, SortType type);

private:
    QVector<SortRule> m_group_rules;
    QVector<SortRule> m_sort_rules;

    // DSL 路径(非空时优先于 m_group_rules/m_sort_rules);
    // 用 shared_ptr: builder 会被拷贝进 worker 线程, evaluator 只读共享
    std::shared_ptr<dsl::Evaluator> m_dsl;
    QString m_dsl_error;
};
