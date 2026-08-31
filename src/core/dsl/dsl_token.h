#pragma once

#include <QString>
#include <QStringView>
#include <QVector>

namespace dsl
{

/// DSL 词法单元类型。
enum class TokenType
{
    Ident,     // 标识符(属性/原语/关键字), text 已归一小写
    Number,    // 整数, value 存值
    String,    // 字符串字面量, text 存解码后的值
    LBrace,    // {
    RBrace,    // }
    LParen,    // (
    RParen,    // )
    Comma,     // ,
    Semicolon, // ;
    Newline,   // \n
    Plus,      // +
    Minus,     // -
    Star,      // *
    Slash,     // /
    Percent,   // %
    EqEq,      // ==
    NotEq,     // !=
    Lt,        // <
    Le,        // <=
    Gt,        // >
    Ge,        // >=
    AndAnd,    // &&
    OrOr,      // ||
    Bang,      // !
    Question,  // ?
    Colon,     // :
    Eof,       // 输入结束
    Error,     // 词法错误
};

/// 单个词法单元, 携带行列号用于错误定位。
struct Token
{
    TokenType type = TokenType::Eof;
    QString text;  // Ident(归一小写) / String(解码后值)
    int value = 0; // Number 的数值
    int line  = 1; // 1 起
    int col   = 1; // 1 起
};

/// 供调试/错误信息使用的类型名。
QString token_type_name(TokenType t);

} // namespace dsl
