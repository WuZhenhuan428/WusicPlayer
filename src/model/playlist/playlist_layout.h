#pragma once

#include <QUuid>
#include <QString>
#include <QVector>

#include "playlist.h"
#include "core/types.h"
#include "core/utils/AudioUtils.h"

struct Node {
    trackId id; // Track UUID. If null, it's a group node.
    TrackMetaData meta;
    QString groupName;
    Node* parent = nullptr;
    QVector<Node*> children;

    int row() const {
        if (parent) {
            return parent->children.indexOf(const_cast<Node*>(this));
        }
        return 0;
    }

    explicit Node(Node* p = nullptr) : parent(p) {}
    ~Node() { qDeleteAll(children); }
};

struct trackEntry
{
    trackId id;
    TrackMetaData meta;
};

struct groupEntry
{
    QString name;
    QVector<trackEntry> tracks;
};

struct playlistLayout
{
    QVector<groupEntry> groups;
};

struct LayoutResult
{
    Node* root;
    QVector<trackId> playbackQueue;
    QVector<trackEntry> updatedMeta;
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
    QVector<SortRule> m_groupRules;
    QVector<SortRule> m_sortRules;
};
