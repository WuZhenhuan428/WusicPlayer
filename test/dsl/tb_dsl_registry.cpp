#include "core/dsl/ast.h"
#include "core/dsl/dsl_token.h"
#include "core/dsl/lexer.h"
#include "core/dsl/parser.h"
#include "core/dsl/registry.h"

#include <QString>
#include <QStringView>

#include <cstdio>

static int g_CHECKs;
static int g_failures;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        ++g_CHECKs;                                                                                \
        if (!(cond)) {                                                                             \
            ++g_failures;                                                                          \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                            \
        }                                                                                          \
    } while (0)

using namespace dsl;

/// 解析 + 静态校验。
static Program parse_validate(const QString& src, bool* ok = nullptr)
{
    Lexer lexer{QStringView(src)};
    const auto toks = lexer.tokenize();
    Parser p(toks);
    auto pr = p.parse();
    if (!pr.ok) {
        if (ok)
            *ok = false;
        return pr;
    }
    const bool v = Registry::instance().validate(pr);
    if (ok)
        *ok = v;
    return pr;
}

int main()
{
    // 1) 所有属性可作为 sort 键并规范化(别名 → 规范名)
    {
        const char* props[] = {"title",    "artist",   "album",      "album_artist", "genre",
                               "composer", "comment",  "lyrics",     "encoder",      "date",
                               "filename", "filepath", "directory",  "extension",    "year",
                               "track",    "disc",     "disc_total", "duration",     "bitrate",
                               "start_at", "index",    "missing"};
        for (const char* p : props) {
            bool ok = false;
            auto pr = parse_validate(QStringLiteral("sort { %1 }").arg(QLatin1String(p)), &ok);
            if (!ok) {
                std::printf("FAIL: property '%s' rejected: %s\n", p, qPrintable(pr.error));
                ++g_failures;
            }
            ++g_CHECKs;
        }
    }

    // 2) 别名解析与规范化
    {
        bool ok = false;
        auto pr = parse_validate(QStringLiteral("sort { track_number asc; folder desc }"), &ok);
        CHECK(ok);
        CHECK(pr.sort.size() == 2);
        CHECK(pr.sort[0].property == QStringLiteral("track"));
        CHECK(pr.sort[1].property == QStringLiteral("directory"));
    }
    {
        bool ok = false;
        auto pr = parse_validate(QStringLiteral("group { length; path }"), &ok);
        CHECK(ok);
        CHECK(pr.group[0].property == QStringLiteral("duration"));
        CHECK(pr.group[1].property == QStringLiteral("directory"));
    }
    {
        bool ok = false;
        auto pr = parse_validate(QStringLiteral("bucket { if disc_number > 1 then \"x\" }"), &ok);
        CHECK(ok);
        CHECK(node_to_string(pr.bucket[0].cond) == QStringLiteral("(> disc 1)"));
    }

    // 3) 未知属性
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("sort { foobar }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("unknown property 'foobar'")));
    }
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("bucket { if year > nope then \"x\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("unknown property 'nope'")));
    }

    // 4) 未知函数
    {
        bool ok = true;
        auto pr =
            parse_validate(QStringLiteral("bucket { if containsx(title, \"a\") then \"x\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("unknown function 'containsx'")));
    }

    // 5) 类型检查
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("bucket { if year and missing then \"x\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("expected bool, got int")));
    }
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("bucket { if year + \"x\" == 1 then \"x\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("expected int, got string")));
    }
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("bucket { if year == \"x\" then \"x\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("must have the same type")));
    }
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("bucket { if missing < missing then \"x\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("requires numeric or string operands")));
    }
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("bucket { if year then \"a\" else \"b\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("condition must be bool")));
    }
    {
        bool ok = true;
        auto pr =
            parse_validate(QStringLiteral("bucket { if year > 0 then missing ? \"a\" : 5 }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("branches of ternary")));
    }
    {
        bool ok = true;
        auto pr =
            parse_validate(QStringLiteral("bucket { if contains(title, 5) then \"x\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("expected string, got int")));
    }

    // 6) 合法组合通过
    {
        bool ok = false;
        auto pr = parse_validate(
            QStringLiteral("bucket {\n"
                           "    if year >= 2010 and genre == \"摇滚\" then \"现代\"\n"
                           "    elif duration < 180 then duration < 90 ? \"很短\" : \"短\"\n"
                           "    else \"其他\"\n"
                           "}\n"
                           "sort { album_artist asc; album asc; disc asc; track asc }"),
            &ok);
        CHECK(ok);
    }

    // 7) null 兼容
    {
        bool ok = false;
        auto pr = parse_validate(
            QStringLiteral("bucket { if year == null then \"未知年代\" else \"有\" }"), &ok);
        CHECK(ok);
    }
    {
        bool ok = false;
        auto pr = parse_validate(QStringLiteral("bucket { if year < null then \"x\" }"), &ok);
        CHECK(ok); // 运行时返回 null → false
    }

    // 8) in 变参
    {
        bool ok = false;
        auto pr = parse_validate(
            QStringLiteral("bucket { if in(genre, \"摇滚\", \"金属\") then \"x\" }"), &ok);
        CHECK(ok);
    }
    {
        bool ok = true;
        auto pr =
            parse_validate(QStringLiteral("bucket { if in(genre, \"摇滚\", 5) then \"x\" }"), &ok);
        CHECK(!ok);
    }
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("bucket { if in(genre) then \"x\" }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("at least 2")));
    }

    // 9) 保留字作属性 → 未知属性(parser 已接受, registry 拒绝)
    {
        bool ok = true;
        auto pr = parse_validate(QStringLiteral("sort { and }"), &ok);
        CHECK(!ok);
        CHECK(pr.error.contains(QStringLiteral("unknown property 'and'")));
    }

    // 10) 字符串函数与数值运算
    {
        bool ok = false;
        auto pr = parse_validate(
            QStringLiteral("bucket { if matches(title, \"^L.*\") and len(artist) > 3 then \"x\" }"),
            &ok);
        CHECK(ok);
    }
    {
        bool ok = false;
        auto pr = parse_validate(
            QStringLiteral("bucket { if upper(genre) == \"ROCK\" then \"x\" }"), &ok);
        CHECK(ok);
    }
    {
        bool ok = false;
        auto pr =
            parse_validate(QStringLiteral("bucket { if year / 10 * 10 == 2020 then \"x\" }"), &ok);
        CHECK(ok);
    }

    std::printf("tb_dsl_registry: %d checks, %d failures\n", g_CHECKs, g_failures);
    return g_failures == 0 ? 0 : 1;
}
