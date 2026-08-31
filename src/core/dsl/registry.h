#pragma once

#include "core/dsl/ast.h"

#include <QStringList>
#include <QVector>

namespace dsl
{

/// 属性值类型。
enum class ValueType
{
    String,
    Int,
    Bool,
};

/// 属性声明(注册表条目)。
struct PropertyDecl
{
    QString name;
    ValueType type = ValueType::String;
    QStringList aliases;
};

/// 节点静态类型(含 Any/Null 供类型检查)。
enum class NodeType
{
    String,
    Int,
    Bool,
    Any, // 未定(如 ternary 结果), 检查时放宽
    Null,
};

/// 原语签名。
struct PrimitiveSig
{
    QString name;
    QVector<NodeType> params;
    NodeType ret             = NodeType::Any;
    bool variadic            = false; // 变参(in)
    int minArgs              = 0;
    bool sameTypeFirst       = false; // == != <...: 前两参数须同类型
    bool sameTypeLast        = false; // ternary: 后两参数须同类型
    bool intOrStringOnly     = false; // < <= > >=: 参数仅 Int/String
    bool variadicSameAsFirst = false; // in: 变参须与首参同类型
};

/// 属性/原语注册表 + 静态校验。
///
/// 引擎唯一数据源: 新增属性只需登记 PropertyDecl, 新增能力登记 PrimitiveSig,
/// 排序/分组/分类自动获得能力。对齐 docs/DSL.md §4/§5/§6。
class Registry
{
public:
    static Registry& instance();

    /// 查属性(含别名); 返回规范声明或 nullptr。
    const PropertyDecl* find_property(const QString& name) const;
    /// 是否为已注册原语名。
    bool is_primitive(const QString& name) const;

    /// 静态校验: 属性/原语名称 + 类型; 成功时把属性名规范化为规范名。
    /// 失败时设置 prog.ok=false 并写入错误(带 行:列)。
    bool validate(Program& prog) const;

private:
    Registry();
    bool check_node(Node& n, Program& prog) const;
    NodeType node_type(const Node& n) const;
    void set_error(Program& prog, int line, int col, const QString& msg) const;

    QVector<PropertyDecl> properties_;
    QVector<PrimitiveSig> primitives_;
};

/// 调试/测试: 节点类型 → 可读名。
QString node_type_name(NodeType t);

} // namespace dsl
