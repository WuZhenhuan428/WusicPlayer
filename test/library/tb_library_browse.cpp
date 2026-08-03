#include "core/utils/path.hpp"
#include "model/library/library_browse_model.h"
#include "model/library/library_manager.h"

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
void make_audio_file(const QString& dir, const QString& name)
{
    QDir().mkpath(dir);
    QFile f(dir + QLatin1Char('/') + name);
    if (f.open(QIODevice::WriteOnly)) {
        f.write("dummy audio data");
        f.close();
    }
}

// 扫描临时音乐根目录并等待完成
void scan_music(LibraryManager* lib, const QString& db_path, const QString& music_root,
                int expected)
{
    QEventLoop loop;
    bool scanned = false;
    QObject::connect(lib, &LibraryManager::sgn_scan_finished, &loop, [&]() {
        scanned = true;
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit); // 超时保护
    CHECK(lib->initialize(db_path));
    lib->add_watched_folder(music_root);
    loop.exec();
    CHECK(scanned);
    CHECK(lib->track_count() == expected);
}

QString leaf_title(LibraryBrowseModel* model, const QModelIndex& group, int row)
{
    const QModelIndex idx = model->index(row, 0, group);
    return model->data(idx, Qt::DisplayRole).toString();
}
} // namespace

/* ---- 分组浏览 / 搜索 / 平铺 ---- */
static void test_browse_model()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_root;
    CHECK(data_dir.isValid());
    CHECK(music_root.isValid());
    if (!data_dir.isValid() || !music_root.isValid()) {
        return;
    }

    // 目录结构:A/{alpha,beta}.mp3、B/gamma.flac(假文件 → 无标签,回退文件名)
    make_audio_file(music_root.path() + "/A", "alpha.mp3");
    make_audio_file(music_root.path() + "/A", "beta.mp3");
    make_audio_file(music_root.path() + "/B", "gamma.flac");

    LibraryManager lib;
    scan_music(&lib, data_dir.filePath("lib.db"), music_root.path(), 3);

    LibraryBrowseModel model(&lib);
    CHECK(model.grouping() == LibraryGrouping::artist); // 默认分类
    CHECK(model.columnCount() == 4);

    // ---- folder 分组 ----
    model.set_grouping(LibraryGrouping::folder);
    CHECK(model.rowCount() == 2); // 两个目录
    const QModelIndex g0 = model.index(0, 0);
    const QModelIndex g1 = model.index(1, 0);
    CHECK(g0.isValid() && g1.isValid());
    CHECK(!g0.parent().isValid()); // 分组节点挂在根
    CHECK(model.parent(g0) == QModelIndex());
    CHECK(model.rowCount(g0) >= 1);
    CHECK(model.rowCount(g0) + model.rowCount(g1) == 3);

    // 组内曲目行:parent 正确、行数据为文件名(无标签回退)
    const QModelIndex leaf = model.index(0, 0, g0);
    CHECK(leaf.isValid());
    CHECK(model.parent(leaf) == g0);
    const QString title0 = model.data(leaf, Qt::DisplayRole).toString();
    CHECK(title0 == "alpha.mp3" || title0 == "beta.mp3");

    // track_id_at:叶节点可解析,分组节点不可
    const auto tid = model.track_id_at(leaf);
    CHECK(tid.has_value());
    CHECK(!model.track_id_at(g0).has_value());
    CHECK(!model.track_id_at(QModelIndex()).has_value());

    // 无效索引
    CHECK(model.index(99, 0) == QModelIndex());
    CHECK(model.index(0, 0, leaf) == QModelIndex()); // 叶节点没有子

    // ---- 平铺(none):单组 All Tracks ----
    model.set_grouping(LibraryGrouping::none);
    CHECK(model.rowCount() == 1);
    const QModelIndex g_all = model.index(0, 0);
    CHECK(g_all.isValid());
    CHECK(model.data(g_all, Qt::DisplayRole).toString().contains("All Tracks"));
    CHECK(model.rowCount(g_all) == 3);

    // ---- 空标签分类:artist 回退 "Unknown Artist";year=0 → "(Unknown)" ----
    model.set_grouping(LibraryGrouping::artist);
    CHECK(model.rowCount() == 1);
    const QModelIndex g_unk = model.index(0, 0);
    CHECK(model.data(g_unk, Qt::DisplayRole).toString().contains("Unknown Artist"));
    CHECK(model.rowCount(g_unk) == 3);

    model.set_grouping(LibraryGrouping::year);
    CHECK(model.rowCount() == 1); // 无年份 → "(Unknown)" 组
    CHECK(model.data(model.index(0, 0), Qt::DisplayRole).toString().contains("(Unknown)"));
    CHECK(model.rowCount(model.index(0, 0)) == 3);
    model.set_grouping(LibraryGrouping::genre);
    CHECK(model.rowCount() == 1);

    // ---- FTS5 搜索:结果按当前分类分组 ----
    model.set_grouping(LibraryGrouping::folder);
    model.set_keyword("gamma");
    CHECK(model.rowCount() == 1); // 只命中 B 目录
    const QModelIndex g_hit = model.index(0, 0);
    CHECK(model.rowCount(g_hit) == 1);
    CHECK(leaf_title(&model, g_hit, 0) == "gamma.flac");
    const auto hit_tid = model.track_id_at(model.index(0, 0, g_hit));
    CHECK(hit_tid.has_value());

    // 无命中 → 空
    model.set_keyword("zzz_no_such");
    CHECK(model.rowCount() == 0);

    // 清空关键字 → 回退全量
    model.set_keyword("");
    CHECK(model.rowCount() == 2);

    // ---- header ----
    CHECK(model.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString() == "Title");
    CHECK(model.headerData(1, Qt::Horizontal, Qt::DisplayRole).toString() == "Artist");
    CHECK(model.headerData(3, Qt::Horizontal, Qt::DisplayRole).toString() == "Duration");
}

/* ---- set_library 后注入(保留分组/关键字并重建) ---- */
static void test_set_library_reinject()
{
    QTemporaryDir data_dir;
    QTemporaryDir music_root;
    CHECK(data_dir.isValid());
    CHECK(music_root.isValid());
    if (!data_dir.isValid() || !music_root.isValid()) {
        return;
    }
    make_audio_file(music_root.path() + "/A", "alpha.mp3");
    make_audio_file(music_root.path() + "/B", "beta.mp3");

    LibraryManager lib;
    scan_music(&lib, data_dir.filePath("lib.db"), music_root.path(), 2);

    // 先以空库构造,后注入
    LibraryBrowseModel model(nullptr);
    CHECK(model.rowCount() == 0);
    model.set_grouping(LibraryGrouping::folder);
    model.set_keyword("beta");
    model.set_library(&lib);
    CHECK(model.rowCount() == 1); // 命中 B(关键字保留)
    CHECK(model.rowCount(model.index(0, 0)) == 1);
    CHECK(leaf_title(&model, model.index(0, 0), 0) == "beta.mp3");
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    test_browse_model();
    test_set_library_reinject();

    std::printf("== tb_library_browse: %d checks, %d failures ==\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
