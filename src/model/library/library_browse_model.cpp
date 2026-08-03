#include "library_browse_model.h"

#include "core/search_types.h"
#include "model/library/library_manager.h"

#include <QFileInfo>

#include <algorithm>

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

// 组内排序:track_number → title
void sort_tracks(QVector<LibraryTrack>& tracks)
{
    std::sort(tracks.begin(), tracks.end(), [](const LibraryTrack& a, const LibraryTrack& b) {
        const int tn_a = a.meta.track_number > 0 ? a.meta.track_number : 1000000;
        const int tn_b = b.meta.track_number > 0 ? b.meta.track_number : 1000000;
        if (tn_a != tn_b) {
            return tn_a < tn_b;
        }
        return a.meta.title.toLower() < b.meta.title.toLower();
    });
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
    return tracks.at(r).track_id;
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
    int g = 0, r = 0;
    decode_leaf(id, &g, &r);
    Q_UNUSED(r);
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

int LibraryBrowseModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
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
    const LibraryTrack& lt = tracks.at(r);

    if (role == Qt::DisplayRole) {
        switch (col) {
        case 0:
            return lt.meta.title.isEmpty() ? lt.meta.filename : lt.meta.title;
        case 1:
            return lt.meta.artist;
        case 2:
            return lt.meta.album;
        case 3:
            return format_duration(lt.duration_ms);
        default:
            break;
        }
    }
    if (role == Qt::ToolTipRole) {
        return lt.filepath;
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

void LibraryBrowseModel::sort_groups()
{
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
    QVector<LibraryTrack> tracks;
    if (m_lib != nullptr) {
        if (m_keyword.trimmed().isEmpty()) {
            // 全量:内存索引
            const auto& idx = m_lib->index();
            tracks.reserve(idx.size());
            for (const LibraryTrack& lt : idx) {
                tracks.append(lt);
            }
        } else {
            // 搜索:FTS5(Plain),限量
            tracks = m_lib->search(m_keyword.trimmed(), SearchQueryMode::Plain, 500);
        }
    }

    QHash<QString, QVector<LibraryTrack>> by_key;
    for (const LibraryTrack& lt : tracks) {
        by_key[group_key(lt)].append(lt);
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
