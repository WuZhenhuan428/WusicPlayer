#include "core/dsl/ast.h"
#include "core/dsl/lexer.h"
#include "core/dsl/parser.h"

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

static Program parse(const QString& src)
{
    Lexer lexer{QStringView(src)};
    const auto toks = lexer.tokenize();
    Parser p(toks);
    return p.parse();
}

int main()
{
    // 1) sort + group 完整解析
    {
        auto pr = parse(QStringLiteral(
            "sort {\n"
            "    artist asc nulls last\n"
            "    year desc\n"
            "    title\n"
            "}\n"
            "group {\n"
            "    genre asc\n"
            "    album\n"
            "}\n"));
        CHECK(pr.ok);
        CHECK(pr.has_sort && pr.has_group && !pr.has_bucket);
        CHECK(pr.sort.size() == 3);
        CHECK(pr.sort[0].property == QStringLiteral("artist"));
        CHECK(!pr.sort[0].desc);
        CHECK(pr.sort[0].nulls == SortItem::Nulls::Last);
        CHECK(pr.sort[1].property == QStringLiteral("year"));
        CHECK(pr.sort[1].desc);
        CHECK(pr.sort[1].nulls == SortItem::Nulls::Default);
        CHECK(pr.sort[2].property == QStringLiteral("title"));
        CHECK(pr.group.size() == 2);
        CHECK(pr.group[0].property == QStringLiteral("genre") && !pr.group[0].desc);
        CHECK(pr.group[1].property == QStringLiteral("album"));
    }

    // 2) sort + bucket(if/elif/else, 表达式键 + 三元)
    {
        auto pr = parse(QStringLiteral(
            "sort { year desc }\n"
            "bucket {\n"
            "    if year >= 2010 and genre == \"摇滚\" then \"现代\"\n"
            "    elif duration < 180 then duration < 90 ? \"很短\" : \"短\"\n"
            "    else \"其他\"\n"
            "}\n"));
        CHECK(pr.ok);
        CHECK(pr.has_sort && pr.has_bucket && !pr.has_group);
        CHECK(pr.bucket.size() == 3);
        CHECK(pr.bucket[0].kind == BucketBranch::Kind::If);
        CHECK(pr.bucket[1].kind == BucketBranch::Kind::Elif);
        CHECK(pr.bucket[2].kind == BucketBranch::Kind::Else);
        CHECK(node_to_string(pr.bucket[0].cond)
              == QStringLiteral("(and (>= year 2010) (== genre \"摇滚\"))"));
        CHECK(node_to_string(pr.bucket[0].key) == QStringLiteral("\"现代\""));
        CHECK(node_to_string(pr.bucket[1].key)
              == QStringLiteral("(ternary (< duration 90) \"很短\" \"短\")"));
        CHECK(node_to_string(pr.bucket[2].key) == QStringLiteral("\"其他\""));
    }

    // 3) 表达式: 一元负 / not / 函数调用 / 算术优先级
    {
        auto pr = parse(QStringLiteral(
            "bucket { if -year > 0 then \"x\" }\n"
            "bucket2 {}")); // 非法, 只取第一个
        (void)pr;
    }
    {
        auto pr = parse(QStringLiteral("bucket { if -year > 0 then \"x\" }"));
        CHECK(pr.ok);
        CHECK(node_to_string(pr.bucket[0].cond) == QStringLiteral("(> (neg year) 0)"));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if !missing then \"x\" }"));
        CHECK(pr.ok);
        CHECK(node_to_string(pr.bucket[0].cond) == QStringLiteral("(not missing)"));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if not (year > 0) then \"x\" }"));
        CHECK(pr.ok);
        CHECK(node_to_string(pr.bucket[0].cond) == QStringLiteral("(not (> year 0))"));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if in(genre, \"摇滚\", \"金属\") then \"x\" }"));
        CHECK(pr.ok);
        CHECK(node_to_string(pr.bucket[0].cond)
              == QStringLiteral("(in genre \"摇滚\" \"金属\")"));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if year / 10 * 10 == 2020 then \"x\" }"));
        CHECK(pr.ok);
        CHECK(node_to_string(pr.bucket[0].cond) == QStringLiteral("(== (* (/ year 10) 10) 2020)"));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if (year >= 2000) and (year < 2010) then \"x\" }"));
        CHECK(pr.ok);
        CHECK(node_to_string(pr.bucket[0].cond)
              == QStringLiteral("(and (>= year 2000) (< year 2010))"));
    }

    // 4) 错误: bucket 分支顺序
    {
        auto pr = parse(QStringLiteral("bucket { elif year > 0 then \"x\" }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("elif")));
    }
    {
        auto pr = parse(QStringLiteral(
            "bucket { if year > 0 then \"a\" else \"b\" elif year then \"c\" }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("elif after 'else'")));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if year > 0 then \"a\" else \"b\" else \"c\" }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("duplicate 'else'")));
    }

    // 5) 错误: group/bucket 互斥
    {
        auto pr = parse(QStringLiteral("group { genre } bucket { if year then \"x\" }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("mutually exclusive")));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if year then \"x\" } group { genre }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("mutually exclusive")));
    }

    // 6) 错误: 重复小节 / 缺 } / 缺 then / 表达式残缺
    {
        auto pr = parse(QStringLiteral("sort { artist } sort { year }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("duplicate 'sort'")));
    }
    {
        auto pr = parse(QStringLiteral("sort { artist"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("unterminated")));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if year \"x\" }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("'then'")));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if then \"x\" }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("unexpected keyword 'then'")));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if year >= then \"x\" }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("unexpected keyword 'then'")));
    }
    {
        auto pr = parse(QStringLiteral("bucket { if year then }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("expected expression")));
    }
    {
        auto pr = parse(QStringLiteral("foo { }"));
        CHECK(!pr.ok);
        CHECK(pr.error.contains(QStringLiteral("expected 'sort', 'group' or 'bucket'")));
    }

    // 7) 空输入 / 纯注释 / 空块
    {
        auto pr = parse(QString());
        CHECK(pr.ok && !pr.has_sort && !pr.has_group && !pr.has_bucket);
    }
    {
        auto pr = parse(QStringLiteral("# just a comment\n"));
        CHECK(pr.ok && !pr.has_sort);
    }
    {
        auto pr = parse(QStringLiteral("sort { }"));
        CHECK(pr.ok && pr.has_sort && pr.sort.isEmpty());
    }

    // 8) 单行紧凑(分号)与大小写无关
    {
        auto pr = parse(QStringLiteral("SORT { Artist ASC; YEAR desc }"));
        CHECK(pr.ok);
        CHECK(pr.sort.size() == 2);
        CHECK(pr.sort[0].property == QStringLiteral("artist") && !pr.sort[0].desc);
        CHECK(pr.sort[1].property == QStringLiteral("year") && pr.sort[1].desc);
    }

    // 9) 错误定位: 行:列 信息存在
    {
        auto pr = parse(QStringLiteral("sort {\n    artist\n    year desc extra\n}"));
        CHECK(pr.ok); // extra 是第二个 item, 合法
        CHECK(pr.sort.size() == 3);
        CHECK(pr.sort[2].property == QStringLiteral("extra"));
    }
    {
        auto pr = parse(QStringLiteral("sort { artist\nbucket { }"));
        CHECK(!pr.ok);
        CHECK(pr.error_line >= 1);
        CHECK(pr.error.contains(QLatin1Char(':')));
    }

    std::printf("tb_dsl_parser: %d checks, %d failures\n", g_CHECKs, g_failures);
    return g_failures == 0 ? 0 : 1;
}
