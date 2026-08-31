#include "playlist_layout.h"

#include "core/dsl/lexer.h"
#include "core/dsl/parser.h"
#include "core/dsl/registry.h"
#include "core/dsl/track_meta_row.h"
#include "core/utils/audio.hpp"

#include <QCollator>
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QStringList>
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

    // ---- 全局排序 + 分组 ----
    if (m_dsl) {
        // 原始序号(供 "index" 属性)
        QHash<const Node*, int> origIndex;
        for (int i = 0; i < trackNodes.size(); ++i)
            origIndex.insert(trackNodes[i], i);

        // 全局排序
        if (m_dsl->has_sort()) {
            std::sort(trackNodes.begin(), trackNodes.end(), [&](const Node* a, const Node* b) {
                dsl::TrackMetaRow ra(a->meta);
                ra.set_index(origIndex.value(a));
                dsl::TrackMetaRow rb(b->meta);
                rb.set_index(origIndex.value(b));
                return m_dsl->less(ra, rb);
            });
        }

        // 多级分组
        std::function<void(Node*, QVector<Node*>&, int)> dslGroup = [&](Node* parent,
                                                                        QVector<Node*>& nodes,
                                                                        int level) {
            if (level >= m_dsl->group_items().size()) {
                parent->children.append(nodes);
                for (auto node : nodes)
                    node->parent = parent;
                return;
            }
            QMap<QString, QVector<Node*>> buckets;
            for (Node* node : nodes) {
                dsl::TrackMetaRow row(node->meta);
                row.set_index(origIndex.value(node));
                const auto keys   = m_dsl->group_keys(row);
                const QString key = (level < keys.size()) ? keys[level] : QStringLiteral("unknown");
                buckets[key].append(node);
            }
            const bool desc  = m_dsl->group_items()[level].desc;
            QStringList keys = buckets.keys();
            QCollator coll;
            coll.setCaseSensitivity(Qt::CaseInsensitive);
            std::sort(keys.begin(), keys.end(), [&](const QString& a, const QString& b) {
                const int c = coll.compare(a, b);
                return desc ? (c > 0) : (c < 0);
            });
            for (const auto& key : keys) {
                Node* groupNode       = new Node(parent);
                groupNode->group_name = key;
                parent->children.append(groupNode);
                dslGroup(groupNode, buckets[key], level + 1);
            }
        };

        // 单级分类(bucket)
        auto buildBucket = [&]() {
            QMap<QString, QVector<Node*>> buckets;
            for (Node* node : trackNodes) {
                dsl::TrackMetaRow row(node->meta);
                row.set_index(origIndex.value(node));
                buckets[m_dsl->bucket_key(row)].append(node);
            }
            QStringList keys = buckets.keys();
            QCollator coll;
            coll.setCaseSensitivity(Qt::CaseInsensitive);
            std::sort(keys.begin(), keys.end(),
                      [&](const QString& a, const QString& b) { return coll.compare(a, b) < 0; });
            for (const auto& key : keys) {
                Node* groupNode       = new Node(result.root);
                groupNode->group_name = key;
                result.root->children.append(groupNode);
                for (auto node : buckets[key]) {
                    node->parent = groupNode;
                    groupNode->children.append(node);
                }
            }
        };

        if (m_dsl->has_group()) {
            dslGroup(result.root, trackNodes, 0);
        } else if (m_dsl->has_bucket()) {
            buildBucket();
        } else {
            result.root->children.append(trackNodes);
            for (auto node : trackNodes)
                node->parent = result.root;
        }
    } else {
        // 旧路径: SortRule(列头点击等)
        if (!m_sort_rules.isEmpty()) {
            std::sort(trackNodes.begin(), trackNodes.end(), createComparator(m_sort_rules));
        }
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
                    if (key.isEmpty())
                        key = "unknown";
                    buckets[key].append(node);
                }

                // Custom key sorting to ignore case
                QStringList keys = buckets.keys();
                std::sort(keys.begin(), keys.end(), [](const QString& s1, const QString& s2) {
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
    }

    // get linear playback queue
    std::function<void(Node*)> collectLeaves = [&](Node* n) {
        if (!n->id.is_null()) {
            result.playback_queue.append(n->id);
            return;
        }
        for (Node* child : n->children)
            collectLeaves(child);
    };
    collectLeaves(result.root);

    return result;
}

bool PlaylistLayoutBuilder::set_dsl(const QString& expression)
{
    if (expression.trimmed().isEmpty()) {
        clear_dsl();
        return true;
    }

    dsl::Lexer lexer{QStringView(expression)};
    const auto toks = lexer.tokenize();
    if (lexer.has_error()) {
        m_dsl_error = lexer.error_message();
        return false;
    }
    dsl::Parser parser(toks);
    auto prog = parser.parse();
    if (!prog.ok) {
        m_dsl_error = prog.error;
        return false;
    }
    if (!dsl::Registry::instance().validate(prog)) {
        m_dsl_error = prog.error;
        return false;
    }
    // 成功: 替换旧 DSL(旧的 SortRule 规则在 build 时被 DSL 屏蔽, 无需清空)
    m_dsl = std::make_shared<dsl::Evaluator>(prog);
    m_dsl_error.clear();
    return true;
}

void PlaylistLayoutBuilder::clear_dsl()
{
    m_dsl.reset();
    m_dsl_error.clear();
}

void PlaylistLayoutBuilder::update_sort(SortRule rule, bool overrideExisting)
{
    if (overrideExisting) {
        m_group_rules.clear();
        m_sort_rules.clear();
    }
    // 列头点击等手动规则: 覆盖 DSL(用户显式改内置规则)
    clear_dsl();

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
