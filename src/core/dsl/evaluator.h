#pragma once

#include "core/dsl/ast.h"
#include "core/dsl/registry.h"

#include <QCollator>
#include <QString>
#include <QStringList>
#include <QVector>

namespace dsl
{

/// 运行时属性值。
struct Value
{
    enum class Type
    {
        String,
        Int,
        Bool,
        Null
    };
    Type type = Type::Null;
    QString str;
    int num   = 0;
    bool flag = false;

    static Value from_string(const QString& s)
    {
        Value v;
        v.type = Type::String;
        v.str  = s;
        return v;
    }
    static Value from_int(int n)
    {
        Value v;
        v.type = Type::Int;
        v.num  = n;
        return v;
    }
    static Value from_bool(bool b)
    {
        Value v;
        v.type = Type::Bool;
        v.flag = b;
        return v;
    }
    static Value null()
    {
        return Value{};
    }
    bool is_null() const
    {
        return type == Type::Null;
    }
    /// bool 上下文: null 视为 false(对齐 docs/DSL.md §7)。
    bool to_bool() const
    {
        return type == Type::Bool && flag;
    }
};

/// 一条记录的属性访问接口(由宿主实现)。
///
/// validate() 已保证 DSL 中引用的属性均合法; 宿主对已注册属性返回其值,
/// 缺失时返回 Value::null()(排序按 nulls 策略, 分组归 "unknown")。
class Row
{
public:
    virtual ~Row()                                    = default;
    virtual Value property(const QString& name) const = 0;
};

/// 求值器: 把已验证的 Program 解释为排序比较器 / 分组键 / 分类键。
///
/// 构造前必须已通过 Registry::validate()(属性名已规范化); 对每条记录,
/// 直接解释原语树求值(树大小即单次求值成本)。
class Evaluator
{
public:
    explicit Evaluator(const Program& prog);

    // ---- 表达式求值(也用于 bucket 键 / 测试) ----
    Value eval_expr(const Node& n, const Row& row) const;

    // ---- 排序: a 是否排在 b 前 ----
    bool less(const Row& a, const Row& b) const;

    // ---- 分组: 每级一个键(空/缺失 → "unknown") ----
    QStringList group_keys(const Row& row) const;

    // ---- 分类: if/elif/else 命中分支的键; 未命中 → "未分类" ----
    QString bucket_key(const Row& row) const;

    bool has_sort() const
    {
        return !prog_.sort.isEmpty();
    }
    bool has_group() const
    {
        return prog_.has_group;
    }
    bool has_bucket() const
    {
        return prog_.has_bucket;
    }
    const QVector<SortItem>& sort_items() const
    {
        return prog_.sort;
    }
    const QVector<GroupItem>& group_items() const
    {
        return prog_.group;
    }

private:
    Value eval_primitive(const Node& n, const Row& row) const;
    /// 非空值比较, 返回 -1/0/1(排序用, 字符串走 QCollator)。
    int compare_values(const Value& a, const Value& b) const;

    Program prog_;
    QCollator collator_; // 语言感知、忽略大小写(排序/分组键序)
};

/// 测试/调试: 值 → 可读字符串。
QString value_to_string(const Value& v);

} // namespace dsl
