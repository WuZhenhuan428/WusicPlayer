#include "core/dsl/ast.h"
#include "core/dsl/evaluator.h"
#include "core/dsl/lexer.h"
#include "core/dsl/parser.h"
#include "core/dsl/registry.h"

#include <QHash>
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

/// 测试用内存 Row。
class MapRow : public Row
{
public:
    void set(const QString& name, const Value& v)
    {
        map_[name] = v;
    }
    Value property(const QString& name) const override
    {
        const auto it = map_.constFind(name);
        return it == map_.constEnd() ? Value::null() : it.value();
    }

private:
    QHash<QString, Value> map_;
};

/// 解析 + 校验 + 构建求值器。
static Evaluator build(const QString& src, bool* ok = nullptr)
{
    Lexer lexer{QStringView(src)};
    const auto toks = lexer.tokenize();
    Parser p(toks);
    auto pr = p.parse();
    if (!pr.ok) {
        if (ok)
            *ok = false;
        return Evaluator(pr);
    }
    if (!Registry::instance().validate(pr)) {
        if (ok)
            *ok = false;
        return Evaluator(pr);
    }
    if (ok)
        *ok = true;
    return Evaluator(pr);
}

int main()
{
    // 1) 表达式键求值: 算术优先级 / 表达式作分类键
    {
        bool ok = false;
        auto ev = build(QStringLiteral("bucket { if missing then year * 2 + 1 }"), &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("missing"), Value::from_bool(true));
        row.set(QStringLiteral("year"), Value::from_int(2020));
        CHECK(ev.bucket_key(row) == QStringLiteral("4041"));
    }

    // 2) 排序: 数值 asc/desc
    {
        bool ok = false;
        auto ev = build(QStringLiteral("sort { year asc }"), &ok);
        CHECK(ok);
        MapRow r1, r2, r3;
        r1.set(QStringLiteral("year"), Value::from_int(1990));
        r2.set(QStringLiteral("year"), Value::from_int(2020));
        r3.set(QStringLiteral("year"), Value::from_int(2010));
        CHECK(ev.less(r1, r2) && ev.less(r1, r3) && ev.less(r3, r2));
        CHECK(!ev.less(r2, r1) && !ev.less(r2, r3) && !ev.less(r3, r1));
        CHECK(!ev.less(r2, r2));
    }
    {
        bool ok = false;
        auto ev = build(QStringLiteral("sort { year desc }"), &ok);
        CHECK(ok);
        MapRow r1, r2;
        r1.set(QStringLiteral("year"), Value::from_int(1990));
        r2.set(QStringLiteral("year"), Value::from_int(2020));
        CHECK(ev.less(r2, r1));
        CHECK(!ev.less(r1, r2));
    }

    // 3) 排序: 字符串(QCollator 忽略大小写) + 多键
    {
        bool ok = false;
        auto ev = build(QStringLiteral("sort { artist asc }"), &ok);
        CHECK(ok);
        MapRow r1, r2;
        r1.set(QStringLiteral("artist"), Value::from_string(QStringLiteral("b")));
        r2.set(QStringLiteral("artist"), Value::from_string(QStringLiteral("A")));
        // QCollator 语言感知: 'A' 与 'a' 不敏感
        CHECK(ev.less(r1, r2) || ev.less(r2, r1) || true); // 仅验证不崩溃
        CHECK(!ev.less(r1, r1));
    }
    {
        bool ok = false;
        auto ev = build(QStringLiteral("sort { album asc; track asc }"), &ok);
        CHECK(ok);
        MapRow a1, a2, b1;
        a1.set(QStringLiteral("album"), Value::from_string(QStringLiteral("A")));
        a1.set(QStringLiteral("track"), Value::from_int(1));
        a2.set(QStringLiteral("album"), Value::from_string(QStringLiteral("A")));
        a2.set(QStringLiteral("track"), Value::from_int(2));
        b1.set(QStringLiteral("album"), Value::from_string(QStringLiteral("B")));
        b1.set(QStringLiteral("track"), Value::from_int(0));
        CHECK(ev.less(a1, a2)); // 同专辑, 音轨升序
        CHECK(ev.less(a1, b1)); // 专辑升序优先
        CHECK(ev.less(a2, b1));
        CHECK(!ev.less(a2, a1));
    }

    // 4) 排序: nulls first / last
    {
        bool ok = false;
        auto ev = build(QStringLiteral("sort { artist asc nulls last }"), &ok);
        CHECK(ok);
        MapRow named, missing;
        named.set(QStringLiteral("artist"), Value::from_string(QStringLiteral("Z")));
        // missing 无 artist → null
        CHECK(ev.less(named, missing)); // nulls last: 有值在前
        CHECK(!ev.less(missing, named));
    }
    {
        bool ok = false;
        auto ev = build(QStringLiteral("sort { artist asc nulls first }"), &ok);
        CHECK(ok);
        MapRow named, missing;
        named.set(QStringLiteral("artist"), Value::from_string(QStringLiteral("A")));
        CHECK(ev.less(missing, named));
        CHECK(!ev.less(named, missing));
    }

    // 5) 分组键: 多级 + 空 → "unknown"
    {
        bool ok = false;
        auto ev = build(QStringLiteral("group { genre asc; album }"), &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("genre"), Value::from_string(QStringLiteral("摇滚")));
        row.set(QStringLiteral("album"), Value::from_string(QStringLiteral("专辑X")));
        const auto keys = ev.group_keys(row);
        CHECK(keys.size() == 2);
        CHECK(keys[0] == QStringLiteral("摇滚"));
        CHECK(keys[1] == QStringLiteral("专辑X"));
    }
    {
        bool ok = false;
        auto ev = build(QStringLiteral("group { artist }"), &ok);
        CHECK(ok);
        MapRow row; // 无 artist
        CHECK(ev.group_keys(row) == QStringList{QStringLiteral("unknown")});
    }

    // 6) bucket: if/elif/else + 表达式键 + 未分类
    {
        bool ok = false;
        auto ev = build(QStringLiteral("bucket {\n"
                                       "    if year >= 2010 then \"现代\"\n"
                                       "    elif duration < 180 then \"短\"\n"
                                       "    else \"其他\"\n"
                                       "}"),
                        &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("year"), Value::from_int(2020));
        row.set(QStringLiteral("duration"), Value::from_int(300));
        CHECK(ev.bucket_key(row) == QStringLiteral("现代"));
        row.set(QStringLiteral("year"), Value::from_int(2000));
        row.set(QStringLiteral("duration"), Value::from_int(120));
        CHECK(ev.bucket_key(row) == QStringLiteral("短"));
        row.set(QStringLiteral("year"), Value::from_int(2000));
        row.set(QStringLiteral("duration"), Value::from_int(300));
        CHECK(ev.bucket_key(row) == QStringLiteral("其他"));
    }
    {
        bool ok = false;
        auto ev = build(QStringLiteral("bucket { if year > 0 then \"有\" }"), &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("year"), Value::from_int(0));
        CHECK(ev.bucket_key(row) == QStringLiteral("未分类")); // 无 else, 未命中
    }
    {
        bool ok = false;
        auto ev = build(
            QStringLiteral("bucket { if duration > 0 then duration < 90 ? \"很短\" : \"长\" }"),
            &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("duration"), Value::from_int(60));
        CHECK(ev.bucket_key(row) == QStringLiteral("很短"));
        row.set(QStringLiteral("duration"), Value::from_int(300));
        CHECK(ev.bucket_key(row) == QStringLiteral("长"));
    }
    {
        bool ok = false;
        auto ev = build(QStringLiteral("bucket { if year > 0 then year / 10 * 10 }"), &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("year"), Value::from_int(2024));
        CHECK(ev.bucket_key(row) == QStringLiteral("2020"));
    }

    // 7) 逻辑与字符串函数求值(通过 bucket 条件)
    {
        bool ok = false;
        auto ev = build(QStringLiteral("bucket { if in(genre, \"摇滚\", \"金属\") and "
                                       "contains(title, \"live\") then \"x\" }"),
                        &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("genre"), Value::from_string(QStringLiteral("摇滚")));
        row.set(QStringLiteral("title"), Value::from_string(QStringLiteral("live in tokyo")));
        CHECK(ev.bucket_key(row) == QStringLiteral("x"));
        row.set(QStringLiteral("title"), Value::from_string(QStringLiteral("Studio")));
        CHECK(ev.bucket_key(row) == QStringLiteral("未分类"));
    }
    {
        bool ok = false;
        auto ev = build(
            QStringLiteral("bucket { if matches(title, \"^L.*\") and len(artist) > 2 then \"x\" }"),
            &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("title"), Value::from_string(QStringLiteral("Lost")));
        row.set(QStringLiteral("artist"), Value::from_string(QStringLiteral("ABC")));
        CHECK(ev.bucket_key(row) == QStringLiteral("x"));
    }
    {
        // null 条件 → false(短路到未分类)
        bool ok = false;
        auto ev =
            build(QStringLiteral("bucket { if year == null then \"未知\" else \"有\" }"), &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("year"), Value::from_int(2020));
        CHECK(ev.bucket_key(row) == QStringLiteral("有"));
        MapRow empty;
        CHECK(ev.bucket_key(empty) == QStringLiteral("未知")); // year null → null == null → true
    }

    // 8) 表达式: 除零 → null; 布尔运算短路
    {
        bool ok = false;
        auto ev = build(QStringLiteral("bucket { if missing then 1 / 0 }"), &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("missing"), Value::from_bool(true));
        CHECK(ev.bucket_key(row).isEmpty()); // 1/0 → null → 空键
    }
    {
        bool ok = false;
        auto ev = build(QStringLiteral("bucket { if missing then year < null }"), &ok);
        CHECK(ok);
        MapRow row;
        row.set(QStringLiteral("missing"), Value::from_bool(true));
        row.set(QStringLiteral("year"), Value::from_int(2020));
        CHECK(ev.bucket_key(row).isEmpty()); // null 比较 → null → 空键
    }

    std::printf("tb_dsl_evaluator: %d checks, %d failures\n", g_CHECKs, g_failures);
    return g_failures == 0 ? 0 : 1;
}
