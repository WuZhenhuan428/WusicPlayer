#pragma once

#include <QString>
#include <QVector>

namespace dsl
{

/// 表达式节点 —— 原语树(与 docs/DSL.md §6 原语模型一致)。
///
/// 中缀/前缀运算符、函数调用统一表示为原语节点:
///   `a and b`   → Primitive("and", [a, b])
///   `a >= b`    → Primitive(">=", [a, b])
///   `-a`        → Primitive("neg", [a])
///   `a ? b : c` → Primitive("ternary", [a, b, c])
///   `in(x, y)`  → Primitive("in", [x, y])
/// 属性引用为 Property 节点; 名称一律小写(词法层已归一)。
enum class NodeKind
{
    Primitive, // 原语调用, args 为参数
    Number,    // value
    String,    // text
    Bool,      // value != 0
    Null,      // 字面量 null
    Property,  // 属性引用, name
};

struct Node
{
    NodeKind kind = NodeKind::Null;
    QString name;       // Primitive: 原语名; Property: 属性名(小写)
    int value = 0;      // Number / Bool
    QString text;       // String 字面量
    QVector<Node> args; // Primitive 参数
    int line = 1, col = 1;

    static Node make_primitive(const QString& name, const QVector<Node>& args, int line = 1,
                               int col = 1)
    {
        Node n;
        n.kind = NodeKind::Primitive;
        n.name = name;
        n.args = args;
        n.line = line;
        n.col  = col;
        return n;
    }
    static Node make_number(int v, int line, int col)
    {
        Node n;
        n.kind  = NodeKind::Number;
        n.value = v;
        n.line  = line;
        n.col   = col;
        return n;
    }
    static Node make_string(const QString& s, int line, int col)
    {
        Node n;
        n.kind = NodeKind::String;
        n.text = s;
        n.line = line;
        n.col  = col;
        return n;
    }
    static Node make_bool(bool b, int line, int col)
    {
        Node n;
        n.kind  = NodeKind::Bool;
        n.value = b ? 1 : 0;
        n.line  = line;
        n.col   = col;
        return n;
    }
    static Node make_null(int line, int col)
    {
        Node n;
        n.kind = NodeKind::Null;
        n.line = line;
        n.col  = col;
        return n;
    }
    static Node make_property(const QString& name, int line, int col)
    {
        Node n;
        n.kind = NodeKind::Property;
        n.name = name;
        n.line = line;
        n.col  = col;
        return n;
    }
};

/// 调试/测试用: 节点 → 可读字符串(S 表达式风格)。
inline QString node_to_string(const Node& n)
{
    switch (n.kind) {
    case NodeKind::Number:
        return QString::number(n.value);
    case NodeKind::String:
        return QStringLiteral("\"%1\"").arg(n.text);
    case NodeKind::Bool:
        return n.value ? QStringLiteral("true") : QStringLiteral("false");
    case NodeKind::Null:
        return QStringLiteral("null");
    case NodeKind::Property:
        return n.name;
    case NodeKind::Primitive: {
        QStringList parts;
        for (const auto& a : n.args)
            parts << node_to_string(a);
        return QStringLiteral("(%1 %2)").arg(n.name, parts.join(QLatin1Char(' ')));
    }
    }
    return QStringLiteral("<unknown>");
}

/// sort 小节的一条规则。
struct SortItem
{
    QString property; // 属性名(小写)
    bool desc = false;
    enum class Nulls
    {
        Default,
        First,
        Last
    };
    Nulls nulls = Nulls::Default;
    int line = 1, col = 1;
};

/// group 小节的一条规则(顺序即层级)。
struct GroupItem
{
    QString property; // 属性名(小写)
    bool desc = false;
    int line = 1, col = 1;
};

/// bucket 小节的一条分支。
struct BucketBranch
{
    enum class Kind
    {
        If,
        Elif,
        Else
    };
    Kind kind = Kind::If;
    Node cond; // If/Elif 的条件表达式
    Node key;  // 分类键(字面量或表达式)
    int line = 1, col = 1;
};

/// 一次完整 DSL 解析的结果。ok=false 时 error 携带 行:列 定位的首个错误。
struct Program
{
    QVector<SortItem> sort;
    QVector<GroupItem> group;
    QVector<BucketBranch> bucket;
    bool has_sort   = false;
    bool has_group  = false;
    bool has_bucket = false;

    bool ok         = true;
    QString error;
    int error_line = 1;
    int error_col  = 1;
};

} // namespace dsl
