#include "core/types.h"
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

/* ---- 单文件策略:keep_external 不注册;import 注册父目录 ---- */
static void test_single_file_policy()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }

    PlaylistManager pm;
    LibraryManager lib;
    CHECK(lib.initialize(data_dir.filePath("lib.db")));
    pm.set_library_manager(&lib);
    pm.create_playlist();
    auto playlists = pm.get_playlists();
    CHECK(playlists.size() == 1);
    if (playlists.isEmpty()) {
        return;
    }
    const PlaylistId pid = playlists.first()->id();

    // keep_external:未命中 → 外部条目,不注册父目录
    const QString f1     = music_dir.path() + "/external song.mp3";
    make_audio_file(music_dir.path(), "external song.mp3");
    pm.add_track(pid, f1, AddFilePolicy::keep_external);
    CHECK(lib.watched_folders().isEmpty());
    auto tracks = playlists.first()->get_tracks();
    CHECK(tracks.size() == 1);
    CHECK(tracks[0].source == TrackSource::external);
    CHECK(tracks[0].filepath == norm(f1));

    // import_to_library:未命中 → 注册父目录(库扫描完成后 upgrade 为库引用)
    const QString f2 = music_dir.path() + "/sub/imported.mp3";
    make_audio_file(music_dir.path() + "/sub", "imported.mp3");
    pm.add_track(pid, f2, AddFilePolicy::import_to_library);
    CHECK(lib.watched_folders().contains(norm(music_dir.path() + "/sub")));
    tracks = playlists.first()->get_tracks();
    CHECK(tracks.size() == 2);
    CHECK(tracks[1].source == TrackSource::external); // 扫描未完成 → 仍为外部条目

    // by_operation(全局默认 by_operation):单文件 → 仅外部,不注册
    const QString f3 = music_dir.path() + "/plain.mp3";
    make_audio_file(music_dir.path(), "plain.mp3");
    pm.add_track(pid, f3, AddFilePolicy::by_operation);
    CHECK(lib.watched_folders().size() == 1); // 只有 sub
    tracks = playlists.first()->get_tracks();
    CHECK(tracks.size() == 3);
    CHECK(tracks[2].source == TrackSource::external);
}

/* ---- 全局策略 + 文件夹策略 ---- */
static void test_global_policy_and_folder()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }

    PlaylistManager pm;
    LibraryManager lib;
    CHECK(lib.initialize(data_dir.filePath("lib.db")));
    pm.set_library_manager(&lib);
    pm.create_playlist();
    auto playlists = pm.get_playlists();
    CHECK(playlists.size() == 1);
    if (playlists.isEmpty()) {
        return;
    }
    const PlaylistId pid = playlists.first()->id();

    // 全局策略 import_to_library:单文件 by_operation 也注册父目录
    pm.set_add_file_policy(AddFilePolicy::import_to_library);
    CHECK(pm.add_file_policy() == AddFilePolicy::import_to_library);
    make_audio_file(music_dir.path(), "a.mp3");
    pm.add_track(pid, music_dir.path() + "/a.mp3", AddFilePolicy::by_operation);
    CHECK(lib.watched_folders().contains(norm(music_dir.path())));

    // add_folder keep_external:不注册目录,文件作为外部条目
    make_audio_file(music_dir.path() + "/dir2", "b.mp3");
    pm.add_folder(pid, music_dir.path() + "/dir2", AddFilePolicy::keep_external);
    CHECK(!lib.watched_folders().contains(norm(music_dir.path() + "/dir2")));
    auto tracks = playlists.first()->get_tracks();
    CHECK(tracks.size() == 2);
    CHECK(tracks[1].source == TrackSource::external);

    // add_folder import_to_library:注册目录
    make_audio_file(music_dir.path() + "/dir3", "c.mp3");
    pm.add_folder(pid, music_dir.path() + "/dir3", AddFilePolicy::import_to_library);
    CHECK(lib.watched_folders().contains(norm(music_dir.path() + "/dir3")));
    tracks = playlists.first()->get_tracks();
    CHECK(tracks.size() == 3);
}

/* ---- 库命中:无论策略如何都返回库引用 ---- */
static void test_library_hit_policy()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }
    make_audio_file(music_dir.path(), "in lib.mp3");

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
    CHECK(lib.track_count() == 1);

    PlaylistManager pm;
    pm.set_library_manager(&lib);
    pm.create_playlist();
    auto playlists = pm.get_playlists();
    CHECK(playlists.size() == 1);
    if (playlists.isEmpty()) {
        return;
    }
    const PlaylistId pid = playlists.first()->id();

    // keep_external 下命中库 → 仍为库引用
    pm.add_track(pid, music_dir.path() + "/in lib.mp3", AddFilePolicy::keep_external);
    auto tracks = playlists.first()->get_tracks();
    CHECK(tracks.size() == 1);
    CHECK(tracks[0].source == TrackSource::library);
    CHECK(!tracks[0].library_track_id.isNull());
    CHECK(tracks[0].filepath == norm(music_dir.path() + "/in lib.mp3"));
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_single_file_policy();
    test_global_policy_and_folder();
    test_library_hit_policy();

    std::printf("== tb_add_file_policy: %d checks, %d failures ==\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
