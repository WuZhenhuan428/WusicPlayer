#include "controller/playlist_controller.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "core/search_types.h"
#include "core/utils/path.hpp"
#include "model/library/library_manager.h"
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

void make_audio_file(const QString& dir, const QString& name)
{
    QDir().mkpath(dir);
    QFile f(dir + QLatin1Char('/') + name);
    if (f.open(QIODevice::WriteOnly)) {
        f.write("dummy audio data");
        f.close();
    }
}
} // namespace

/* ---- 搜索当前播放列表(库引用条目 + 外部条目) ---- */
static void test_search_current_playlist(QCoreApplication& app)
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }
    make_audio_file(music_dir.path(), "song one.mp3");
    make_audio_file(music_dir.path(), "other.mp3");

    // 库:扫描两个文件(假文件 → 库 meta.title 回退文件名)
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

    // 播放列表:两首库引用条目 + 一首外部条目(内联 meta)
    PlaylistManager pm;
    pm.set_library_manager(&lib);
    pm.create_playlist();
    auto playlists = pm.get_playlists();
    CHECK(playlists.size() == 1);
    if (playlists.isEmpty()) {
        return;
    }
    const PlaylistId pid = playlists.first()->id();
    pm.add_track(pid, music_dir.path() + "/song one.mp3");
    pm.add_track(pid, music_dir.path() + "/other.mp3");

    Track ext        = Track::from_filepath("/mnt/music/ext song.mp3");
    ext.meta.title   = "Ext Song";
    ext.meta.artist  = "Artist";
    ext.meta.isValid = true;
    pm.m_repo->find_playlist_by_id(pid)->add_track_object(ext);

    PlaylistController pc(&pm, nullptr, &app);
    pc.switch_to_playlist(pid); // 设置当前列表(resolve_pid 无显式 pid 时使用)
    InMemorySearchBackend backend(&pc);
    backend.warmup(pid);

    // Plain:库引用条目命中(meta.title = 文件名)
    SearchQuery q;
    q.keyword = "one";
    q.mode    = SearchQueryMode::Plain;
    auto hits = backend.search(q);
    CHECK(hits.size() == 1);
    if (hits.size() == 1) {
        CHECK(hits[0].title == "song one.mp3");
        CHECK(hits[0].filepath == norm(music_dir.path() + "/song one.mp3"));
        CHECK(!hits[0].track_id.is_null()); // 库引用 → 库级身份
    }

    // 外部条目:filepath 播放依据 + track_id 为空
    q.keyword = "ext";
    hits      = backend.search(q);
    CHECK(hits.size() == 1);
    if (hits.size() == 1) {
        CHECK(hits[0].filepath == norm("/mnt/music/ext song.mp3"));
        CHECK(hits[0].track_id.is_null());
    }

    // 无命中 / 空关键字
    q.keyword = "zzz_no_such";
    CHECK(backend.search(q).isEmpty());
    q.keyword = "";
    CHECK(backend.search(q).isEmpty());

    // invalidate 后按需重建索引
    backend.invalidate(pid);
    q.keyword = "other";
    hits      = backend.search(q);
    CHECK(hits.size() == 1);
    if (hits.size() == 1) {
        CHECK(hits[0].filepath == norm(music_dir.path() + "/other.mp3"));
    }
    // 阶段6:条目身份 → 播放路径解析(播放经身份而非 filepath)
    const auto& pl_tracks = pm.m_repo->find_playlist_by_id(pid)->get_tracks();
    CHECK(!pl_tracks.isEmpty());
    CHECK(pc.track_file_path(pl_tracks[0].entry_id) == norm(music_dir.path() + "/song one.mp3"));
    CHECK(pc.track_file_path(EntryId::create_uuid()).isEmpty());
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_search_current_playlist(app);

    std::printf("== tb_search_backend: %d checks, %d failures ==\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
