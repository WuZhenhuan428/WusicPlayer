#include "core/dsl/lexer.h"
#include "core/dsl/parser.h"
#include "core/dsl/registry.h"
#include "core/dsl/track_meta_row.h"
#include "model/library/library_browse_model.h"
#include "model/playlist/playlist.h"
#include "model/playlist/playlist_layout.h"

#include <QString>

#include <cstdio>

static int g_CHECKs;
static int g_failures;

#define CHECK(...)                                                                                 \
    do {                                                                                           \
        ++g_CHECKs;                                                                                \
        if (!(__VA_ARGS__)) {                                                                      \
            ++g_failures;                                                                          \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                     \
        }                                                                                          \
    } while (0)

/// 构造含 3 首曲目的播放列表: A(1990, rock), B(2020, jazz), C(2010, rock)。
static Playlist make_playlist()
{
    Playlist pl(QStringLiteral("dsl-test"));
    auto mk = [&](const QString& title, int year, const QString& genre) {
        Track t           = Track::from_filepath(QStringLiteral("/tmp/%1.mp3").arg(title));
        t.meta.isValid    = true;
        t.meta.title      = title;
        t.meta.year       = year;
        t.meta.genre      = genre;
        t.meta.duration_s = 200;
        pl.add_track_object(t);
    };
    mk(QStringLiteral("A"), 1990, QStringLiteral("rock"));
    mk(QStringLiteral("B"), 2020, QStringLiteral("jazz"));
    mk(QStringLiteral("C"), 2010, QStringLiteral("rock"));
    return pl;
}

/// 按 queue 中 EntryId 顺序返回标题列表。
static QStringList queue_titles(const LayoutResult& res, const Playlist& pl)
{
    QStringList out;
    for (const auto& eid : res.playback_queue) {
        const Track* t = pl.find_track_by_id(eid);
        out << (t ? t->meta.title : QStringLiteral("<missing>"));
    }
    return out;
}

int main()
{
    const Playlist pl = make_playlist();

    // 1) DSL sort desc: B(2020) → C(2010) → A(1990)
    {
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("sort { year desc }")));
        CHECK(b.has_dsl());
        const auto res = b.build(pl);
        CHECK(queue_titles(res, pl) ==
              QStringList{QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("A")});
    }

    // 2) DSL sort asc
    {
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("sort { year asc }")));
        const auto res = b.build(pl);
        CHECK(queue_titles(res, pl) ==
              QStringList{QStringLiteral("A"), QStringLiteral("C"), QStringLiteral("B")});
    }

    // 3) DSL 多级分组: genre 组, 组内 year desc
    {
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("group { genre } sort { year desc }")));
        const auto res = b.build(pl);
        CHECK(res.root->children.size() == 2);
        // 组间 QCollator 升序: jazz < rock
        CHECK(res.root->children[0]->group_name == QStringLiteral("jazz"));
        CHECK(res.root->children[0]->children.size() == 1);
        CHECK(res.root->children[1]->group_name == QStringLiteral("rock"));
        CHECK(res.root->children[1]->children.size() == 2);
        // rock 组内按 year desc: C(2010) 在 A(1990) 前
        const Node* rock = res.root->children[1];
        CHECK(rock->children[0]->meta.title == QStringLiteral("C"));
        CHECK(rock->children[1]->meta.title == QStringLiteral("A"));
        // 播放队列: 组序 jazz(B) → rock(C, A)
        CHECK(queue_titles(res, pl) ==
              QStringList{QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("A")});
    }

    // 4) bucket 分类
    {
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("bucket { if year >= 2010 then \"new\" else \"old\" }")));
        const auto res = b.build(pl);
        CHECK(res.root->children.size() == 2);
        // 升序: new < old
        CHECK(res.root->children[0]->group_name == QStringLiteral("new"));
        CHECK(res.root->children[0]->children.size() == 2); // B, C
        CHECK(res.root->children[1]->group_name == QStringLiteral("old"));
        CHECK(res.root->children[1]->children.size() == 1); // A
    }

    // 5) 非法 DSL: 失败且保留原规则 + 错误可读
    {
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("sort { year desc }")));
        CHECK(!b.set_dsl(QStringLiteral("sort { foobar }")));
        CHECK(!b.dsl_error().isEmpty());
        CHECK(b.has_dsl()); // 原 DSL 保留
        const auto res = b.build(pl);
        CHECK(queue_titles(res, pl) ==
              QStringList{QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("A")});
    }

    // 6) 空表达式 → 清除 DSL(无规则 → 原始顺序)
    {
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("sort { year desc }")));
        CHECK(b.set_dsl(QString()));
        CHECK(!b.has_dsl());
        CHECK(b.dsl_error().isEmpty());
        const auto res = b.build(pl);
        CHECK(queue_titles(res, pl) ==
              QStringList{QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")});
    }

    // 7) 组内排序(无显式 sort → 保持原始顺序)
    {
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("group { genre }")));
        const auto res = b.build(pl);
        CHECK(res.root->children.size() == 2);
        CHECK(res.root->children[1]->group_name == QStringLiteral("rock"));
        // 无 sort: A 在 C 前(原始顺序)
        CHECK(res.root->children[1]->children[0]->meta.title == QStringLiteral("A"));
        CHECK(res.root->children[1]->children[1]->meta.title == QStringLiteral("C"));
    }

    // 8) 空/纯注释 DSL → 无规则
    {
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("  ")));
        CHECK(!b.has_dsl());
    }

    // 9) format() 占位符("Unknown xxx")在 DSL 中视为空值 → nulls/unknown 生效
    {
        TrackMetaData meta;
        meta.isValid = true;
        meta.title   = QStringLiteral("T");
        meta.album   = QStringLiteral("Unknown Album");
        meta.artist  = QStringLiteral("Unknown Artist");
        meta.genre   = QStringLiteral("Unknown Genre");
        dsl::TrackMetaRow row(meta);
        CHECK(row.property(QStringLiteral("album")).is_null());
        CHECK(row.property(QStringLiteral("artist")).is_null());
        CHECK(row.property(QStringLiteral("genre")).is_null());
        meta.album = QStringLiteral("Real Album");
        CHECK(!row.property(QStringLiteral("album")).is_null());
        CHECK(row.property(QStringLiteral("album")).str == QStringLiteral("Real Album"));
    }

    // 10) 集成: "Unknown Album" 不再作为真实专辑组名, 归入 "unknown";
    //     sort 的 nulls first 生效
    {
        Playlist pl(QStringLiteral("dsl-unknown"));
        auto mk2 = [&](const QString& title, const QString& album) {
            Track t        = Track::from_filepath(QStringLiteral("/tmp/%1.mp3").arg(title));
            t.meta.isValid = true;
            t.meta.title   = title;
            t.meta.album   = album;
            pl.add_track_object(t);
        };
        mk2(QStringLiteral("A"), QStringLiteral("Album X"));
        mk2(QStringLiteral("B"), QStringLiteral("Unknown Album"));
        mk2(QStringLiteral("C"), QStringLiteral("Album X"));

        // group: Unknown Album → "unknown" 组, 而非真实组名
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(QStringLiteral("group { album } sort { title asc }")));
        const auto res = b.build(pl);
        CHECK(res.root->children.size() == 2);
        CHECK(res.root->children[0]->group_name == QStringLiteral("Album X"));
        CHECK(res.root->children[0]->children.size() == 2);
        CHECK(res.root->children[1]->group_name == QStringLiteral("unknown"));
        CHECK(res.root->children[1]->children.size() == 1);

        // sort nulls first: Unknown Album 曲目排最前
        PlaylistLayoutBuilder b2;
        CHECK(b2.set_dsl(QStringLiteral("sort { album asc nulls first }")));
        const auto res2 = b2.build(pl);
        CHECK(res2.root->children.size() == 3);
        CHECK(res2.root->children[0]->meta.title == QStringLiteral("B"));
    }

    // 11) dsl_source 往返(持久化数据源)
    {
        const QString src = QStringLiteral("sort { year desc }");
        PlaylistLayoutBuilder b;
        CHECK(b.set_dsl(src));
        CHECK(b.dsl_source() == src);
        CHECK(b.set_dsl(QStringLiteral("  "))); // 空 → 清除
        CHECK(b.dsl_source().isEmpty());

        LibraryBrowseModel model(nullptr);
        CHECK(model.set_dsl_grouping(src));
        CHECK(model.has_dsl());
        CHECK(model.dsl_source() == src);
        model.clear_dsl_grouping();
        CHECK(!model.has_dsl());
        CHECK(model.dsl_source().isEmpty());
    }

    std::printf("tb_dsl_integration: %d checks, %d failures\n", g_CHECKs, g_failures);
    return g_failures == 0 ? 0 : 1;
}
