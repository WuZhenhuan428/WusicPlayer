#include "core/types.h"
#include "core/utils/path.hpp"
#include "model/library/library_manager.h"
#include "model/playback_queue/playback_queue.h"
#include "model/playback_queue/playback_queue_service.h"
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

QueueItem make_item(const QString& path, const QString& title = QString())
{
    QueueItem it;
    it.filepath      = norm(path);
    it.meta.filepath = it.filepath;
    it.meta.filename = QFileInfo(it.filepath).fileName();
    it.meta.title    = title;
    it.meta.isValid  = true;
    return it;
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

/* ---- 队列基本操作 ---- */
static void test_queue_basic()
{
    PlaybackQueue q;
    CHECK(q.is_empty());
    CHECK(q.size() == 0);
    CHECK(q.current_index() == -1);
    CHECK(!q.current().has_value());
    CHECK(!q.item_at(0).has_value());

    const int i0 = q.enqueue(make_item("/a/1.mp3", "one"));
    const int i1 = q.enqueue(make_item("/a/2.mp3", "two"));
    const int i2 = q.enqueue(make_item("/a/3.mp3", "three"));
    CHECK(i0 == 0 && i1 == 1 && i2 == 2);
    CHECK(q.size() == 3);
    CHECK(!q.is_empty());
    CHECK(q.item_at(1)->meta.title == "two");
    CHECK(!q.item_at(5).has_value());
    CHECK(q.items().size() == 3);
}

/* ---- 当前项 ---- */
static void test_current()
{
    PlaybackQueue q;
    q.enqueue(make_item("/a/1.mp3"));
    q.enqueue(make_item("/a/2.mp3"));
    q.enqueue(make_item("/a/3.mp3"));

    CHECK(!q.set_current(-1)); // 越界
    CHECK(!q.set_current(9));
    CHECK(q.current_index() == -1);

    CHECK(q.set_current(1));
    CHECK(q.current_index() == 1);
    CHECK(q.current()->filepath == norm("/a/2.mp3"));
    CHECK(!q.set_current(1)); // 未变化返回 false

    q.clear_current();
    CHECK(q.current_index() == -1);
    CHECK(!q.current().has_value());
}

/* ---- enqueue_next ---- */
static void test_enqueue_next()
{
    PlaybackQueue q;
    q.enqueue(make_item("/a/1.mp3"));
    q.enqueue(make_item("/a/3.mp3"));
    // 无当前项 → 插入头部
    CHECK(q.enqueue_next(make_item("/a/x.mp3")) == 0);
    CHECK(q.item_at(0)->filepath == norm("/a/x.mp3"));

    CHECK(q.set_current(2));
    const int ni = q.enqueue_next(make_item("/a/2.mp3"));
    CHECK(ni == 3); // 当前项(下标 2)之后
    CHECK(q.size() == 4);
    CHECK(q.item_at(3)->filepath == norm("/a/2.mp3"));
    CHECK(q.current_index() == 2); // 当前项不变
}

/* ---- remove / move / clear ---- */
static void test_remove_move_clear()
{
    PlaybackQueue q;
    q.enqueue(make_item("/a/1.mp3"));
    q.enqueue(make_item("/a/2.mp3"));
    q.enqueue(make_item("/a/3.mp3"));
    q.enqueue(make_item("/a/4.mp3"));

    // 移除当前项之后的项 → 当前下标不变
    q.set_current(2); // "3.mp3"
    q.remove_at(3);
    CHECK(q.size() == 3);
    CHECK(q.current_index() == 2);

    // 移除当前项之前的项 → 当前下标前移
    q.remove_at(0);
    CHECK(q.current_index() == 1);
    CHECK(q.item_at(1)->filepath == norm("/a/3.mp3"));

    // 移除当前项 → 清空当前
    q.remove_at(1);
    CHECK(q.current_index() == -1);

    // 越界忽略
    q.remove_at(-1);
    q.remove_at(99);
    CHECK(q.size() == 1);

    q.clear();
    CHECK(q.is_empty());
    CHECK(q.current_index() == -1);

    // move:当前项本身移动
    PlaybackQueue m;
    m.enqueue(make_item("/a/1.mp3"));
    m.enqueue(make_item("/a/2.mp3"));
    m.enqueue(make_item("/a/3.mp3"));
    m.set_current(1); // "2.mp3"
    m.move(0, 2);     // [2,3,1];当前项从 1 移到 0
    CHECK(m.item_at(0)->filepath == norm("/a/2.mp3"));
    CHECK(m.item_at(2)->filepath == norm("/a/1.mp3"));
    CHECK(m.current_index() == 0);

    m.move(2, 0); // [1,2,3];项从 2 移到 0,当前(0)整体后移 → 1
    CHECK(m.item_at(0)->filepath == norm("/a/1.mp3"));
    CHECK(m.item_at(1)->filepath == norm("/a/2.mp3"));
    CHECK(m.current_index() == 1);

    // move:前面的项移到当前项之后 → 当前下标前移
    m.set_current(1); // "2.mp3"
    m.move(0, 2);     // [2,3,1],当前(原1)前移为 0
    CHECK(m.current_index() == 0);

    // 无效移动忽略
    m.move(0, 0);
    m.move(-1, 2);
    m.move(0, 99);
    CHECK(m.size() == 3);
}

/* ---- 导航:in_order 不自动回绕 ---- */
static void test_nav_in_order()
{
    PlaybackQueue q;
    q.enqueue(make_item("/a/1.mp3"));
    q.enqueue(make_item("/a/2.mp3"));
    q.enqueue(make_item("/a/3.mp3"));

    // 无当前项:next→0,prev→末尾
    auto n0 = q.next(PlayMode::in_order);
    CHECK(n0.has_value());
    CHECK(n0->filepath == norm("/a/1.mp3"));
    CHECK(q.current_index() == 0);

    auto n1 = q.next(PlayMode::in_order);
    CHECK(n1->filepath == norm("/a/2.mp3"));
    auto n2 = q.next(PlayMode::in_order);
    CHECK(n2->filepath == norm("/a/3.mp3"));
    // 末尾:不回绕
    CHECK(!q.next(PlayMode::in_order).has_value());
    CHECK(q.current_index() == 2);

    auto p1 = q.prev(PlayMode::in_order);
    CHECK(p1->filepath == norm("/a/2.mp3"));
    auto p0 = q.prev(PlayMode::in_order);
    CHECK(p0->filepath == norm("/a/1.mp3"));
    // 首:不回绕
    CHECK(!q.prev(PlayMode::in_order).has_value());
    CHECK(q.current_index() == 0);
}

/* ---- 导航:loop 回绕 ---- */
static void test_nav_loop()
{
    PlaybackQueue q;
    q.enqueue(make_item("/a/1.mp3"));
    q.enqueue(make_item("/a/2.mp3"));
    q.set_current(1);
    auto n = q.next(PlayMode::loop);
    CHECK(n.has_value());
    CHECK(n->filepath == norm("/a/1.mp3")); // 回绕到首
    CHECK(q.current_index() == 0);
    auto p = q.prev(PlayMode::loop);
    CHECK(p.has_value());
    CHECK(p->filepath == norm("/a/2.mp3")); // 回绕到尾
    CHECK(q.current_index() == 1);
}

/* ---- 导航:shuffle 范围内 ---- */
static void test_nav_shuffle()
{
    PlaybackQueue q;
    q.enqueue(make_item("/a/1.mp3"));
    q.enqueue(make_item("/a/2.mp3"));
    q.enqueue(make_item("/a/3.mp3"));
    for (int i = 0; i < 20; ++i) {
        auto n = q.next(PlayMode::shuffle);
        CHECK(n.has_value());
        CHECK(n->filepath.startsWith("/a/"));
        CHECK(q.current_index() >= 0 && q.current_index() < 3);
        // out_of_order_* 同样走随机
        auto g = q.prev(PlayMode::out_of_order_track);
        CHECK(g.has_value());
        CHECK(q.current_index() >= 0 && q.current_index() < 3);
    }
}

/* ---- 空队列导航 ---- */
static void test_nav_empty()
{
    PlaybackQueue q;
    CHECK(!q.next(PlayMode::in_order).has_value());
    CHECK(!q.next(PlayMode::loop).has_value());
    CHECK(!q.next(PlayMode::shuffle).has_value());
    CHECK(!q.prev(PlayMode::in_order).has_value());
}

/* ---- 信号 ---- */
static void test_signals()
{
    PlaybackQueue q;
    int queue_changes   = 0;
    int current_changes = 0;
    int last_current    = -99;
    QObject::connect(&q, &PlaybackQueue::sgn_queue_changed, [&]() { ++queue_changes; });
    QObject::connect(&q, &PlaybackQueue::sgn_current_changed, [&](int idx) {
        ++current_changes;
        last_current = idx;
    });

    q.enqueue(make_item("/a/1.mp3"));
    q.enqueue(make_item("/a/2.mp3"));
    CHECK(queue_changes == 2);
    q.enqueue_many({make_item("/a/3.mp3"), make_item("/a/4.mp3")});
    CHECK(queue_changes == 3); // 批量只发一次
    q.set_current(1);
    CHECK(current_changes == 1 && last_current == 1);
    q.next(PlayMode::in_order);
    CHECK(current_changes == 2 && last_current == 2);
    q.clear();
    CHECK(queue_changes == 4);
    CHECK(current_changes == 3 && last_current == -1); // 清空带当前项 → 发 -1
}

/* ---- 持久化 round-trip ---- */
static void test_persistence()
{
    QTemporaryDir dir;
    CHECK(dir.isValid());
    if (!dir.isValid()) {
        return;
    }
    const QString path = dir.filePath("queue.json");

    PlaybackQueueService s1;
    QueueItem it;
    it.library_track_id   = TrackId::createUuid();
    it.playlist_entry_id  = EntryId::createUuid();
    it.source_playlist_id = PlaylistId::createUuid();
    it.source_label       = "列表:测试";
    it.filepath           = norm("/mnt/music/one.mp3");
    it.meta.title         = "One";
    it.meta.artist        = "Artist";
    it.meta.duration_s    = 123;
    it.meta.isValid       = true;
    s1.queue()->enqueue(it);
    QueueItem it2;
    it2.filepath      = norm("/mnt/music/two.flac");
    it2.meta.filename = "two.flac";
    it2.meta.isValid  = true;
    s1.queue()->enqueue(it2);
    CHECK(s1.queue()->set_current(1));
    CHECK(s1.save_to(path));

    PlaybackQueueService s2;
    CHECK(s2.load_from(path));
    CHECK(s2.queue()->size() == 2);
    CHECK(s2.queue()->current_index() == 1);
    auto loaded = s2.queue()->item_at(0);
    CHECK(loaded.has_value());
    CHECK(loaded->library_track_id == it.library_track_id);
    CHECK(loaded->playlist_entry_id == it.playlist_entry_id);
    CHECK(loaded->source_playlist_id == it.source_playlist_id);
    CHECK(loaded->source_label == "列表:测试");
    CHECK(loaded->filepath == it.filepath);
    CHECK(loaded->meta.title == "One");
    CHECK(loaded->meta.artist == "Artist");
    CHECK(loaded->meta.duration_s == 123);
    auto loaded2 = s2.queue()->item_at(1);
    CHECK(loaded2.has_value());
    CHECK(loaded2->meta.filename == "two.flac");

    // 不存在文件 → 加载失败
    CHECK(!s2.load_from(dir.filePath("nope.json")));
}

/* ---- 服务:播放(入队 + 设当前 + 发信号) ---- */
static void test_service_play()
{
    PlaybackQueueService s;
    int play_signals = 0;
    QString played_path;
    QObject::connect(&s, &PlaybackQueueService::sgn_play_requested, [&](const QueueItem& item) {
        ++play_signals;
        played_path = item.filepath;
    });

    TrackMetaData meta;
    meta.title    = "T";
    meta.isValid  = true;
    const int idx = s.play_external("rel/foo.mp3", meta);
    CHECK(idx == 0);
    CHECK(play_signals == 1); // 播放触发一次信号
    CHECK(s.queue()->current_index() == 0);
    CHECK(played_path == norm("rel/foo.mp3"));
    CHECK(s.queue()->current()->meta.title == "T");

    // 库曲目:未注入 LibraryManager → 失败且不发信号
    const int before = play_signals;
    CHECK(!s.play_library_track(TrackId::createUuid()));
    CHECK(play_signals == before);
    CHECK(s.queue()->size() == 1); // 未入队
}

/* ---- 服务:外部文件 ---- */
static void test_service_external()
{
    PlaybackQueueService s;
    const int i = s.enqueue_external("relative_dir/foo.mp3");
    CHECK(i == 0);
    auto it = s.queue()->item_at(0);
    CHECK(it.has_value());
    CHECK(it->filepath == norm("relative_dir/foo.mp3"));
    CHECK(it->is_external());
    CHECK(it->meta.filename == "foo.mp3");

    // 提供合法 meta 时保留字段,缺失时回填路径/文件名
    TrackMetaData meta;
    meta.title   = "T";
    meta.isValid = true;
    s.enqueue_external("/abs/x.flac", meta);
    auto it2 = s.queue()->item_at(1);
    CHECK(it2->meta.title == "T");
    CHECK(it2->meta.filepath == norm("/abs/x.flac"));
    CHECK(it2->meta.filename == "x.flac");
}

/* ---- 服务:播放列表条目解析 ---- */
static void test_service_playlist_entry()
{
    PlaylistManager pm;
    pm.create_playlist();
    auto pls = pm.get_playlists();
    CHECK(!pls.isEmpty());
    if (pls.isEmpty()) {
        return;
    }
    const PlaylistId pid = pls.first()->id();
    pm.add_track(pid, "/mnt/music/playlist_song.mp3");
    const auto& tracks = pls.first()->get_tracks();
    CHECK(!tracks.isEmpty());
    if (tracks.isEmpty()) {
        return;
    }
    const EntryId eid = tracks.last().entry_id;

    PlaybackQueueService s;
    s.set_playlist_manager(&pm);
    CHECK(s.enqueue_playlist_entry(pid, eid));
    auto it = s.queue()->item_at(0);
    CHECK(it.has_value());
    CHECK(it->is_playlist());
    CHECK(it->playlist_entry_id == eid);
    CHECK(it->source_playlist_id == pid);
    CHECK(it->filepath == norm("/mnt/music/playlist_song.mp3"));

    // 未知条目 → 失败
    CHECK(!s.enqueue_playlist_entry(pid, EntryId::createUuid()));
    // 未注入管理器 → 失败
    PlaybackQueueService s2;
    CHECK(!s2.enqueue_playlist_entry(pid, eid));
}

/* ---- 服务:媒体库曲目解析(扫描后) ---- */
static void test_service_library_track()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_dir;
    CHECK(data_dir.isValid());
    CHECK(music_dir.isValid());
    if (!data_dir.isValid() || !music_dir.isValid()) {
        return;
    }
    make_audio_file(music_dir.path(), "lib_song.mp3");

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

    const auto& idx = lib.index();
    CHECK(idx.size() == 1);
    const TrackId tid = idx.constBegin()->track_id;

    PlaybackQueueService s;
    s.set_library_manager(&lib);
    CHECK(s.enqueue_library_track(tid));
    auto it = s.queue()->item_at(0);
    CHECK(it.has_value());
    CHECK(it->is_library());
    CHECK(it->library_track_id == tid);
    CHECK(it->filepath == norm(music_dir.path() + "/lib_song.mp3"));

    // 未知曲目 / 未注入 → 失败
    CHECK(!s.enqueue_library_track(TrackId::createUuid()));
    PlaybackQueueService s2;
    CHECK(!s2.enqueue_library_track(tid));
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_queue_basic();
    test_current();
    test_enqueue_next();
    test_remove_move_clear();
    test_nav_in_order();
    test_nav_loop();
    test_nav_shuffle();
    test_nav_empty();
    test_signals();
    test_persistence();
    test_service_play();
    test_service_external();
    test_service_playlist_entry();
    test_service_library_track();

    std::printf("== tb_playback_queue: %d checks, %d failures ==\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
