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
#include <ranges>

static std::function<bool(const Node*, const Node*)>
create_comparator(const QVector<SortRule>& rules)
{
    return [rules](const Node* a, const Node* b) -> bool {
        for (const auto& rule : rules) {
            QVariant val_a = PlaylistLayoutBuilder::get_metadata_value(a->meta, rule.type);
            QVariant val_b = PlaylistLayoutBuilder::get_metadata_value(b->meta, rule.type);

            int cmp        = 0;
            if (val_a.typeId() == QMetaType::Int) {
                int ia = val_a.toInt();
                int ib = val_b.toInt();
                if (ia < ib) {
                    cmp = -1;
                } else if (ia > ib) {
                    cmp = 1;
                } else {
                    cmp = 0;
                }
            } else {
                cmp = QString::compare(val_a.toString(), val_b.toString(), Qt::CaseInsensitive);
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
    QVector<Node*> track_nodes;
    QVector<Track> tracks = playlist.get_tracks();
    for (const auto& t : tracks) {
        Node* node    = new Node();
        node->id      = t.entry_id;
        node->missing = t.missing;
        if (t.meta.is_valid) {
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
            if (!node->meta.is_valid) {
                node->meta.title = QFileInfo(t.filepath).fileName();
            }
            node->meta = utils::audio::format(node->meta);
            result.updated_meta.append({t.entry_id, node->meta});
        }
        track_nodes.append(node);
    }

    // ---- 全局排序 + 分组 ----
    if (dsl_) {
        // 原始序号(供 "index" 属性)
        QHash<const Node*, int> orig_index;
        for (int i = 0; i < track_nodes.size(); ++i) {
            orig_index.insert(track_nodes[i], i);
        }

        // 全局排序
        if (dsl_->has_sort()) {
            std::ranges::sort(track_nodes, [&](const Node* a, const Node* b) {
                dsl::TrackMetaRow ra(&a->meta);
                ra.set_index(orig_index.value(a));
                dsl::TrackMetaRow rb(&b->meta);
                rb.set_index(orig_index.value(b));
                return dsl_->less(ra, rb);
            });
        }

        // 多级分组
        std::function<void(Node*, QVector<Node*>&, int)> dsl_group = [&](Node* parent,
                                                                         QVector<Node*>& nodes,
                                                                         int level) {
            if (level >= dsl_->group_items().size()) {
                parent->children.append(nodes);
                for (Node* node : nodes) {
                    node->parent = parent;
                }
                return;
            }
            QMap<QString, QVector<Node*>> buckets;
            for (Node* node : nodes) {
                dsl::TrackMetaRow row(&node->meta);
                row.set_index(orig_index.value(node));
                const auto keys   = dsl_->group_keys(row);
                const QString key = (level < keys.size()) ? keys[level] : QStringLiteral("unknown");
                buckets[key].append(node);
            }
            const bool desc  = dsl_->group_items()[level].desc;
            QStringList keys = buckets.keys();
            QCollator coll;
            coll.setCaseSensitivity(Qt::CaseInsensitive);
            std::ranges::sort(keys, [&](const QString& a, const QString& b) {
                const int c = coll.compare(a, b);
                return desc ? (c > 0) : (c < 0);
            });
            for (const auto& key : keys) {
                Node* group_node       = new Node(parent);
                group_node->group_name = key;
                parent->children.append(group_node);
                dsl_group(group_node, buckets[key], level + 1);
            }
        };

        // 单级分类(bucket)
        auto build_bucket = [&]() {
            QMap<QString, QVector<Node*>> buckets;
            for (Node* node : track_nodes) {
                dsl::TrackMetaRow row(&node->meta);
                row.set_index(orig_index.value(node));
                buckets[dsl_->bucket_key(row)].append(node);
            }
            QStringList keys = buckets.keys();
            QCollator coll;
            coll.setCaseSensitivity(Qt::CaseInsensitive);
            std::ranges::sort(
                keys, [&](const QString& a, const QString& b) { return coll.compare(a, b) < 0; });
            for (const auto& key : keys) {
                Node* group_node       = new Node(result.root);
                group_node->group_name = key;
                result.root->children.append(group_node);
                for (Node* node : buckets[key]) {
                    node->parent = group_node;
                    group_node->children.append(node);
                }
            }
        };

        if (dsl_->has_group()) {
            dsl_group(result.root, track_nodes, 0);
        } else if (dsl_->has_bucket()) {
            build_bucket();
        } else {
            result.root->children.append(track_nodes);
            for (Node* node : track_nodes) {
                node->parent = result.root;
            }
        }
    } else {
        // 旧路径: SortRule(列头点击等)
        if (!sort_rules_.isEmpty()) {
            std::ranges::sort(track_nodes, create_comparator(sort_rules_));
        }
        std::function<void(Node*, QVector<Node*>&, int)> process_group =
            [&](Node* parent, QVector<Node*>& nodes, int level_index) {
                if (level_index >= group_rules_.size()) {
                    parent->children.append(nodes);
                    for (Node* node : nodes) {
                        node->parent = parent;
                    }
                    return;
                }

                SortRule current_sort_rule = group_rules_[level_index];

                QMap<QString, QVector<Node*>> buckets;
                for (Node* node : nodes) {
                    QString key = get_metadata_value(node->meta, current_sort_rule.type).toString();
                    if (key.isEmpty()) {
                        key = "unknown";
                    }
                    buckets[key].append(node);
                }

                // Custom key sorting to ignore case
                QStringList keys = buckets.keys();
                std::ranges::sort(keys, [](const QString& s1, const QString& s2) {
                    if (s1.contains("Unknown") && !s2.contains("Unknown")) {
                        return true;
                    }
                    if (!s1.contains("Unknown") && s2.contains("Unknown")) {
                        return false;
                    }
                    return s1.compare(s2, Qt::CaseInsensitive) < 0;
                });

                for (const auto& key : keys) {
                    Node* group_node       = new Node(parent);
                    group_node->group_name = key;
                    parent->children.append(group_node);
                    process_group(group_node, buckets[key], level_index + 1);
                }
            };
        process_group(result.root, track_nodes, 0);
    }

    // get linear playback queue
    std::function<void(Node*)> collect_leaves = [&](Node* n) {
        if (!n->id.is_null()) {
            result.playback_queue.append(n->id);
            return;
        }
        for (Node* child : n->children) {
            collect_leaves(child);
        }
    };
    collect_leaves(result.root);

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
        dsl_error_ = lexer.error_message();
        return false;
    }
    dsl::Parser parser(toks);
    auto prog = parser.parse();
    if (!prog.ok) {
        dsl_error_ = prog.error;
        return false;
    }
    if (!dsl::Registry::instance().validate(prog)) {
        dsl_error_ = prog.error;
        return false;
    }
    // 成功: 替换旧 DSL(旧的 SortRule 规则在 build 时被 DSL 屏蔽, 无需清空)
    dsl_ = std::make_shared<dsl::Evaluator>(prog);
    dsl_error_.clear();
    dsl_source_ = expression;
    return true;
}

void PlaylistLayoutBuilder::clear_dsl()
{
    dsl_.reset();
    dsl_error_.clear();
    dsl_source_.clear();
}

void PlaylistLayoutBuilder::update_sort(SortRule rule, bool override_existing)
{
    if (override_existing) {
        group_rules_.clear();
        sort_rules_.clear();
    }
    // 列头点击等手动规则: 覆盖 DSL(用户显式改内置规则)
    clear_dsl();

    group_rules_
        .clear(); // Currently enforcing single grouping for this method based on usage context
    group_rules_.append(rule);

    sort_rules_.clear();

    // Default smart sorting: if grouping by Album, sort tracks by Disc -> Track#
    if (rule.type == SortType::Album) {
        sort_rules_.append({SortType::DiscNumber, Qt::AscendingOrder});
        sort_rules_.append({SortType::TrackNumber, Qt::AscendingOrder});
    }
    // If grouping by Artist, sort by Year -> Album -> Track#
    else if (rule.type == SortType::Artist) {
        sort_rules_.append({SortType::Year, Qt::DescendingOrder});
        sort_rules_.append({SortType::Album, Qt::AscendingOrder});
        sort_rules_.append({SortType::TrackNumber, Qt::AscendingOrder});
    }
    // Default fallback: Title
    else {
        sort_rules_.append({SortType::Title, Qt::AscendingOrder});
    }
}

void PlaylistLayoutBuilder::set_group_rule(const QVector<SortRule>& group_rule)
{
    this->group_rules_.clear();
    if (group_rule.isEmpty()) {
        return;
    }
    for (const auto& it : group_rule) {
        this->group_rules_.append(it);
    }
}

void PlaylistLayoutBuilder::set_sort_rule(const QVector<SortRule>& sort_rule)
{
    this->sort_rules_.clear();
    if (sort_rule.isEmpty()) {
        return;
    }
    for (const auto& it : sort_rule) {
        this->sort_rules_.append(it);
    }
}

QVector<SortRule> PlaylistLayoutBuilder::sort_rules() const
{
    return this->sort_rules_;
}

QVector<SortRule> PlaylistLayoutBuilder::group_rules() const
{
    return this->group_rules_;
}

QVariant PlaylistLayoutBuilder::get_metadata_value(const TrackMetaData& meta, SortType type)
{
    static const QHash<SortType, QString TrackMetaData::*> str_map{
        {SortType::Album, &TrackMetaData::album},
        {SortType::AlbumArtist, &TrackMetaData::album_artist},
        {SortType::Artist, &TrackMetaData::artist},
        {SortType::Composer, &TrackMetaData::composer},
        {SortType::Directory, &TrackMetaData::filepath},
        {SortType::Filename, &TrackMetaData::filename},
        {SortType::Genre, &TrackMetaData::genre},
        {SortType::Title, &TrackMetaData::title},
    };

    static const QHash<SortType, int TrackMetaData::*> int_map{
        {SortType::Bitrate, &TrackMetaData::bitrate},
        {SortType::DiscNumber, &TrackMetaData::disc_number},
        {SortType::Duration, &TrackMetaData::duration_s},
        {SortType::TrackNumber, &TrackMetaData::track_number},
        {SortType::Year, &TrackMetaData::year},
    };

    if (type == SortType::NotSorted) {
        return {};
    }
    if (type == SortType::Directory) {
        return QFileInfo(meta.filepath).absolutePath();
    }

    if (str_map.contains(type)) {
        return meta.*str_map.value(type);
    }
    if (int_map.contains(type)) {
        return meta.*int_map.value(type);
    }
    return {};
}
