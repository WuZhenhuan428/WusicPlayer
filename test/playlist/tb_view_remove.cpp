#include "model/playlist/playlist.h"
#include "model/playlist/playlist_repo.h"
#include "model/playlist/playlist_view_model.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>

#include <cstdio>
#include <functional>

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

/// 轮询事件循环直到条件成立(视图重建走异步线程)
static void pump_until(const std::function<bool()>& cond, int timeout_ms = 5000)
{
    QElapsedTimer timer;
    timer.start();
    while (!cond() && timer.elapsed() < timeout_ms) {
        QCoreApplication::processEvents();
        QThread::msleep(2);
    }
}

/// 行级移除(清理缺失/删除单曲)应: 不发 modelReset、发 rowsRemoved、
/// 同步播放队列、空组自动删除 —— 保证视图不跳顶。
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv); // 可执行名隔离 cache 目录

    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                              QStringLiteral("/playlists");
    QDir(cacheRoot).removeRecursively();

    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString pathA = dir.filePath(QStringLiteral("a.mp3"));
    const QString pathB = dir.filePath(QStringLiteral("b.mp3"));
    const QString pathC = dir.filePath(QStringLiteral("c.mp3"));
    for (const QString& p : {pathA, pathB, pathC}) {
        QFile f(p);
        CHECK(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();
    }

    PlaylistRepo repo;
    const PlaylistId pid = repo.create_list();
    CHECK(!pid.is_null());
    auto pl = repo.find_playlist_by_id(pid);
    CHECK(pl != nullptr);

    // a(G1) / b(G2) / c(G1)
    // 注意: meta.isValid=false 时 layout 会重新解析真实文件(dummy 文件无标签),
    // 因此内联元数据必须标记为有效才会被使用
    Track ta        = Track::from_filepath(pathA);
    ta.meta.album   = QStringLiteral("G1");
    ta.meta.title   = QStringLiteral("A");
    ta.meta.isValid = true;
    Track tb        = Track::from_filepath(pathB);
    tb.meta.album   = QStringLiteral("G2");
    tb.meta.title   = QStringLiteral("B");
    tb.meta.isValid = true;
    Track tc        = Track::from_filepath(pathC);
    tc.meta.album   = QStringLiteral("G1");
    tc.meta.title   = QStringLiteral("C");
    tc.meta.isValid = true;
    pl->add_track_object(ta);
    pl->add_track_object(tb);
    pl->add_track_object(tc);
    CHECK(pl->track_count() == 3u);
    repo.save_list_to_cache(pl);

    const EntryId id_a = ta.entry_id;
    const EntryId id_b = tb.entry_id;
    const EntryId id_c = tc.entry_id;

    PlaylistViewModel vm(&repo);
    vm.set_playlist(pid);
    pump_until([&] { return vm.rowCount(QModelIndex()) == 3; });
    CHECK(vm.rowCount(QModelIndex()) == 3);
    CHECK(vm.playback_queue().size() == 3);

    int resets = 0;
    QVector<int> removed_rows;
    QVector<bool> removed_parent_valid;
    QObject::connect(&vm, &QAbstractItemModel::modelReset, &vm, [&] { ++resets; });
    QObject::connect(&vm, &QAbstractItemModel::rowsRemoved, &vm,
                     [&](const QModelIndex& parent, int first, int last) {
                         removed_parent_valid.append(parent.isValid());
                         removed_rows.append(last - first + 1);
                     });

    // ---- 1) 平铺场景: 行级移除 c ----
    pl->remove_track(id_c);
    repo.save_list_to_cache(pl);
    vm.remove_tracks_by_ids({id_c});

    CHECK(vm.rowCount(QModelIndex()) == 2);
    CHECK(vm.playback_queue().size() == 2);
    CHECK(resets == 0); // 无整表 reset
    CHECK(removed_rows.size() == 1);
    CHECK(removed_rows.at(0) == 1);
    CHECK(removed_parent_valid.size() == 1);
    CHECK(removed_parent_valid.at(0) == false); // 平铺: 父为根

    // ---- 2) 分组场景: 移除组内最后一首 → 空组自动删除 ----
    // 注意: 重建是异步的, 必须等 modelReset 而非仅看行数(平铺态行数同为 2)
    const int resets_before_group = resets;
    vm.set_sort_expression(QStringLiteral("group { album }"));
    pump_until([&] { return resets > resets_before_group; });
    QCoreApplication::processEvents();
    CHECK(vm.rowCount(QModelIndex()) == 2);  // G1 / G2
    CHECK(vm.rowCount(vm.index(0, 0)) == 1); // G1 内剩 a

    pl->remove_track(id_a);
    repo.save_list_to_cache(pl);
    vm.remove_tracks_by_ids({id_a});

    CHECK(vm.rowCount(QModelIndex()) == 1);   // G1 空 → 删除, 只剩 G2
    CHECK(resets == resets_before_group + 1); // 移除过程无 reset
    // 两次移除: 组内曲目(父=组) + 空组(父=根)
    CHECK(removed_rows.size() == 3);
    CHECK(removed_rows.at(1) == 1);
    CHECK(removed_parent_valid.at(1) == true);  // 曲目在组内
    CHECK(removed_parent_valid.at(2) == false); // 空组从根删除
    CHECK(vm.playback_queue().size() == 1);

    const QModelIndex group_idx = vm.index(0, 0);
    CHECK(vm.rowCount(group_idx) == 1);
    CHECK(vm.track_at(vm.index(0, 0, group_idx)) == id_b);

    // ---- 3) 不存在的条目: 无副作用 ----
    vm.remove_tracks_by_ids({EntryId::create_uuid()});
    CHECK(vm.rowCount(QModelIndex()) == 1);
    CHECK(removed_rows.size() == 3);

    QCoreApplication::processEvents();
    QDir(cacheRoot).removeRecursively();

    std::printf("tb_view_remove: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
