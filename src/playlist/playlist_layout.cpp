#include "playlist_layout.h"
#include <QVariant>
#include <functional>
#include <QFileInfo>
#include <QMap>
#include <QCollator>

static std::function<bool(const Node*, const Node*)>
createComparator(const QVector<SortRule>& rules) {
    return [rules](const Node* a, const Node* b) -> bool {
        for (const auto& rule : rules) {
            QVariant valA = PlaylistLayoutBuilder::getMetaDataValue(a->meta, rule.type);
            QVariant valB = PlaylistLayoutBuilder::getMetaDataValue(b->meta, rule.type);

            int cmp = 0;
            if (valA.typeId() == QMetaType::Int) {
                int ia = valA.toInt();
                int ib = valB.toInt();
                cmp = (ia < ib) ? -1 : (ia > ib ? 1 : 0);
            } else {
                cmp = QString::localeAwareCompare(valA.toString(), valB.toString());
            }

            if (cmp != 0) {
                return (rule.order == Qt::AscendingOrder) ? (cmp<0) : (cmp>0);
            }
        }
        return false;
    };
}

LayoutResult PlaylistLayoutBuilder::build(const Playlist& playlist) {
    LayoutResult result;
    result.root = new Node();

    // get tracks' metadata
    QVector<Node*> trackNodes;
    QVector<Track> tracks = playlist.getTracks();
    for (const auto& t : tracks) {
        Node* node = new Node();
        node->id = t.uuid;
        node->meta = Audio::parse(t.filepath.toStdString());
        if (!node->meta.isValid) {
            node->meta.title = QFileInfo(t.filepath).fileName();
        }
        trackNodes.append(node);
    }

    // global sorting: make the items in the same group are relatively orderly
    if (!m_sortRules.isEmpty()) {
        std::sort(trackNodes.begin(), trackNodes.end(), createComparator(m_sortRules));
    }

    // grouping
    std::function<void(Node*, QVector<Node*>&, int)> 
    processGroup = [&](Node* parent, QVector<Node*>& nodes, int levelIndex) {
        if (levelIndex >= m_groupRules.size()) {
            parent->children.append(nodes);
            for (auto node : nodes)
                node->parent = parent;
            return;
        }

        SortRule current_sort_rule = m_groupRules[levelIndex];

        QMap<QString, QVector<Node*>> buckets;
        for (Node* node : nodes) {
            QString key = getMetaDataValue(node->meta, current_sort_rule.type).toString();
            if (key.isEmpty())
                key = "unknown";
            buckets[key].append(node);
        }

        for (auto it = buckets.begin(); it != buckets.end(); ++it) {
            Node* groupNode = new Node(parent);
            groupNode->groupName = it.key();
            parent->children.append(groupNode);

            processGroup(groupNode, it.value(), levelIndex + 1);
        }
    };
    processGroup(result.root, trackNodes, 0);


    // get linear playback queue
    std::function<void(Node*)>
    collectLeaves = [&](Node* n) {
        if (!n->id.isNull()) {
            result.playbackQueue.append(n->id);
            return;
        }
        for (Node* child : n->children)
            collectLeaves(child);
    };
    collectLeaves(result.root);

    return result;
}

void PlaylistLayoutBuilder::updateSort(SortRule rule, bool overrideExisting) {
    SortRule single_group_rule = rule;
    QVector<SortRule> new_sort_rules;

    for (auto r : m_groupRules) {
        if (r.type != rule.type) {
            new_sort_rules.append(rule);
        }
    }
    for (auto r : m_sortRules) {
        if (r.type != rule.type) {
            new_sort_rules.append(rule);
        }
    }

    m_groupRules.clear();
    m_groupRules.append(single_group_rule);

    m_sortRules.clear();
    m_sortRules = new_sort_rules;
}

void PlaylistLayoutBuilder::setGroupRule(const QVector<SortRule>& group_rule) {
    this->m_groupRules.clear();
    if (group_rule.isEmpty()) {
        return;
    } else {
        for (const auto& it : group_rule) {
            this->m_groupRules.append(it);
        }
    }
}

void PlaylistLayoutBuilder::setSortRule(const QVector<SortRule>& sort_rule) {
    this->m_sortRules.clear();
    if (sort_rule.isEmpty()) {
        return;
    } else {
        for (const auto & it : sort_rule) {
            this->m_sortRules.append(it);
        }
    }
}


QVariant PlaylistLayoutBuilder::getMetaDataValue(const TrackMetaData& meta, SortType type) {
    static const QHash<SortType, QString TrackMetaData::*> strMap {
        {SortType::album       , &TrackMetaData::album},
        {SortType::album_artist, &TrackMetaData::album_artist},
        {SortType::artist      , &TrackMetaData::artist},
        {SortType::bitrate     , &TrackMetaData::album_artist},
        {SortType::composer    , &TrackMetaData::composer},
        {SortType::directory   , &TrackMetaData::filepath},
        {SortType::filename    , &TrackMetaData::filename},
        {SortType::genre       , &TrackMetaData::genre},
        {SortType::title       , &TrackMetaData::title}
    };

    static const QHash<SortType, int TrackMetaData::*> intMap {
        {SortType::disc_number , &TrackMetaData::disc_number},
        {SortType::track_number, &TrackMetaData::track_number},
        {SortType::year        , &TrackMetaData::year}
    };

    if (type == SortType::not_sorted) {
        return QVariant();
    }
    if (type ==SortType::directory) {
        return QFileInfo(meta.filepath).absolutePath();
    }

    if (strMap.contains(type)) {
        return meta.*(strMap.value(type));
    }
    if (intMap.contains(type)) {
        return meta.*(intMap.value(type));
    }
    return QVariant();
}