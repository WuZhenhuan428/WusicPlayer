#include "core/dsl/registry.h"

#include <QStringList>

namespace dsl
{

namespace
{
/// 属性名不得与之冲突的保留字(结构/逻辑关键字)。
const QStringList kReservedWords = {
    QStringLiteral("sort"),  QStringLiteral("group"), QStringLiteral("bucket"),
    QStringLiteral("if"),    QStringLiteral("elif"),  QStringLiteral("else"),
    QStringLiteral("then"),  QStringLiteral("asc"),   QStringLiteral("desc"),
    QStringLiteral("nulls"), QStringLiteral("first"), QStringLiteral("last"),
    QStringLiteral("and"),   QStringLiteral("or"),    QStringLiteral("not"),
    QStringLiteral("true"),  QStringLiteral("false"), QStringLiteral("null"),
};

bool concrete(NodeType t)
{
    return t == NodeType::String || t == NodeType::Int || t == NodeType::Bool;
}

/// 类型兼容: Any 放宽; Null/Any 实际类型与任何要求兼容(运行时再定)。
bool compatible(NodeType required, NodeType actual)
{
    return required == NodeType::Any || actual == NodeType::Any || actual == NodeType::Null ||
           required == actual;
}
} // namespace

QString node_type_name(NodeType t)
{
    switch (t) {
    case NodeType::String:
        return QStringLiteral("string");
    case NodeType::Int:
        return QStringLiteral("int");
    case NodeType::Bool:
        return QStringLiteral("bool");
    case NodeType::Any:
        return QStringLiteral("any");
    case NodeType::Null:
        return QStringLiteral("null");
    }
    return QStringLiteral("unknown");
}

Registry& Registry::instance()
{
    static Registry s_instance;
    return s_instance;
}

Registry::Registry()
{
    // ---- 属性表(对齐 docs/DSL.md §4) ----
    auto prop = [this](const QString& name, ValueType type, QStringList aliases = {}) {
        properties_.push_back(PropertyDecl{name, type, std::move(aliases)});
    };
    prop(QStringLiteral("title"), ValueType::String);
    prop(QStringLiteral("artist"), ValueType::String);
    prop(QStringLiteral("album"), ValueType::String);
    prop(QStringLiteral("album_artist"), ValueType::String);
    prop(QStringLiteral("genre"), ValueType::String);
    prop(QStringLiteral("composer"), ValueType::String);
    prop(QStringLiteral("comment"), ValueType::String);
    prop(QStringLiteral("lyrics"), ValueType::String);
    prop(QStringLiteral("encoder"), ValueType::String);
    prop(QStringLiteral("date"), ValueType::String);
    prop(QStringLiteral("filename"), ValueType::String);
    prop(QStringLiteral("filepath"), ValueType::String);
    prop(QStringLiteral("directory"), ValueType::String,
         {QStringLiteral("folder"), QStringLiteral("path")});
    prop(QStringLiteral("extension"), ValueType::String);

    prop(QStringLiteral("year"), ValueType::Int);
    prop(QStringLiteral("track"), ValueType::Int, {QStringLiteral("track_number")});
    prop(QStringLiteral("disc"), ValueType::Int, {QStringLiteral("disc_number")});
    prop(QStringLiteral("disc_total"), ValueType::Int);
    prop(QStringLiteral("duration"), ValueType::Int, {QStringLiteral("length")});
    prop(QStringLiteral("bitrate"), ValueType::Int);
    prop(QStringLiteral("start_at"), ValueType::Int);
    prop(QStringLiteral("index"), ValueType::Int);

    prop(QStringLiteral("missing"), ValueType::Bool);

    // ---- 原语表(对齐 docs/DSL.md §6) ----
    auto prim = [this](const QString& name, QVector<NodeType> params, NodeType ret,
                       PrimitiveSig extra = {}) {
        PrimitiveSig s;
        s.name                = name;
        s.params              = std::move(params);
        s.ret                 = ret;
        s.variadic            = extra.variadic;
        s.minArgs             = extra.minArgs;
        s.sameTypeFirst       = extra.sameTypeFirst;
        s.sameTypeLast        = extra.sameTypeLast;
        s.intOrStringOnly     = extra.intOrStringOnly;
        s.variadicSameAsFirst = extra.variadicSameAsFirst;
        primitives_.push_back(s);
    };

    // 逻辑
    prim(QStringLiteral("and"), {NodeType::Bool, NodeType::Bool}, NodeType::Bool);
    prim(QStringLiteral("or"), {NodeType::Bool, NodeType::Bool}, NodeType::Bool);
    prim(QStringLiteral("not"), {NodeType::Bool}, NodeType::Bool);
    // 比较
    PrimitiveSig eq;
    eq.sameTypeFirst = true;
    prim(QStringLiteral("=="), {NodeType::Any, NodeType::Any}, NodeType::Bool, eq);
    prim(QStringLiteral("!="), {NodeType::Any, NodeType::Any}, NodeType::Bool, eq);
    PrimitiveSig ord;
    ord.sameTypeFirst   = true;
    ord.intOrStringOnly = true;
    prim(QStringLiteral("<"), {NodeType::Any, NodeType::Any}, NodeType::Bool, ord);
    prim(QStringLiteral("<="), {NodeType::Any, NodeType::Any}, NodeType::Bool, ord);
    prim(QStringLiteral(">"), {NodeType::Any, NodeType::Any}, NodeType::Bool, ord);
    prim(QStringLiteral(">="), {NodeType::Any, NodeType::Any}, NodeType::Bool, ord);
    // 算术
    prim(QStringLiteral("+"), {NodeType::Int, NodeType::Int}, NodeType::Int);
    prim(QStringLiteral("-"), {NodeType::Int, NodeType::Int}, NodeType::Int);
    prim(QStringLiteral("*"), {NodeType::Int, NodeType::Int}, NodeType::Int);
    prim(QStringLiteral("/"), {NodeType::Int, NodeType::Int}, NodeType::Int);
    prim(QStringLiteral("%"), {NodeType::Int, NodeType::Int}, NodeType::Int);
    prim(QStringLiteral("neg"), {NodeType::Int}, NodeType::Int);
    // 三元
    PrimitiveSig ter;
    ter.sameTypeLast = true;
    prim(QStringLiteral("ternary"), {NodeType::Bool, NodeType::Any, NodeType::Any}, NodeType::Any,
         ter);
    // 字符串函数
    prim(QStringLiteral("contains"), {NodeType::String, NodeType::String}, NodeType::Bool);
    prim(QStringLiteral("starts_with"), {NodeType::String, NodeType::String}, NodeType::Bool);
    prim(QStringLiteral("ends_with"), {NodeType::String, NodeType::String}, NodeType::Bool);
    prim(QStringLiteral("matches"), {NodeType::String, NodeType::String}, NodeType::Bool);
    prim(QStringLiteral("len"), {NodeType::String}, NodeType::Int);
    prim(QStringLiteral("upper"), {NodeType::String}, NodeType::String);
    prim(QStringLiteral("lower"), {NodeType::String}, NodeType::String);
    PrimitiveSig in_sig;
    in_sig.variadic            = true;
    in_sig.minArgs             = 2;
    in_sig.variadicSameAsFirst = true;
    prim(QStringLiteral("in"), {NodeType::Any}, NodeType::Bool, in_sig);

    // ---- 保留字冲突检查(构建期断言) ----
    for (const auto& p : properties_) {
        Q_ASSERT(!kReservedWords.contains(p.name));
        for (const auto& alias : p.aliases)
            Q_ASSERT(!kReservedWords.contains(alias));
        for (const auto& s : primitives_)
            Q_ASSERT(p.name != s.name && !p.aliases.contains(s.name));
    }
}

const PropertyDecl* Registry::find_property(const QString& name) const
{
    for (const auto& p : properties_) {
        if (p.name == name || p.aliases.contains(name))
            return &p;
    }
    return nullptr;
}

bool Registry::is_primitive(const QString& name) const
{
    for (const auto& s : primitives_) {
        if (s.name == name)
            return true;
    }
    return false;
}

void Registry::set_error(Program& prog, int line, int col, const QString& msg) const
{
    if (prog.ok) {
        prog.ok         = false;
        prog.error      = QStringLiteral("%1:%2: %3").arg(line).arg(col).arg(msg);
        prog.error_line = line;
        prog.error_col  = col;
    }
}

NodeType Registry::node_type(const Node& n) const
{
    switch (n.kind) {
    case NodeKind::Number:
        return NodeType::Int;
    case NodeKind::String:
        return NodeType::String;
    case NodeKind::Bool:
        return NodeType::Bool;
    case NodeKind::Null:
        return NodeType::Null;
    case NodeKind::Property: {
        const PropertyDecl* d = find_property(n.name);
        if (!d)
            return NodeType::Any;
        switch (d->type) {
        case ValueType::String:
            return NodeType::String;
        case ValueType::Int:
            return NodeType::Int;
        case ValueType::Bool:
            return NodeType::Bool;
        }
        return NodeType::Any;
    }
    case NodeKind::Primitive:
        for (const auto& s : primitives_) {
            if (s.name == n.name)
                return s.ret;
        }
        return NodeType::Any;
    }
    return NodeType::Any;
}

bool Registry::check_node(Node& n, Program& prog) const
{
    switch (n.kind) {
    case NodeKind::Number:
    case NodeKind::String:
    case NodeKind::Bool:
    case NodeKind::Null:
        return true;
    case NodeKind::Property: {
        const PropertyDecl* d = find_property(n.name);
        if (!d) {
            set_error(prog, n.line, n.col, QStringLiteral("unknown property '%1'").arg(n.name));
            return false;
        }
        n.name = d->name; // 规范化为规范名
        return true;
    }
    case NodeKind::Primitive: {
        const PrimitiveSig* sig = nullptr;
        for (const auto& s : primitives_) {
            if (s.name == n.name) {
                sig = &s;
                break;
            }
        }
        if (!sig) {
            set_error(prog, n.line, n.col, QStringLiteral("unknown function '%1'").arg(n.name));
            return false;
        }

        // 参数个数
        if (sig->variadic) {
            if (n.args.size() < sig->minArgs) {
                set_error(prog, n.line, n.col,
                          QStringLiteral("function '%1' expects at least %2 argument(s), got %3")
                              .arg(n.name)
                              .arg(sig->minArgs)
                              .arg(n.args.size()));
                return false;
            }
        } else if (n.args.size() != sig->params.size()) {
            set_error(prog, n.line, n.col,
                      QStringLiteral("function '%1' expects %2 argument(s), got %3")
                          .arg(n.name)
                          .arg(sig->params.size())
                          .arg(n.args.size()));
            return false;
        }

        // 递归检查并收集实际类型
        QVector<NodeType> types;
        types.reserve(n.args.size());
        for (auto& arg : n.args) {
            if (!check_node(arg, prog))
                return false;
            types.append(node_type(arg));
        }

        // 参数类型匹配
        for (int i = 0; i < n.args.size(); ++i) {
            const NodeType required = (sig->variadic && i >= sig->params.size())
                                          ? (sig->variadicSameAsFirst ? types[0] : NodeType::Any)
                                          : sig->params[i];
            if (!compatible(required, types[i])) {
                set_error(prog, n.line, n.col,
                          QStringLiteral("type mismatch in '%1': expected %2, got %3")
                              .arg(n.name, node_type_name(required), node_type_name(types[i])));
                return false;
            }
        }

        // 前两参数同类型(== != < <= > >=)
        if (sig->sameTypeFirst && n.args.size() >= 2) {
            const NodeType a = types[0];
            const NodeType b = types[1];
            if (concrete(a) && concrete(b) && a != b) {
                set_error(prog, n.line, n.col,
                          QStringLiteral("operands of '%1' must have the same type (%2 vs %3)")
                              .arg(n.name, node_type_name(a), node_type_name(b)));
                return false;
            }
        }
        // 后两参数同类型(ternary then/else)
        if (sig->sameTypeLast && n.args.size() >= 3) {
            const NodeType a = types[1];
            const NodeType b = types[2];
            if (concrete(a) && concrete(b) && a != b) {
                set_error(prog, n.line, n.col,
                          QStringLiteral("branches of ternary must have the same type (%2 vs %3)")
                              .arg(node_type_name(a), node_type_name(b)));
                return false;
            }
        }
        // < <= > >=: 仅 Int/String
        if (sig->intOrStringOnly) {
            for (int i = 0; i < n.args.size() && i < 2; ++i) {
                const NodeType t = types[i];
                if (concrete(t) && t != NodeType::Int && t != NodeType::String) {
                    set_error(prog, n.line, n.col,
                              QStringLiteral("'%1' requires numeric or string operands, got %2")
                                  .arg(n.name, node_type_name(t)));
                    return false;
                }
            }
        }
        return true;
    }
    }
    return true;
}

bool Registry::validate(Program& prog) const
{
    for (auto& item : prog.sort) {
        if (!find_property(item.property)) {
            set_error(prog, item.line, item.col,
                      QStringLiteral("unknown property '%1'").arg(item.property));
            return false;
        }
        item.property = find_property(item.property)->name; // 规范化
    }
    for (auto& item : prog.group) {
        if (!find_property(item.property)) {
            set_error(prog, item.line, item.col,
                      QStringLiteral("unknown property '%1'").arg(item.property));
            return false;
        }
        item.property = find_property(item.property)->name;
    }
    for (auto& b : prog.bucket) {
        if (b.kind != BucketBranch::Kind::Else) {
            if (!check_node(b.cond, prog))
                return false;
            // if/elif 条件必须是 bool
            const NodeType ct = node_type(b.cond);
            if (concrete(ct) && ct != NodeType::Bool) {
                set_error(prog, b.cond.line, b.cond.col,
                          QStringLiteral("'if'/'elif' condition must be bool, got %1")
                              .arg(node_type_name(ct)));
                return false;
            }
        }
        if (!check_node(b.key, prog))
            return false;
    }
    return true;
}

} // namespace dsl
