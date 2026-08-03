#include "core/utils/path.hpp"
#include "model/library/library_track.h"
#include "model/playlist/playlist.h"
#include "model/playlist/playlist_manager.h"
#include "model/playlist/playlist_repo.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>
#include <optional>

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

/* ---- Track 身份与路径规范化 ---- */
static void test_track_identity()
{
    Playlist pl("identity");
    Track t1 = pl.addTrack("relative/dir/foo.mp3"); // 相对路径 → 规范化
    CHECK(!t1.entry_id.isNull());
    CHECK(t1.filepath == utils::path::normalize_path("relative/dir/foo.mp3"));
    CHECK(t1.source == TrackSource::external);
    CHECK(t1.meta.filepath == t1.filepath); // 不变量:meta.filepath 与 filepath 一致
    CHECK(t1.meta.filename == "foo.mp3");
    CHECK(!t1.meta.isValid); // 未解析标签时无效

    Track t2 = pl.addTrack("/abs/path/bar.flac");
    CHECK(!t2.entry_id.isNull());
    CHECK(t2.entry_id != t1.entry_id); // 每首曲目独立身份
    CHECK(pl.track_count() == 2u);

    // from_entry:保留指定身份(反序列化路径)
    EntryId eid = EntryId::createUuid();
    Track t3    = Track::from_entry(eid, "/abs/path/baz.ogg");
    CHECK(t3.entry_id == eid);
    CHECK(t3.filepath == utils::path::normalize_path("/abs/path/baz.ogg"));
    CHECK(t3.source == TrackSource::external);

    // 库条目字段
    Track t4;
    t4.library_track_id = TrackId::createUuid();
    t4.source           = TrackSource::library;
    t4.missing          = true;
    pl.addTrackObject(t4);
    CHECK(pl.track_count() == 3u);
    CHECK(t4.entry_id.isNull() == false);
}

/* ---- 路径工具 ---- */
static void test_path_utils()
{
    // 相对路径 → 绝对路径
    const QString rel = "relative_dir/foo.mp3";
    CHECK(utils::path::normalize_path(rel) ==
          utils::path::normalize_path(QDir::current().filePath(rel)));

    // 不存在文件:canonical 回退绝对路径
    const QString abs = QDir::current().absoluteFilePath("definitely_not_exists_x.mp3");
    CHECK(utils::path::canonical_path(abs) == abs);

#ifdef Q_OS_WIN
    CHECK(utils::path::case_fold("AbC") == "abc");
#else
    CHECK(utils::path::case_fold("AbC") == "AbC");
#endif
}

/* ---- 查找与删除 ---- */
static void test_find_and_remove()
{
    Playlist pl("find");
    Track t            = pl.addTrack("/abs/path/a.mp3");

    const Track* found = pl.findTrackByID(t.entry_id);
    CHECK(found != nullptr);
    if (found) {
        CHECK(found->filepath == t.filepath);
    }
    CHECK(pl.findTrackByID(EntryId()) == nullptr);             // 空身份未命中
    CHECK(pl.findTrackByID(EntryId::createUuid()) == nullptr); // 随机身份未命中

    pl.removeTrack(t.entry_id);
    CHECK(pl.track_count() == 0u);
    CHECK(pl.findTrackByID(t.entry_id) == nullptr);
    CHECK(pl.isEmpty());
}

/* ---- 元数据更新 ---- */
static void test_update_meta()
{
    Playlist pl("meta");
    Track t = pl.addTrack("/abs/path/a.mp3");

    TrackMetaData meta;
    meta.title   = "Title";
    meta.artist  = "Artist";
    meta.isValid = true;
    CHECK(pl.updateTrackMeta(t.entry_id, meta));

    const Track* found = pl.findTrackByID(t.entry_id);
    CHECK(found != nullptr);
    if (found) {
        CHECK(found->meta.title == "Title");
        CHECK(found->meta.artist == "Artist");
        CHECK(found->meta.filepath == found->filepath); // 不变量
        CHECK(found->meta.filename == "a.mp3");
    }
    CHECK(!pl.updateTrackMeta(EntryId::createUuid(), meta)); // 未命中返回 false
}

/* ---- repo 序列化 round-trip(新格式) ---- */
static void test_repo_roundtrip()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    if (!dir.isValid()) {
        return;
    }

    PlaylistRepo repo;
    PlaylistId pid = repo.createList();
    CHECK(!pid.isNull());
    auto pl = repo.findPlaylistById(pid);
    CHECK(pl != nullptr);
    if (!pl) {
        return;
    }

    Track t1 = pl->addTrack("/music/album/01.mp3");
    TrackMetaData meta;
    meta.title   = "Song One";
    meta.artist  = "Artist";
    meta.album   = "Album";
    meta.isValid = true;
    pl->updateTrackMeta(t1.entry_id, meta);

    pl->addTrack("/music/album/02.flac");

    // 一个库条目
    Track lib;
    lib.library_track_id = TrackId::createUuid();
    lib.source           = TrackSource::library;
    lib.filepath         = "/music/album/03.ogg";
    lib.missing          = true;
    pl->addTrackObject(lib);

    const QString file = dir.filePath("roundtrip.wcpl");
    repo.saveList(pid, file);
    CHECK(QFile::exists(file));

    PlaylistRepo repo2;
    PlaylistId new_pid = repo2.loadList(file);
    CHECK(!new_pid.isNull());
    auto loaded = repo2.findPlaylistById(new_pid);
    CHECK(loaded != nullptr);
    if (!loaded) {
        return;
    }
    CHECK(loaded->track_count() == 3u);

    const QVector<Track>& tracks = loaded->getTracks();
    // 身份保持
    CHECK(tracks[0].entry_id == t1.entry_id);
    CHECK(tracks[0].filepath == utils::path::normalize_path("/music/album/01.mp3"));
    CHECK(tracks[0].source == TrackSource::external);
    CHECK(tracks[0].meta.isValid);
    CHECK(tracks[0].meta.title == "Song One");
    CHECK(tracks[0].meta.album == "Album");
    CHECK(tracks[0].meta.filepath == tracks[0].filepath);

    // 库条目 round-trip
    CHECK(tracks[2].library_track_id == lib.library_track_id);
    CHECK(tracks[2].source == TrackSource::library);
    CHECK(tracks[2].missing);
    CHECK(tracks[2].filepath == utils::path::normalize_path("/music/album/03.ogg"));
}

/* ---- 异步批处理加载(需事件循环) ---- */
static void test_repo_load_batched()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    if (!dir.isValid()) {
        return;
    }

    PlaylistRepo repo;
    PlaylistId pid = repo.createList();
    auto pl        = repo.findPlaylistById(pid);
    CHECK(pl != nullptr);
    if (!pl) {
        return;
    }
    pl->addTrack("/music/a.mp3");
    pl->addTrack("/music/b.flac");

    const QString file = dir.filePath("batch.wcpl");
    repo.saveList(pid, file);

    PlaylistRepo repo2;
    QEventLoop loop;
    bool finished = false;
    QObject::connect(&repo2, &PlaylistRepo::playlistLoadFinished, &loop, [&](const PlaylistId&) {
        finished = true;
        loop.quit();
    });

    PlaylistId new_pid = repo2.loadListBatched(file, 10);
    CHECK(!new_pid.isNull());

    QTimer::singleShot(3000, &loop, &QEventLoop::quit); // 超时保护
    loop.exec();

    CHECK(finished);
    auto loaded = repo2.findPlaylistById(new_pid);
    CHECK(loaded != nullptr);
    if (loaded) {
        CHECK(loaded->track_count() == 2u);
    }
}

/* ---- 旧格式缓存(缺 entry_id):回退复用旧 "id" 保持身份稳定 ---- */
static void test_legacy_format_degrades()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    if (!dir.isValid()) {
        return;
    }

    QJsonObject root;
    root["schemaVersion"] = 1;
    root["id"]            = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee";
    root["name"]          = "legacy";
    QJsonArray tracks;
    QJsonObject t;
    t["id"]       = "11111111-2222-3333-4444-555555555555";
    t["filepath"] = "/music/legacy.mp3";
    tracks.append(t);
    root["tracks"]     = tracks;

    const QString file = dir.filePath("legacy.wcpl");
    QFile f(file);
    CHECK(f.open(QIODevice::WriteOnly));
    if (f.isOpen()) {
        f.write(QJsonDocument(root).toJson());
        f.close();
    }

    PlaylistRepo repo;
    PlaylistId pid = repo.loadList(file);
    CHECK(!pid.isNull());
    auto pl = repo.findPlaylistById(pid);
    CHECK(pl != nullptr);
    if (!pl) {
        return;
    }
    CHECK(pl->track_count() == 1u);
    const QVector<Track>& tr = pl->getTracks();
    CHECK(!tr[0].entry_id.isNull());
    // 旧格式 "id" 语义即条目身份:复用而非重新分配,保证恢复播放等引用不失效
    CHECK(tr[0].entry_id == EntryId("11111111-2222-3333-4444-555555555555"));
    CHECK(tr[0].filepath == utils::path::normalize_path("/music/legacy.mp3"));
    CHECK(tr[0].source == TrackSource::external);
}

/* ---- 外部条目升级为库引用条目 ---- */
static void test_upgrade_external()
{
    Playlist pl("upgrade");
    Track ext = pl.addTrack("/lib/now_in_lib.mp3");
    CHECK(ext.source == TrackSource::external);

    const TrackId lib_id = TrackId::createUuid();
    const int upgraded =
        pl.upgradeExternalTracks([&](const QString& path) -> std::optional<LibraryTrack> {
            if (path == utils::path::normalize_path("/lib/now_in_lib.mp3")) {
                LibraryTrack lt;
                lt.track_id     = lib_id;
                lt.filepath     = path;
                lt.meta.title   = "In Library";
                lt.meta.isValid = true;
                return lt;
            }
            return std::nullopt;
        });
    CHECK(upgraded == 1);
    const Track* t = pl.findTrackByID(ext.entry_id);
    CHECK(t != nullptr);
    if (t) {
        CHECK(t->source == TrackSource::library);
        CHECK(t->library_track_id == lib_id);
        CHECK(t->meta.title == "In Library");
    }

    // 路径不在库中 → 不升级
    const int upgraded2 = pl.upgradeExternalTracks(
        [](const QString&) -> std::optional<LibraryTrack> { return std::nullopt; });
    CHECK(upgraded2 == 0);
    CHECK(pl.findTrackByID(ext.entry_id)->source == TrackSource::library); // 已升级,保持
}

/* ---- 库引用条目:刷新与缺失处理 ---- */
static void test_library_ref_and_missing()
{
    Playlist pl("ref");

    // 库引用条目
    Track lib_track;
    lib_track.source           = TrackSource::library;
    lib_track.library_track_id = TrackId::createUuid();
    lib_track.filepath         = "/lib/1.mp3";
    lib_track.meta.title       = "Old";
    lib_track.meta.isValid     = true;
    pl.addTrackObject(lib_track);

    // 外部条目(标记缺失)
    Track ext = pl.addTrack("/ext/missing.mp3");
    CHECK(pl.setTrackMissing(ext.entry_id, true));
    CHECK(pl.findTrackByID(ext.entry_id)->missing);

    // refreshLibraryTracks:仅刷新库引用条目;解析器返回 nullopt 表示库中已无
    int updated = pl.refreshLibraryTracks([&](const TrackId& id) -> std::optional<LibraryTrack> {
        if (id == lib_track.library_track_id) {
            LibraryTrack lt;
            lt.track_id     = id;
            lt.filepath     = "/lib/1.mp3";
            lt.missing      = true; // 库中标记缺失
            lt.meta.title   = "New";
            lt.meta.isValid = true;
            return lt;
        }
        return std::nullopt;
    });
    CHECK(updated == 1); // 外部条目不参与刷新
    CHECK(pl.track_count() == 2);
    const Track* t = pl.findTrackByID(lib_track.entry_id);
    CHECK(t != nullptr);
    if (t) {
        CHECK(t->meta.title == "New");
        CHECK(t->missing); // 库的缺失标记已同步
    }
    CHECK(pl.track_count() == 2);

    // 移除缺失条目(库引用条目 + 外部条目都被移除)
    int removed = pl.removeMissingTracks();
    CHECK(removed == 2);
    CHECK(pl.track_count() == 0);
    CHECK(pl.isEmpty());
}

/* ---- 删除最后一个播放列表(回归:曾因空指针解引用崩溃) ---- */
static void test_remove_last_playlist()
{
    PlaylistManager pm;
    pm.createPlaylist();
    auto playlists = pm.getPlaylists();
    CHECK(playlists.size() == 1);
    const PlaylistId pid = playlists.last()->id();
    pm.switchToPlaylist(pid);

    pm.removePlaylist(pid); // 删除最后一个列表,不应崩溃
    CHECK(pm.getPlaylists().isEmpty());
    CHECK(pm.getCurrentPlaylistId().isNull());
    CHECK(pm.getCurrentPlaylistName().isEmpty()); // 空列表名
    CHECK(pm.getPlaylistById(pid).isEmpty());     // 不存在的列表
    CHECK(pm.getCurrentTrack().isEmpty());
    CHECK(!pm.getCurrentMetadata().isValid); // 空元数据
}

/* ---- nextTrack/prevTrack 返回条目身份(阶段6:身份而非 filepath) ---- */
static void test_next_prev_track_id()
{
    PlaylistManager pm;
    pm.createPlaylist();
    auto pls = pm.getPlaylists();
    CHECK(pls.size() == 1);
    if (pls.isEmpty()) {
        return;
    }
    const PlaylistId pid = pls.first()->id();
    pm.switchToPlaylist(pid);
    pm.addTrack(pid, "/mnt/next/1.mp3");
    pm.addTrack(pid, "/mnt/next/2.mp3");
    pm.getViewModel()->rebuild(); // 同步构建播放队列

    const auto& tracks = pls.first()->getTracks();
    CHECK(tracks.size() == 2);
    pm.m_context->setPlayTrack(tracks[0].entry_id);

    // in_order:第一首 → 第二首
    const EntryId next = pm.nextTrack(PlayMode::in_order);
    CHECK(next == tracks[1].entry_id);

    // 第二首 → 第一首
    const EntryId prev = pm.prevTrack(PlayMode::in_order);
    CHECK(prev == tracks[0].entry_id);

    // 首部不回绕:返回空
    const EntryId none = pm.prevTrack(PlayMode::in_order);
    CHECK(none.isNull());

    // 当前项已被更新(m_context 副作用)
    CHECK(pm.m_context->getPlayTrackId() == tracks[0].entry_id);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_path_utils();
    test_track_identity();
    test_find_and_remove();
    test_update_meta();
    test_repo_roundtrip();
    test_repo_load_batched();
    test_legacy_format_degrades();
    test_library_ref_and_missing();
    test_upgrade_external();
    test_remove_last_playlist();
    test_next_prev_track_id();

    std::printf("== tb_playlist: %d checks, %d failures ==\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
