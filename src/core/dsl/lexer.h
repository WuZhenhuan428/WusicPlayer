#pragma once

#include "core/dsl/dsl_token.h"

namespace dsl
{

/// DSL 词法分析器。
///
/// 规则(与 docs/DSL.md §3.4 一致):
///  - 标识符 [A-Za-z_][A-Za-z0-9_]*, 解析时归一为小写;
///  - 整数(无符号, 负号由 parser 的 unary 处理);
///  - 字符串 "..." 或 '...', 支持转义 \" \' \\ \n \t;
///  - 注释 `#` 到行尾, 完全忽略;
///  - 换行产生 Newline, 与 Semicolon 等价(parser 统一视作分隔符);
///  - 遇到非法字符/未闭合字符串 → 产出 Error token 并记录错误信息。
///
/// 关键字(and/or/not/true/false/null 等)不在词法层区分,
/// 一律产出 Ident(小写), 由 parser/registry 按上下文解释——保持词法简单。
class Lexer
{
public:
    explicit Lexer(QStringView src);

    /// 逐 token 扫描直到 Eof; 失败时在末尾追加 Error token。
    QVector<Token> tokenize();

    bool has_error() const
    {
        return !error_.isEmpty();
    }
    /// 形如 "1:5: unterminated string literal"。
    QString error_message() const
    {
        return error_;
    }

private:
    QStringView src_;
    int pos_  = 0;
    int line_ = 1;
    int col_  = 1;

    QVector<Token> out_;
    QString error_;

    QChar peek(int ahead = 0) const;
    QChar advance();
    void add_token(TokenType type, int line, int col);
    void set_error(const QString& msg);
};

} // namespace dsl
