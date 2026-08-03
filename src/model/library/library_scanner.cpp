#include "library_scanner.h"

#include "core/utils/audio.hpp"
#include "core/utils/path.hpp"

#include <QFileInfo>

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

LibraryScanner::LibraryScanner(QObject* parent) : QObject(parent) {}

namespace
{
// 递归收集根目录下的音频文件(跳过符号链接与权限错误)
void collect_audio_files(const fs::path& dir, QVector<fs::path>& out)
{
    std::error_code ec;
    fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        return;
    }
    const fs::directory_iterator end;
    for (; it != end; it.increment(ec)) {
        if (ec) {
            break;
        }
        const fs::directory_entry& entry = *it;
        std::error_code symlink_ec;
        if (entry.is_symlink(symlink_ec)) {
            continue; // 跳过符号链接,避免循环
        }
        if (entry.is_directory(ec)) {
            collect_audio_files(entry.path(), out);
        } else if (!ec && entry.is_regular_file(ec) && utils::audio::is_audio_file(entry.path())) {
            out.append(entry.path());
        }
    }
}

qint64 file_mtime_secs(const fs::path& p)
{
    std::error_code ec;
    const auto time = fs::last_write_time(p, ec);
    if (ec) {
        return 0;
    }
    return std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
}

qint64 fs_file_size(const fs::path& p)
{
    std::error_code ec;
    const auto s = fs::file_size(p, ec);
    return ec ? 0 : static_cast<qint64>(s);
}

// 规范化根目录列表
QStringList normalize_roots(const QStringList& roots)
{
    QStringList result;
    for (const auto& root : roots) {
        result.append(utils::path::normalize_path(root));
    }
    return result;
}

// 判断 path 是否位于某个根目录之下(路径边界匹配)
bool under_root(const QString& path, const QStringList& roots)
{
    for (const auto& root : roots) {
        if (path == root || (path.startsWith(root) && path.at(root.size()) == QLatin1Char('/'))) {
            return true;
        }
    }
    return false;
}
} // namespace

void LibraryScanner::start_scan(const QStringList& roots, const LibrarySnapshot& snapshot)
{
    const QStringList roots_norm = normalize_roots(roots);

    // ---- 阶段 1:收集文件列表(快速,不解析标签) ----
    QVector<fs::path> files;
    files.reserve(1024);
    for (const auto& root : roots_norm) {
        collect_audio_files(utils::audio::to_fs_path(root), files);
    }
    const int total = files.size();

    // ---- 阶段 2:分类与标签解析 ----
    QVector<TrackUpdate> batch;
    batch.reserve(512);
    constexpr int kBatchSize = 512;
    int processed            = 0;

    for (const auto& file : files) {
        const QString filepath = utils::audio::from_fs_path(file);
        const QString norm     = utils::path::normalize_path(filepath);
        const qint64 size      = fs_file_size(file);
        const qint64 mtime     = file_mtime_secs(file);

        const auto it          = snapshot.constFind(norm);
        if (it != snapshot.constEnd() && it->size == size && it->mtime == mtime) {
            ++processed; // 未变化,跳过标签解析
            if (processed % 100 == 0) {
                emit sgn_progress(processed, total);
            }
            continue;
        }

        TrackUpdate update;
        update.change  = (it == snapshot.constEnd()) ? TrackChange::added : TrackChange::modified;
        update.path    = norm;

        LibraryTrack t = LibraryTrack::from_path(norm);
        if (it != snapshot.constEnd()) {
            t.track_id = it->track_id; // 保留既有身份,避免修改后身份漂移
        }
        t.file_size        = size;
        t.mtime            = mtime;
        TrackMetaData meta = utils::audio::parse_to_local_meta(filepath);
        if (!meta.isValid) {
            meta.title = QFileInfo(filepath).fileName();
        }
        meta          = utils::audio::format(meta);
        t.duration_ms = meta.duration_s * 1000;
        t.meta        = meta;
        update.track  = t;

        batch.append(update);
        if (batch.size() >= kBatchSize) {
            emit sgn_batch_ready(batch);
            batch.clear();
        }
        ++processed;
        if (processed % 100 == 0) {
            emit sgn_progress(processed, total);
        }
    }

    // ---- 阶段 3:检测缺失文件(快照有、磁盘无、且在本次扫描根内) ----
    for (auto it = snapshot.constBegin(); it != snapshot.constEnd(); ++it) {
        const QString& path = it.key();
        if (!under_root(path, roots_norm)) {
            continue;
        }
        if (!QFileInfo::exists(path)) {
            TrackUpdate update;
            update.change = TrackChange::missing;
            update.path   = path;
            batch.append(update);
            if (batch.size() >= kBatchSize) {
                emit sgn_batch_ready(batch);
                batch.clear();
            }
        }
    }

    if (!batch.isEmpty()) {
        emit sgn_batch_ready(batch);
    }
    emit sgn_progress(processed, total);
    emit sgn_finished();
}
