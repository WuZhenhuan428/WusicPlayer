#include "model/library/library_browse_model.h"

#include "core/dsl/lexer.h"
#include "core/dsl/parser.h"
#include "core/dsl/registry.h"
#include "core/dsl/track_meta_row.h"
#include "core/search_types.h"
#include "model/library/library_manager.h"

#include <QCollator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeData>

#include <algorithm>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("library_browser", {"console", "gui"});
}

namespace
{

// 叶节点 internalId 最高位置 1 以区分分组/叶;组编码用 bit16-30,行编码用 bit0-15
constexpr quintptr kLeafBase = quintptr(1) << 31;

quintptr encode_group(int g)
{
    return quintptr(g + 1);
}

quintptr encode_leaf(int g, int r)
{
    return kLeafBase | (quintptr(g + 1) << 16) | quintptr(r + 1);
}

bool is_leaf(quintptr id)
{
    return id >= kLeafBase;
}

int group_row(quintptr id)
{
    return int(id) - 1;
}

void decode_leaf(quintptr id, int* g, int* r)
{
    *g = int((id >> 16) & 0x7fff) - 1;
    *r = int(id & 0xffff) - 1;
}

QString format_duration(int ms)
{
    const int total_s = ms / 1000;
    const int h       = total_s / 3600;
    const int m       = (total_s % 3600) / 60;
    const int s       = total_s % 60;
    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}

} // namespace

LibraryBrowseModel::LibraryBrowseModel(LibraryManager* lib, QObject* parent) :
    QAbstractItemModel(parent), m_lib(lib)
{
    if (m_lib != nullptr) {
        connect(m_lib, &LibraryManager::sgn_library_changed, this, &LibraryBrowseModel::refresh);
    }
    rebuild();
}

void LibraryBrowseModel::set_grouping(LibraryGrouping grouping)
{
    if (m_grouping == grouping) {
        return;
    }
    m_grouping = grouping;
    clear_dsl_grouping(); // 预设切换 → 清除 DSL 自定义分类
    rebuild();
}

bool LibraryBrowseModel::set_dsl_grouping(const QString& expression)
{
    if (expression.trimmed().isEmpty()) {
        clear_dsl_grouping();
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
    m_dsl = std::make_unique<dsl::Evaluator>(prog);
    m_dsl_error.clear();
    rebuild();
    return true;
}

void LibraryBrowseModel::clear_dsl_grouping()
{
    m_dsl.reset();
    m_dsl_error.clear();
    rebuild();
}

void LibraryBrowseModel::set_keyword(const QString& keyword)
{
    if (m_keyword == keyword) {
        return;
    }
    m_keyword = keyword;
    rebuild();
}

void LibraryBrowseModel::refresh()
{
    rebuild();
}

void LibraryBrowseModel::set_library(LibraryManager* lib)
{
    if (m_lib == lib) {
        return;
    }
    if (m_lib != nullptr) {
        disconnect(m_lib, nullptr, this, nullptr);
    }
    m_lib = lib;
    if (m_lib != nullptr) {
        connect(m_lib, &LibraryManager::sgn_library_changed, this, &LibraryBrowseModel::refresh);
    }
    rebuild();
}

std::optional<TrackId> LibraryBrowseModel::track_id_at(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return std::nullopt;
    }
    const quintptr id = quintptr(index.internalId());
    if (!is_leaf(id)) {
        return std::nullopt;
    }
    int g = 0, r = 0;
    decode_leaf(id, &g, &r);
    if (g < 0 || g >= m_groups.size()) {
        return std::nullopt;
    }
    const auto& tracks = m_groups.at(g).tracks;
    if (r < 0 || r >= tracks.size()) {
        return std::nullopt;
    }
    return tracks.at(r)->track_id;
}

QVector<TrackId> LibraryBrowseModel::collect_track_ids(const QModelIndexList& indexes) const
{
    QVector<TrackId> result;
    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }
        if (const auto tid = track_id_at(index)) {
            if (!result.contains(*tid)) {
                result.push_back(*tid);
            }
            continue;
        }
        // 分组节点:展开为该组全部曲目
        const quintptr id = quintptr(index.internalId());
        if (!is_leaf(id)) {
            const int g = group_row(id);
            if (g >= 0 && g < m_groups.size()) {
                for (const auto& t : m_groups.at(g).tracks) {
                    if (!result.contains(t->track_id)) {
                        result.push_back(t->track_id);
                    }
                }
            }
        }
    }
    return result;
}

Qt::ItemFlags LibraryBrowseModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        f |= Qt::ItemIsDragEnabled;
    }
    return f;
}

QStringList LibraryBrowseModel::mimeTypes() const
{
    return {QString::fromLatin1(wusic::kLibraryTracksMime)};
}

QMimeData* LibraryBrowseModel::mimeData(const QModelIndexList& indexes) const
{
    // 多选混合语义:全组节点 或 全曲目行 有效;混合 → 拒绝并记录日志
    bool has_group = false;
    bool has_leaf  = false;
    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }
        if (index.parent().isValid()) {
            has_leaf = true;
        } else {
            has_group = true;
        }
    }
    if (has_group && has_leaf) {
        logger->warn("[LibraryBrowseModel] mixed drag selection (group + track) rejected");
        return nullptr;
    }
    const QVector<TrackId> tids = collect_track_ids(indexes);
    if (tids.isEmpty()) {
        return nullptr;
    }
    QJsonArray arr;
    for (const TrackId& tid : tids) {
        arr.append(tid.to_string_without_brace());
    }
    auto* mime = new QMimeData;
    mime->setData(QString::fromLatin1(wusic::kLibraryTracksMime),
                  QJsonDocument(arr).toJson(QJsonDocument::Compact));
    return mime;
}

QModelIndex LibraryBrowseModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row < 0 || column < 0 || column >= columnCount(parent)) {
        return {};
    }
    if (!parent.isValid()) {
        if (row >= m_groups.size()) {
            return {};
        }
        return createIndex(row, column, encode_group(row));
    }
    const int g = group_row(quintptr(parent.internalId()));
    if (g < 0 || g >= m_groups.size() || row >= m_groups.at(g).tracks.size()) {
        return {};
    }
    return createIndex(row, column, encode_leaf(g, row));
}

QModelIndex LibraryBrowseModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) {
        return {};
    }
    const quintptr id = quintptr(child.internalId());
    if (!is_leaf(id)) {
        return {}; // 分组节点 → 根
    }
    int g                  = 0;
    [[maybe_unused]] int r = 0;
    decode_leaf(id, &g, &r);
    if (g < 0 || g >= m_groups.size()) {
        return {};
    }
    return createIndex(g, 0, encode_group(g));
}

int LibraryBrowseModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return m_groups.size();
    }
    const quintptr id = quintptr(parent.internalId());
    if (is_leaf(id)) {
        return 0; // 叶节点没有子
    }
    const int g = group_row(id);
    if (g < 0 || g >= m_groups.size()) {
        return 0;
    }
    return m_groups.at(g).tracks.size();
}

int LibraryBrowseModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
{
    return 4; // Title / Artist / Album / Duration
}

QVariant LibraryBrowseModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const int col       = index.column();
    const quintptr id   = quintptr(index.internalId());
    const bool is_leaf_ = is_leaf(id);

    if (!is_leaf_) {
        // 分组节点:第一列显示组名 + 计数
        if (role == Qt::DisplayRole && col == 0) {
            const int g = group_row(id);
            if (g < 0 || g >= m_groups.size()) {
                return {};
            }
            const Group& group = m_groups.at(g);
            QString key        = group.key;
            if (m_grouping == LibraryGrouping::none) {
                key = QStringLiteral("All Tracks");
            } else if (key.isEmpty()) {
                key = QStringLiteral("(Unknown)");
            }
            return QStringLiteral("%1 (%2)").arg(key).arg(group.tracks.size());
        }
        return {};
    }

    int g = 0, r = 0;
    decode_leaf(id, &g, &r);
    if (g < 0 || g >= m_groups.size()) {
        return {};
    }
    const auto& tracks = m_groups.at(g).tracks;
    if (r < 0 || r >= tracks.size()) {
        return {};
    }
    const auto& lt = tracks.at(r);

    if (role == Qt::DisplayRole) {
        switch (col) {
        case 0:
            return lt->meta.title.isEmpty() ? lt->meta.filename : lt->meta.title;
        case 1:
            return lt->meta.artist;
        case 2:
            return lt->meta.album;
        case 3:
            return format_duration(lt->duration_ms);
        default:
            break;
        }
    }
    if (role == Qt::ToolTipRole) {
        return lt->filepath;
    }
    return {};
}

QVariant LibraryBrowseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
    case 0:
        return QStringLiteral("Title");
    case 1:
        return QStringLiteral("Artist");
    case 2:
        return QStringLiteral("Album");
    case 3:
        return QStringLiteral("Duration");
    default:
        return {};
    }
}

QString LibraryBrowseModel::group_key(const LibraryTrack& lt) const
{
    // DSL 自定义分类优先(单级视图: bucket 全量; group 取第一级键)
    if (m_dsl) {
        dsl::TrackMetaRow row(lt.meta, lt.filepath, lt.missing ? 1 : 0);
        if (m_dsl->has_bucket())
            return m_dsl->bucket_key(row);
        if (m_dsl->has_group()) {
            const auto keys = m_dsl->group_keys(row);
            return keys.isEmpty() ? QString() : keys[0];
        }
        return QString();
    }

    switch (m_grouping) {
    case LibraryGrouping::none:
        return QString();
    case LibraryGrouping::artist:
        return lt.meta.artist;
    case LibraryGrouping::album:
        return lt.meta.album;
    case LibraryGrouping::genre:
        return lt.meta.genre;
    case LibraryGrouping::folder:
        return QFileInfo(lt.filepath).absolutePath();
    case LibraryGrouping::year:
        return lt.meta.year > 0 ? QString::number(lt.meta.year) : QString();
    }
    return QString();
}

void LibraryBrowseModel::sort_tracks(QVector<std::shared_ptr<const LibraryTrack>>& tracks)
{
    // DSL sort 优先(组内排序)
    if (m_dsl && m_dsl->has_sort()) {
        std::sort(tracks.begin(), tracks.end(),
                  [&](const std::shared_ptr<const LibraryTrack>& a,
                      const std::shared_ptr<const LibraryTrack>& b) {
                      dsl::TrackMetaRow ra(a->meta, a->filepath, a->missing ? 1 : 0);
                      dsl::TrackMetaRow rb(b->meta, b->filepath, b->missing ? 1 : 0);
                      return m_dsl->less(ra, rb);
                  });
        return;
    }
    // 默认: track_number → title
    std::sort(tracks.begin(), tracks.end(),
              [](const std::shared_ptr<const LibraryTrack>& a,
                 const std::shared_ptr<const LibraryTrack>& b) {
                  const int tn_a = a->meta.track_number > 0 ? a->meta.track_number : 1000000;
                  const int tn_b = b->meta.track_number > 0 ? b->meta.track_number : 1000000;
                  if (tn_a != tn_b) {
                      return tn_a < tn_b;
                  }
                  return a->meta.title.toLower() < b->meta.title.toLower();
              });
}

void LibraryBrowseModel::sort_groups()
{
    // DSL 分类: 组间按键排序(QCollator, 语言感知); group 首级方向生效
    if (m_dsl) {
        bool desc = false;
        if (m_dsl->has_group() && !m_dsl->group_items().isEmpty())
            desc = m_dsl->group_items()[0].desc;
        QCollator coll;
        coll.setCaseSensitivity(Qt::CaseInsensitive);
        std::sort(m_groups.begin(), m_groups.end(), [&](const Group& a, const Group& b) {
            const int c = coll.compare(a.key, b.key);
            return desc ? (c > 0) : (c < 0);
        });
        return;
    }

    if (m_grouping == LibraryGrouping::year) {
        std::sort(m_groups.begin(), m_groups.end(), [](const Group& a, const Group& b) {
            return a.key.toInt() > b.key.toInt(); // 年份降序
        });
        return;
    }
    // 空键("(Unknown)")排最后;其余按名称
    std::sort(m_groups.begin(), m_groups.end(), [](const Group& a, const Group& b) {
        const QString ka = a.key.isEmpty() ? QStringLiteral("\uffff") : a.key;
        const QString kb = b.key.isEmpty() ? QStringLiteral("\uffff") : b.key;
        return ka.toLower() < kb.toLower();
    });
}

void LibraryBrowseModel::rebuild()
{
    QVector<std::shared_ptr<const LibraryTrack>> tracks;
    if (m_lib != nullptr) {
        if (m_keyword.trimmed().isEmpty()) {
            // 全量:内存索引(共享引用, 零拷贝)
            const auto& idx = m_lib->index();
            tracks.reserve(idx.size());
            for (const auto& lt : idx) {
                tracks.append(lt);
            }
        } else {
            // 搜索:FTS5(Plain),限量
            tracks = m_lib->search(m_keyword.trimmed(), SearchQueryMode::Plain, 500);
        }
    }

    QHash<QString, QVector<std::shared_ptr<const LibraryTrack>>> by_key;
    for (const auto& lt : tracks) {
        by_key[group_key(*lt)].append(lt);
    }

    beginResetModel();
    m_groups.clear();
    m_groups.reserve(by_key.size());
    for (auto it = by_key.begin(); it != by_key.end(); ++it) {
        Group g;
        g.key    = it.key();
        g.tracks = it.value();
        sort_tracks(g.tracks);
        m_groups.append(g);
    }
    sort_groups();
    endResetModel();
}
