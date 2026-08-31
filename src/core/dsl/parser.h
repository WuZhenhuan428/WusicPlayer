#pragma once

#include "core/dsl/ast.h"
#include "core/dsl/dsl_token.h"

#include <QVector>

namespace dsl
{

/// DSL 递归下降解析器(对齐 docs/DSL.md §3 语法规范)。
///
/// - 顶层: `sort {}` / `group {}` / `bucket {}` 三节, 块内子句以 `;` 或换行分隔;
/// - 表达式: 中缀优先级(三元 → or → and → not → 比较 → 加减 → 乘除 → 一元),
///   统一折叠为原语树(见 ast.h);
/// - `group` 与 `bucket` 互斥, 各小节至多出现一次;
/// - 属性名/原语名不做语义校验(注册表在 registry 阶段负责),
///   仅做语法层检查, 错误带 行:列 定位。
class Parser
{
public:
    explicit Parser(const QVector<Token>& tokens);

    /// 解析完整 DSL; 失败时返回 ok=false 的 Program(含首个错误定位)。
    Program parse();

private:
    const QVector<Token>& ts_;
    int pos_ = 0;
    Program prog_;

    // ---- token 辅助 ----
    const Token& cur() const
    {
        return ts_[pos_];
    }
    const Token& peek(int ahead) const
    {
        const int i = pos_ + ahead;
        return ts_[qMin(i, ts_.size() - 1)];
    }
    bool at(TokenType t) const
    {
        return cur().type == t;
    }
    bool at_ident(const QString& name) const
    {
        return at(TokenType::Ident) && cur().text == name;
    }
    bool accept(TokenType t);
    bool accept_ident(const QString& name);
    bool expect(TokenType t, const QString& what);
    bool expect_ident(const QString& name, const QString& what);
    void error(const QString& msg, const Token& at);
    bool skip_seps();

    // ---- 小节 ----
    bool parse_sort();
    bool parse_group();
    bool parse_bucket();
    Node parse_key_expr();

    // ---- 表达式 ----
    Node parse_expr();
    Node parse_ternary();
    Node parse_or();
    Node parse_and();
    Node parse_not();
    Node parse_cmp();
    Node parse_add();
    Node parse_mul();
    Node parse_unary();
    Node parse_primary();
};

} // namespace dsl
