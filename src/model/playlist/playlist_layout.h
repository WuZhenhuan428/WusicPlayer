#pragma once

#include "core/types.h"
#include "playlist.h"

#include <QString>
#include <QUuid>
#include <QVector>

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

    void setGroupRule(const QVector<SortRule>& group_rule);
    void setSortRule(const QVector<SortRule>& sort_rule);
    const QVector<SortRule> sortRules() const;
    const QVector<SortRule> groupRules() const;

    /**
     * @brief Auxiliary: used to handle event such as table header was clicked
     * @param overrideExisting true=override, false=append
     * @note only one SortRule for group is supported
     */
    void updateSort(SortRule rule, bool overrideExisting = false);

    static QVariant getMetaDataValue(const TrackMetaData& mata, SortType type);

private:
    QVector<SortRule> m_group_rules;
    QVector<SortRule> m_sort_rules;
};
