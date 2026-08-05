#include "controller/search_backend/library_search_backend.h"
#include "core/search_types.h"
#include "core/utils/path.hpp"
#include "model/library/library.h"
#include "model/library/library_manager.h"
#include "model/library/library_repo.h"
#include "model/library/library_scanner.h"
#include "model/library/library_track.h"
#include "model/playlist/playlist_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>

static int g_checks   = 0;
static int g_failures = 0;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        ++g_checks;                                                                                \
        if (!(cond)) {                                                                             \
            ++g_failures;                                                                          \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                            \
        }                                                                                          \
    } while (0)

namespace
{
QString norm(const QString& p)
{
    return utils::path::normalize_path(p);
}

// 在临时目录中创建假音频文件(仅扩展名满足 is_audio_file)
QString make_audio_file(const QString& dir, const QString& name)
{
    const QString path = dir + QLatin1Char('/') + name;
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write("dummy audio data");
        f.close();
    }
    return path;
}
} // namespace

/* ---- Library 内存索引 ---- */
static void test_library_index()
{
    Library lib;
    LibraryTrack t1 = LibraryTrack::from_path(norm("/a/1.mp3"));
    LibraryTrack t2 = LibraryTrack::from_path(norm("/a/2.mp3"));
    lib.upsert(t1);
    lib.upsert(t2);
    CHECK(lib.track_count() == 2);
    CHECK(lib.track_by_path(norm("/a/1.mp3")).has_value());
    CHECK(lib.track_by_id(t2.track_id).has_value());
    CHECK(lib.track_by_id(TrackId::createUuid()) == std::nullopt);
    CHECK(lib.track_by_path(norm("/a/3.mp3")) == std::nullopt);

    // upsert 同一路径:身份保持,元数据更新
    t1.meta.title = "Updated";
    lib.upsert(t1);
    CHECK(lib.track_count() == 2);
    CHECK(lib.track_by_path(norm("/a/1.mp3"))->meta.title == "Updated");
    CHECK(lib.track_by_id(t1.track_id)->meta.title == "Updated");

    // 缺失标记
    CHECK(lib.mark_missing(norm("/a/1.mp3"), true));
    CHECK(lib.track_by_path(norm("/a/1.mp3"))->missing);
    CHECK(!lib.mark_missing(norm("/a/9.mp3"), true));

    // 移除
    lib.remove_by_path(norm("/a/2.mp3"));
    CHECK(lib.track_count() == 1);
    CHECK(lib.track_by_id(t2.track_id) == std::nullopt);
}

/* ---- LibraryRepo SQLite 持久化 ---- */
static void test_repo_roundtrip()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    if (!dir.isValid()) {
        return;
    }

    LibraryRepo repo;
    CHECK(repo.open(dir.filePath("test.db")));
    CHECK(repo.is_open());

    LibraryTrack t;
    t.track_id     = TrackId::createUuid();
    t.filepath     = norm("/music/01.mp3");
    t.file_size    = 1024;
    t.mtime        = 12345;
    t.duration_ms  = 180000;
    t.meta.title   = "Song One";
    t.meta.artist  = "Artist";
    t.meta.album   = "Album";
    t.meta.isValid = true;
    CHECK(repo.upsert_track(t));

    CHECK(repo.track_count() == 1);
    auto loaded = repo.load_all_tracks();
    CHECK(loaded.size() == 1);
    CHECK(loaded[0].track_id == t.track_id);
    CHECK(loaded[0].filepath == t.filepath);
    CHECK(loaded[0].file_size == 1024);
    CHECK(loaded[0].mtime == 12345);
    CHECK(loaded[0].duration_ms == 180000);
    CHECK(loaded[0].meta.duration_s == 180);
    CHECK(loaded[0].meta.title == "Song One");
    CHECK(loaded[0].meta.album == "Album");
    CHECK(loaded[0].meta.isValid);

    // watched folders(幂等)
    CHECK(repo.add_watched_folder("/music"));
    CHECK(repo.add_watched_folder("/music"));
    CHECK(repo.watched_folders() == QStringList{"/music"});
    CHECK(repo.remove_watched_folder("/music"));
    CHECK(repo.watched_folders().isEmpty());

    // 缺失标记
    CHECK(repo.mark_missing(t.filepath, true));
    loaded = repo.load_all_tracks();
    CHECK(loaded[0].missing);

    // FTS5 搜索(可用时校验命中;不可用时软跳过)
    auto hits = repo.search("Song", SearchQueryMode::Plain);
    if (!hits.isEmpty()) {
        CHECK(hits[0].track_id == t.track_id);
    }

    // 移除
    CHECK(repo.remove_track(t.filepath));
    CHECK(repo.track_count() == 0);
    repo.close();
    CHECK(!repo.is_open());
}

/* ---- LibraryScanner 增量扫描(同步调用) ---- */
static void test_scanner_incremental()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    if (!dir.isValid()) {
        return;
    }
    const QString f1  = make_audio_file(dir.path(), "song1.mp3");
    const QString f2  = make_audio_file(dir.path(), "song2.flac");
    const QString sub = dir.path() + "/sub";
    QDir().mkpath(sub);
    const QString f3 = make_audio_file(sub, "nested.ogg");

    LibraryScanner scanner;
    QVector<TrackUpdate> updates;
    bool finished = false;
    QObject::connect(&scanner, &LibraryScanner::sgn_batch_ready, &scanner,
                     [&](const QVector<TrackUpdate>& batch) {
                         for (const auto& u : batch) {
                             updates.append(u);
                         }
                     });
    QObject::connect(&scanner, &LibraryScanner::sgn_finished, &scanner, [&]() { finished = true; });

    // 首次扫描:全部 added(含嵌套目录)
    scanner.start_scan({dir.path()}, LibrarySnapshot{});
    CHECK(finished);
    CHECK(updates.size() == 3);
    for (const auto& u : updates) {
        CHECK(u.change == TrackChange::added);
    }
    CHECK(!updates[0].track.track_id.isNull());
    CHECK(!updates[0].track.meta.title.isEmpty()); // 假文件标签无效 → 回退文件名

    // 二次扫描(快照一致):全部 unchanged,无更新
    LibrarySnapshot snap;
    for (const auto& u : updates) {
        snap.insert(u.path, FileStamp{u.track.file_size, u.track.mtime, u.track.track_id});
    }
    updates.clear();
    finished = false;
    scanner.start_scan({dir.path()}, snap);
    CHECK(finished);
    CHECK(updates.isEmpty());

    // 删除一个文件:检测 missing
    QFile::remove(f1);
    updates.clear();
    finished = false;
    scanner.start_scan({dir.path()}, snap);
    CHECK(finished);
    CHECK(updates.size() == 1);
    CHECK(updates[0].change == TrackChange::missing);
    CHECK(updates[0].path == norm(f1));
}

/* ---- LibraryManager 门面(异步,需事件循环) ---- */
static void test_manager_scan()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }
    make_audio_file(music_dir.path(), "m1.mp3");
    make_audio_file(music_dir.path(), "m2.flac");

    LibraryManager mgr;
    QEventLoop loop;
    bool finished = false;
    QObject::connect(&mgr, &LibraryManager::sgn_scan_finished, &loop, [&]() {
        finished = true;
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit); // 超时保护

    CHECK(mgr.initialize(data_dir.filePath("lib.db")));
    mgr.add_watched_folder(music_dir.path()); // 触发异步扫描
    loop.exec();

    CHECK(finished);
    CHECK(mgr.track_count() == 2);
    const QString p = norm(music_dir.path() + "/m1.mp3");
    auto found      = mgr.track_by_path(p);
    CHECK(found.has_value());
    CHECK(mgr.watched_folders() == QStringList{norm(music_dir.path())});

    // 重新初始化:从数据库恢复
    LibraryManager mgr2;
    CHECK(mgr2.initialize(data_dir.filePath("lib.db")));
    CHECK(mgr2.track_count() == 2);
    CHECK(mgr2.track_by_path(p).has_value());
}

/* ---- 播放列表-音乐库集成:add_track 通过库解析 ---- */
static void test_playlist_library_resolution()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }
    make_audio_file(music_dir.path(), "r1.mp3");
    make_audio_file(music_dir.path(), "r2.flac");

    LibraryManager lib;
    QEventLoop loop;
    bool scanned = false;
    QObject::connect(&lib, &LibraryManager::sgn_scan_finished, &loop, [&]() {
        scanned = true;
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit); // 超时保护

    CHECK(lib.initialize(data_dir.filePath("lib.db")));
    lib.add_watched_folder(music_dir.path());
    loop.exec();
    CHECK(scanned);
    CHECK(lib.track_count() == 2);

    PlaylistManager pm;
    pm.set_library_manager(&lib);
    pm.create_playlist();
    auto playlists = pm.get_playlists();
    CHECK(playlists.size() == 1);
    const PlaylistId pid = playlists.last()->id();

    // 库内文件 → 库引用条目(元数据走库缓存)
    const QString f1     = music_dir.path() + "/r1.mp3";
    pm.add_track(pid, f1);
    auto tracks = playlists.last()->get_tracks();
    CHECK(tracks.size() == 1);
    CHECK(tracks[0].source == TrackSource::library);
    CHECK(!tracks[0].library_track_id.isNull());
    CHECK(tracks[0].filepath == norm(f1));
    CHECK(tracks[0].meta.title == "r1.mp3"); // 假文件回退文件名

    // 库外文件 → 外部条目(不强制入库)
    const QString f3 = make_audio_file(music_dir.path(), "external.mp3");
    pm.add_track(pid, f3);
    tracks = playlists.last()->get_tracks();
    CHECK(tracks.size() == 2);
    CHECK(tracks[1].source == TrackSource::external);
    CHECK(tracks[1].library_track_id.isNull());
}

/* ---- FTS5 搜索后端 ---- */
static void test_search_backend()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }
    make_audio_file(music_dir.path(), "track song.mp3");
    make_audio_file(music_dir.path(), "other.flac");

    LibraryManager lib;
    QEventLoop loop;
    bool scanned = false;
    QObject::connect(&lib, &LibraryManager::sgn_scan_finished, &loop, [&]() {
        scanned = true;
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit); // 超时保护
    CHECK(lib.initialize(data_dir.filePath("lib.db")));
    lib.add_watched_folder(music_dir.path());
    loop.exec();
    CHECK(scanned);
    CHECK(lib.track_count() == 2);

    LibrarySearchBackend backend(&lib);
    SearchQuery query;
    query.keyword = "song";
    query.mode    = SearchQueryMode::Plain;
    auto hints    = backend.search(query);
    CHECK(hints.size() == 1);
    if (hints.size() == 1) {
        CHECK(hints[0].title == "track song.mp3");
        CHECK(!hints[0].track_id.isNull());
    }

    // 前缀模式
    query.mode    = SearchQueryMode::Prefix;
    query.keyword = "tra";
    hints         = backend.search(query);
    CHECK(hints.size() == 1);

    // 子串(前缀命中不了,FTS 兜底 LIKE %sub%)→ 仍应命中
    query.mode    = SearchQueryMode::Prefix;
    query.keyword = "ong";
    hints         = backend.search(query);
    CHECK(hints.size() == 1);
    if (hints.size() == 1) {
        CHECK(hints[0].title == "track song.mp3");
    }

    // 多 token:每个 token 前缀 AND
    query.mode    = SearchQueryMode::Prefix;
    query.keyword = "tra so";
    hints         = backend.search(query);
    CHECK(hints.size() == 1);
    if (hints.size() == 1) {
        CHECK(hints[0].title == "track song.mp3");
    }

    // 空关键字 → 无结果
    query.mode    = SearchQueryMode::Plain;
    query.keyword = "   ";
    hints         = backend.search(query);
    CHECK(hints.isEmpty());
}

/* ---- add_folder 触发库扫描(搜索依赖库被填充) ---- */
static void test_add_folder_populates_library()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }
    make_audio_file(music_dir.path(), "f1.mp3");
    make_audio_file(music_dir.path(), "f2.flac");

    LibraryManager lib;
    QEventLoop loop;
    bool lib_scanned = false;
    QObject::connect(&lib, &LibraryManager::sgn_scan_finished, &loop, [&]() {
        lib_scanned = true;
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit); // 超时保护
    CHECK(lib.initialize(data_dir.filePath("lib.db")));

    PlaylistManager pm;
    pm.set_library_manager(&lib);
    pm.create_playlist();
    auto playlists = pm.get_playlists();
    CHECK(playlists.size() == 1);
    const PlaylistId pid = playlists.last()->id();

    // add_folder 应同时把目录加入库监控并触发扫描
    pm.add_folder(pid, music_dir.path());
    loop.exec();
    CHECK(lib_scanned);
    CHECK(lib.track_count() == 2); // 库被填充 → 搜索可用

    // 库扫描完成后,播放列表外部条目升级为库引用
    const auto& tracks = playlists.last()->get_tracks();
    CHECK(tracks.size() == 2);
    for (const auto& t : tracks) {
        CHECK(t.source == TrackSource::library);
        CHECK(!t.library_track_id.isNull());
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_library_index();
    test_repo_roundtrip();
    test_scanner_incremental();
    test_manager_scan();
    test_playlist_library_resolution();
    test_search_backend();
    test_add_folder_populates_library();

    std::printf("== tb_library: %d checks, %d failures ==\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
