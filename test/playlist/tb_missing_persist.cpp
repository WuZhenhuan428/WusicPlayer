#include "model/playlist/playlist.h"
#include "model/playlist/playlist_repo.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>

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

/// 清理缺失曲目后, cache 中的 .wcpl 应同步删除对应项(重启后不再出现)。
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv); // 可执行名隔离 cache 目录

    // 清理历史测试数据
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                              QStringLiteral("/playlists");
    QDir(cacheRoot).removeRecursively();

    // 真实存在的正常文件(清理判定基于文件可达性, 必须真实存在才不会被误删)
    QTemporaryDir dir;
    CHECK(dir.isValid());
    const QString okPath  = dir.filePath(QStringLiteral("ok.mp3"));
    const QString libMiss = dir.filePath(QStringLiteral("lib_missing.mp3"));
    const QString extGone = dir.filePath(QStringLiteral("external_gone.mp3"));
    {
        QFile f(okPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write("x");
        }
    }
    CHECK(QFileInfo::exists(okPath));
    CHECK(!QFileInfo::exists(libMiss));
    CHECK(!QFileInfo::exists(extGone));

    PlaylistId pid;
    {
        PlaylistRepo repo;
        pid = repo.create_list();
        CHECK(!pid.is_null());
        auto pl = repo.find_playlist_by_id(pid);
        CHECK(pl != nullptr);

        // 1 首正常 + 2 首缺失:
        //  - lib_miss: 显式 missing 标记(库引用)
        //  - ext_gone: 外部条目, 从未标 missing, 但文件实际不存在(最易被漏清的场景)
        pl->add_track(okPath);
        Track lib_miss   = Track::from_filepath(libMiss);
        lib_miss.missing = true;
        pl->add_track_object(lib_miss);
        Track ext_gone   = Track::from_filepath(extGone);
        ext_gone.missing = false; // 外部缺失: 标志从未被设置
        pl->add_track_object(ext_gone);
        CHECK(pl->track_count() == 3u);
        repo.save_list_to_cache(pl); // 落盘(3 首)

        // "清理标灰曲目": 应同时清除显式缺失与文件实际不存在的条目
        const int removed = pl->remove_missing_tracks();
        CHECK(removed == 2);
        CHECK(pl->track_count() == 1u);
        repo.save_list_to_cache(pl); // 再次落盘(应只剩 1 首)
    }

    // 模拟重启: 从磁盘重新加载, pid 应保持一致(序列化含 uuid)
    {
        PlaylistRepo repo2;
        repo2.load_cache();
        bool found      = false;
        long long count = -1;
        for (const auto& p : repo2.get_lists()) {
            if (p->id() == pid) {
                found = true;
                count = static_cast<long long>(p->track_count());
                break;
            }
        }
        CHECK(found);
        CHECK(count == 1); // 缺失项已从持久化文件中删除
    }

    QDir(cacheRoot).removeRecursively();

    std::printf("tb_missing_persist: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
