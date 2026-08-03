#include "playlist_layout.h"

#include "core/utils/audio.hpp"

#include <QCollator>
#include <QFileInfo>
#include <QMap>
#include <QVariant>
#include <functional>

static std::function<bool(const Node*, const Node*)>
createComparator(const QVector<SortRule>& rules)
{
    return [rules](const Node* a, const Node* b) -> bool {
        for (const auto& rule : rules) {
            QVariant valA = PlaylistLayoutBuilder::get_metadata_value(a->meta, rule.type);
            QVariant valB = PlaylistLayoutBuilder::get_metadata_value(b->meta, rule.type);

            int cmp       = 0;
            if (valA.typeId() == QMetaType::Int) {
                int ia = valA.toInt();
                int ib = valB.toInt();
                cmp    = (ia < ib) ? -1 : (ia > ib ? 1 : 0);
            } else {
                cmp = QString::compare(valA.toString(), valB.toString(), Qt::CaseInsensitive);
            }

            if (cmp != 0) {
                return (rule.order == Qt::AscendingOrder) ? (cmp < 0) : (cmp > 0);
            }
        }
        return false;
    };
}

LayoutResult PlaylistLayoutBuilder::build(const Playlist& playlist)
{
    LayoutResult result;
    result.root = new Node();

    // get tracks' metadata
    QVector<Node*> trackNodes;
    QVector<Track> tracks = playlist.get_tracks();
    for (const auto& t : tracks) {
        Node* node    = new Node();
        node->id      = t.entry_id;
        node->missing = t.missing;
        if (t.meta.isValid) {
            node->meta = t.meta;
            if (node->meta.filepath.isEmpty()) {
                node->meta.filepath = t.filepath;
            }
            if (node->meta.filename.isEmpty()) {
                node->meta.filename = QFileInfo(t.filepath).fileName();
            }
        } else {
            node->meta          = utils::audio::parse_to_local_meta(t.filepath);
            node->meta.filepath = t.filepath;
            if (!node->meta.isValid) {
                node->meta.title = QFileInfo(t.filepath).fileName();
            }
            node->meta = utils::audio::format(node->meta);
            result.updated_meta.append({t.entry_id, node->meta});
        }
        trackNodes.append(node);
    }

    // global sorting: make the items in the same group are relatively orderly
    if (!m_sort_rules.isEmpty()) {
        std::sort(trackNodes.begin(), trackNodes.end(), createComparator(m_sort_rules));
    }

    // grouping
    std::function<void(Node*, QVector<Node*>&, int)> processGroup =
        [&](Node* parent, QVector<Node*>& nodes, int levelIndex) {
            if (levelIndex >= m_group_rules.size()) {
                parent->children.append(nodes);
                for (auto node : nodes)
                    node->parent = parent;
                return;
            }

            SortRule current_sort_rule = m_group_rules[levelIndex];

            QMap<QString, QVector<Node*>> buckets;
            for (Node* node : nodes) {
                QString key = get_metadata_value(node->meta, current_sort_rule.type).toString();
                // @note: utils::audio::parse_to_local_meta()将对内容进行格式化, if表达式将废弃
                if (key.isEmpty())
                    key = "unknown";
                buckets[key].append(node);
            }

            // Custom key sorting to ignore case
            QStringList keys = buckets.keys();
            std::sort(keys.begin(), keys.end(), [](const QString& s1, const QString& s2) {
                // @note: 此处为简易判断
                // @todo: 完善判断逻辑
                if (s1.contains("Unknown") && !(s2.contains("Unknown")))
                    return true;
                if (!(s1.contains("Unknown")) && s2.contains("Unknown"))
                    return false;
                return s1.compare(s2, Qt::CaseInsensitive) < 0;
            });

            for (const auto& key : keys) {
                Node* groupNode       = new Node(parent);
                groupNode->group_name = key;
                parent->children.append(groupNode);

                processGroup(groupNode, buckets[key], levelIndex + 1);
            }
        };
    processGroup(result.root, trackNodes, 0);

    // get linear playback queue
    std::function<void(Node*)> collectLeaves = [&](Node* n) {
        if (!n->id.isNull()) {
            result.playback_queue.append(n->id);
            return;
        }
        for (Node* child : n->children)
            collectLeaves(child);
    };
    collectLeaves(result.root);

    return result;
}

void PlaylistLayoutBuilder::update_sort(SortRule rule, bool overrideExisting)
{
    if (overrideExisting) {
        m_group_rules.clear();
        m_sort_rules.clear();
    }

    m_group_rules
        .clear(); // Currently enforcing single grouping for this method based on usage context
    m_group_rules.append(rule);

    m_sort_rules.clear();

    // Default smart sorting: if grouping by Album, sort tracks by Disc -> Track#
    if (rule.type == SortType::album) {
        m_sort_rules.append({SortType::disc_number, Qt::AscendingOrder});
        m_sort_rules.append({SortType::track_number, Qt::AscendingOrder});
    }
    // If grouping by Artist, sort by Year -> Album -> Track#
    else if (rule.type == SortType::artist) {
        m_sort_rules.append({SortType::year, Qt::DescendingOrder});
        m_sort_rules.append({SortType::album, Qt::AscendingOrder});
        m_sort_rules.append({SortType::track_number, Qt::AscendingOrder});
    }
    // Default fallback: Title
    else {
        m_sort_rules.append({SortType::title, Qt::AscendingOrder});
    }
}

void PlaylistLayoutBuilder::set_group_rule(const QVector<SortRule>& group_rule)
{
    this->m_group_rules.clear();
    if (group_rule.isEmpty()) {
        return;
    } else {
        for (const auto& it : group_rule) {
            this->m_group_rules.append(it);
        }
    }
}

void PlaylistLayoutBuilder::set_sort_rule(const QVector<SortRule>& sort_rule)
{
    this->m_sort_rules.clear();
    if (sort_rule.isEmpty()) {
        return;
    } else {
        for (const auto& it : sort_rule) {
            this->m_sort_rules.append(it);
        }
    }
}

const QVector<SortRule> PlaylistLayoutBuilder::sort_rules() const
{
    return this->m_sort_rules;
}

const QVector<SortRule> PlaylistLayoutBuilder::group_rules() const
{
    return this->m_group_rules;
}

QVariant PlaylistLayoutBuilder::get_metadata_value(const TrackMetaData& meta, SortType type)
{
    static const QHash<SortType, QString TrackMetaData::*> strMap{
        {SortType::album, &TrackMetaData::album},
        {SortType::album_artist, &TrackMetaData::album_artist},
        {SortType::artist, &TrackMetaData::artist},
        {SortType::composer, &TrackMetaData::composer},
        {SortType::directory, &TrackMetaData::filepath},
        {SortType::filename, &TrackMetaData::filename},
        {SortType::genre, &TrackMetaData::genre},
        {SortType::title, &TrackMetaData::title}};

    static const QHash<SortType, int TrackMetaData::*> intMap{
        {SortType::bitrate, &TrackMetaData::bitrate},
        {SortType::disc_number, &TrackMetaData::disc_number},
        {SortType::duration, &TrackMetaData::duration_s},
        {SortType::track_number, &TrackMetaData::track_number},
        {SortType::year, &TrackMetaData::year}};

    if (type == SortType::not_sorted) {
        return QVariant();
    }
    if (type == SortType::directory) {
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
