#include "core/dsl/evaluator.h"

#include <QRegularExpression>

namespace dsl
{

namespace
{
bool eq(const Value& a, const Value& b)
{
    if (a.type != b.type)
        return false;
    switch (a.type) {
    case Value::Type::String:
        return a.str == b.str;
    case Value::Type::Int:
        return a.num == b.num;
    case Value::Type::Bool:
        return a.flag == b.flag;
    case Value::Type::Null:
        return true;
    }
    return false;
}

/// 比较运算符(表达式内, 码点序); 类型不同或 null → 不可比。
int compare_cmp(const Value& a, const Value& b)
{
    if (a.type == Value::Type::Int && b.type == Value::Type::Int)
        return (a.num < b.num) ? -1 : (a.num > b.num ? 1 : 0);
    if (a.type == Value::Type::String && b.type == Value::Type::String)
        return QString::compare(a.str, b.str, Qt::CaseSensitive);
    if (a.type == Value::Type::Bool && b.type == Value::Type::Bool)
        return (a.flag == b.flag) ? 0 : (a.flag ? 1 : -1);
    return 0; // 类型不同 → 视为无法比较
}
} // namespace

QString value_to_string(const Value& v)
{
    switch (v.type) {
    case Value::Type::String:
        return v.str;
    case Value::Type::Int:
        return QString::number(v.num);
    case Value::Type::Bool:
        return v.flag ? QStringLiteral("true") : QStringLiteral("false");
    case Value::Type::Null:
        return QString();
    }
    return QString();
}

Evaluator::Evaluator(const Program& prog) : prog_(prog)
{
    collator_.setCaseSensitivity(Qt::CaseInsensitive);
}

Value Evaluator::eval_expr(const Node& n, const Row& row) const
{
    switch (n.kind) {
    case NodeKind::Number:
        return Value::from_int(n.value);
    case NodeKind::String:
        return Value::from_string(n.text);
    case NodeKind::Bool:
        return Value::from_bool(n.value != 0);
    case NodeKind::Null:
        return Value::null();
    case NodeKind::Property:
        return row.property(n.name);
    case NodeKind::Primitive:
        return eval_primitive(n, row);
    }
    return Value::null();
}

Value Evaluator::eval_primitive(const Node& n, const Row& row) const
{
    const auto& args = n.args;
    const auto evalA = [&](int i) { return eval_expr(args[i], row); };

    // ---- 逻辑 ----
    if (n.name == QStringLiteral("and")) {
        const Value a = evalA(0);
        if (a.is_null())
            return Value::null();
        if (!a.flag)
            return Value::from_bool(false);
        const Value b = evalA(1);
        return b.is_null() ? Value::null() : Value::from_bool(b.flag);
    }
    if (n.name == QStringLiteral("or")) {
        const Value a = evalA(0);
        if (a.is_null())
            return Value::null();
        if (a.flag)
            return Value::from_bool(true);
        const Value b = evalA(1);
        return b.is_null() ? Value::null() : Value::from_bool(b.flag);
    }
    if (n.name == QStringLiteral("not")) {
        const Value a = evalA(0);
        return a.is_null() ? Value::null() : Value::from_bool(!a.flag);
    }

    // ---- 比较(==/!= 用于判空: null==null 为 true, 与任一非空不相等) ----
    if (n.name == QStringLiteral("==") || n.name == QStringLiteral("!=")) {
        const Value a = evalA(0);
        const Value b = evalA(1);
        if (a.is_null() || b.is_null()) {
            const bool bothNull = a.is_null() && b.is_null();
            return Value::from_bool(n.name == QStringLiteral("==") ? bothNull : !bothNull);
        }
        const bool e = eq(a, b);
        return Value::from_bool(n.name == QStringLiteral("==") ? e : !e);
    }
    if (n.name == QStringLiteral("<") || n.name == QStringLiteral("<=") ||
        n.name == QStringLiteral(">") || n.name == QStringLiteral(">=")) {
        const Value a = evalA(0);
        const Value b = evalA(1);
        if (a.is_null() || b.is_null())
            return Value::null();
        if (a.type != b.type)
            return Value::null();
        const int c = compare_cmp(a, b);
        if (n.name == QStringLiteral("<"))
            return Value::from_bool(c < 0);
        if (n.name == QStringLiteral("<="))
            return Value::from_bool(c <= 0);
        if (n.name == QStringLiteral(">"))
            return Value::from_bool(c > 0);
        return Value::from_bool(c >= 0);
    }

    // ---- 算术 ----
    if (n.name == QStringLiteral("+") || n.name == QStringLiteral("-") ||
        n.name == QStringLiteral("*") || n.name == QStringLiteral("/") ||
        n.name == QStringLiteral("%")) {
        const Value a = evalA(0);
        const Value b = evalA(1);
        if (a.is_null() || b.is_null())
            return Value::null();
        if (n.name == QStringLiteral("+"))
            return Value::from_int(a.num + b.num);
        if (n.name == QStringLiteral("-"))
            return Value::from_int(a.num - b.num);
        if (n.name == QStringLiteral("*"))
            return Value::from_int(a.num * b.num);
        if (b.num == 0)
            return Value::null(); // 除零 → null
        if (n.name == QStringLiteral("/"))
            return Value::from_int(a.num / b.num);
        return Value::from_int(a.num % b.num);
    }
    if (n.name == QStringLiteral("neg")) {
        const Value a = evalA(0);
        return a.is_null() ? Value::null() : Value::from_int(-a.num);
    }

    // ---- 三元 ----
    if (n.name == QStringLiteral("ternary")) {
        const Value c = evalA(0);
        return c.to_bool() ? evalA(1) : evalA(2);
    }

    // ---- 字符串函数 ----
    if (n.name == QStringLiteral("contains")) {
        const Value a = evalA(0);
        const Value b = evalA(1);
        return (a.is_null() || b.is_null()) ? Value::null()
                                            : Value::from_bool(a.str.contains(b.str));
    }
    if (n.name == QStringLiteral("starts_with")) {
        const Value a = evalA(0);
        const Value b = evalA(1);
        return (a.is_null() || b.is_null()) ? Value::null()
                                            : Value::from_bool(a.str.startsWith(b.str));
    }
    if (n.name == QStringLiteral("ends_with")) {
        const Value a = evalA(0);
        const Value b = evalA(1);
        return (a.is_null() || b.is_null()) ? Value::null()
                                            : Value::from_bool(a.str.endsWith(b.str));
    }
    if (n.name == QStringLiteral("matches")) {
        const Value a = evalA(0);
        const Value b = evalA(1);
        if (a.is_null() || b.is_null())
            return Value::null();
        const QRegularExpression re(b.str);
        return Value::from_bool(re.isValid() && re.match(a.str).hasMatch());
    }
    if (n.name == QStringLiteral("len")) {
        const Value a = evalA(0);
        return a.is_null() ? Value::null() : Value::from_int(a.str.size());
    }
    if (n.name == QStringLiteral("upper")) {
        const Value a = evalA(0);
        return a.is_null() ? Value::null() : Value::from_string(a.str.toUpper());
    }
    if (n.name == QStringLiteral("lower")) {
        const Value a = evalA(0);
        return a.is_null() ? Value::null() : Value::from_string(a.str.toLower());
    }
    if (n.name == QStringLiteral("in")) {
        const Value a = evalA(0);
        if (a.is_null())
            return Value::null();
        for (int i = 1; i < args.size(); ++i) {
            const Value item = eval_expr(args[i], row);
            if (!item.is_null() && eq(a, item))
                return Value::from_bool(true);
        }
        return Value::from_bool(false);
    }

    return Value::null(); // 未识别原语(validate 应已拦截)
}

int Evaluator::compare_values(const Value& a, const Value& b) const
{
    if (a.type == Value::Type::Int && b.type == Value::Type::Int)
        return (a.num < b.num) ? -1 : (a.num > b.num ? 1 : 0);
    if (a.type == Value::Type::String && b.type == Value::Type::String)
        return collator_.compare(a.str, b.str);
    if (a.type == Value::Type::Bool && b.type == Value::Type::Bool)
        return (a.flag == b.flag) ? 0 : (a.flag ? 1 : -1);
    return 0;
}

bool Evaluator::less(const Row& a, const Row& b) const
{
    for (const auto& item : prog_.sort) {
        const Value va = a.property(item.property);
        const Value vb = b.property(item.property);
        const bool na  = va.is_null();
        const bool nb  = vb.is_null();
        if (na || nb) {
            if (na && nb)
                continue;
            const bool first = (item.nulls == SortItem::Nulls::First);
            return na ? first : !first;
        }
        const int cmp = compare_values(va, vb);
        if (cmp != 0)
            return item.desc ? (cmp > 0) : (cmp < 0);
    }
    return false;
}

QStringList Evaluator::group_keys(const Row& row) const
{
    QStringList keys;
    keys.reserve(prog_.group.size());
    for (const auto& item : prog_.group) {
        const Value v = row.property(item.property);
        QString key;
        if (v.is_null())
            key = QStringLiteral("unknown");
        else {
            key = value_to_string(v);
            if (key.isEmpty())
                key = QStringLiteral("unknown");
        }
        keys.append(key);
    }
    return keys;
}

QString Evaluator::bucket_key(const Row& row) const
{
    for (const auto& b : prog_.bucket) {
        if (b.kind == BucketBranch::Kind::Else) {
            return value_to_string(eval_expr(b.key, row));
        }
        const Value c = eval_expr(b.cond, row);
        if (c.to_bool()) {
            return value_to_string(eval_expr(b.key, row));
        }
    }
    return QStringLiteral("未分类");
}

} // namespace dsl
