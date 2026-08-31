#include "core/dsl/lexer.h"

#include <QChar>

namespace dsl
{

QString token_type_name(TokenType t)
{
    switch (t) {
    case TokenType::Ident:
        return "identifier";
    case TokenType::Number:
        return "number";
    case TokenType::String:
        return "string";
    case TokenType::LBrace:
        return "'{'";
    case TokenType::RBrace:
        return "'}'";
    case TokenType::LParen:
        return "'('";
    case TokenType::RParen:
        return "')'";
    case TokenType::Comma:
        return "','";
    case TokenType::Semicolon:
        return "';'";
    case TokenType::Newline:
        return "newline";
    case TokenType::Plus:
        return "'+'";
    case TokenType::Minus:
        return "'-'";
    case TokenType::Star:
        return "'*'";
    case TokenType::Slash:
        return "'/'";
    case TokenType::Percent:
        return "'%'";
    case TokenType::EqEq:
        return "'=='";
    case TokenType::NotEq:
        return "'!='";
    case TokenType::Lt:
        return "'<'";
    case TokenType::Le:
        return "'<='";
    case TokenType::Gt:
        return "'>'";
    case TokenType::Ge:
        return "'>='";
    case TokenType::AndAnd:
        return "'&&'";
    case TokenType::OrOr:
        return "'||'";
    case TokenType::Bang:
        return "'!'";
    case TokenType::Question:
        return "'?'";
    case TokenType::Colon:
        return "':'";
    case TokenType::Eof:
        return "end of input";
    case TokenType::Error:
        return "error";
    }
    return "unknown";
}

Lexer::Lexer(QStringView src) : src_(src) {}

QChar Lexer::peek(int ahead) const
{
    const int i = pos_ + ahead;
    return (i < src_.size()) ? src_.at(i) : QChar();
}

QChar Lexer::advance()
{
    if (pos_ >= src_.size())
        return QChar();
    const QChar c = src_.at(pos_++);
    if (c == QLatin1Char('\n')) {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return c;
}

void Lexer::add_token(TokenType type, int line, int col)
{
    Token t;
    t.type = type;
    t.line = line;
    t.col  = col;
    out_.append(t);
}

void Lexer::set_error(const QString& msg)
{
    if (error_.isEmpty())
        error_ = QStringLiteral("%1:%2: %3").arg(line_).arg(col_).arg(msg);
}

QVector<Token> Lexer::tokenize()
{
    while (pos_ < src_.size()) {
        const QChar c = peek();

        // 空白(不含换行)
        if (c.isSpace() && c != QLatin1Char('\n')) {
            advance();
            continue;
        }
        // 换行 → Newline(与 ';' 等价的分隔符)
        if (c == QLatin1Char('\n')) {
            const int line = line_, col = col_;
            advance();
            add_token(TokenType::Newline, line, col);
            continue;
        }
        // 注释: '#' 到行尾
        if (c == QLatin1Char('#')) {
            while (pos_ < src_.size() && peek() != QLatin1Char('\n'))
                advance();
            continue;
        }

        const int line = line_, col = col_;

        // 双字符运算符
        const QChar n = peek(1);
        if (c == QLatin1Char('=') && n == QLatin1Char('=')) {
            advance();
            advance();
            add_token(TokenType::EqEq, line, col);
            continue;
        }
        if (c == QLatin1Char('!') && n == QLatin1Char('=')) {
            advance();
            advance();
            add_token(TokenType::NotEq, line, col);
            continue;
        }
        if (c == QLatin1Char('<') && n == QLatin1Char('=')) {
            advance();
            advance();
            add_token(TokenType::Le, line, col);
            continue;
        }
        if (c == QLatin1Char('>') && n == QLatin1Char('=')) {
            advance();
            advance();
            add_token(TokenType::Ge, line, col);
            continue;
        }
        if (c == QLatin1Char('&') && n == QLatin1Char('&')) {
            advance();
            advance();
            add_token(TokenType::AndAnd, line, col);
            continue;
        }
        if (c == QLatin1Char('|') && n == QLatin1Char('|')) {
            advance();
            advance();
            add_token(TokenType::OrOr, line, col);
            continue;
        }

        // 单字符符号
        switch (c.toLatin1()) {
        case '{':
            advance();
            add_token(TokenType::LBrace, line, col);
            continue;
        case '}':
            advance();
            add_token(TokenType::RBrace, line, col);
            continue;
        case '(':
            advance();
            add_token(TokenType::LParen, line, col);
            continue;
        case ')':
            advance();
            add_token(TokenType::RParen, line, col);
            continue;
        case ',':
            advance();
            add_token(TokenType::Comma, line, col);
            continue;
        case ';':
            advance();
            add_token(TokenType::Semicolon, line, col);
            continue;
        case '+':
            advance();
            add_token(TokenType::Plus, line, col);
            continue;
        case '-':
            advance();
            add_token(TokenType::Minus, line, col);
            continue;
        case '*':
            advance();
            add_token(TokenType::Star, line, col);
            continue;
        case '/':
            advance();
            add_token(TokenType::Slash, line, col);
            continue;
        case '%':
            advance();
            add_token(TokenType::Percent, line, col);
            continue;
        case '!':
            advance();
            add_token(TokenType::Bang, line, col);
            continue;
        case '<':
            advance();
            add_token(TokenType::Lt, line, col);
            continue;
        case '>':
            advance();
            add_token(TokenType::Gt, line, col);
            continue;
        case '?':
            advance();
            add_token(TokenType::Question, line, col);
            continue;
        case ':':
            advance();
            add_token(TokenType::Colon, line, col);
            continue;
        default:
            break;
        }

        // 字符串字面量
        if (c == QLatin1Char('"') || c == QLatin1Char('\'')) {
            const QChar quote = c;
            advance(); // 开引号
            QString value;
            bool closed = false;
            while (pos_ < src_.size()) {
                QChar ch = advance();
                if (ch == quote) {
                    closed = true;
                    break;
                }
                if (ch == QLatin1Char('\\')) {
                    if (pos_ >= src_.size())
                        break;
                    const QChar esc = advance();
                    switch (esc.toLatin1()) {
                    case '"':
                        value += QLatin1Char('"');
                        break;
                    case '\'':
                        value += QLatin1Char('\'');
                        break;
                    case '\\':
                        value += QLatin1Char('\\');
                        break;
                    case 'n':
                        value += QLatin1Char('\n');
                        break;
                    case 't':
                        value += QLatin1Char('\t');
                        break;
                    default:
                        value += esc;
                        break;
                    }
                } else {
                    value += ch;
                }
            }
            if (!closed) {
                set_error(QStringLiteral("unterminated string literal"));
                Token t;
                t.type = TokenType::Error;
                t.line = line;
                t.col  = col;
                out_.append(t);
                break;
            }
            Token t;
            t.type = TokenType::String;
            t.text = value;
            t.line = line;
            t.col  = col;
            out_.append(t);
            continue;
        }

        // 数字
        if (c.isDigit()) {
            qulonglong v = 0;
            while (pos_ < src_.size() && peek().isDigit()) {
                v = v * 10 + static_cast<qulonglong>(advance().digitValue());
            }
            Token t;
            t.type  = TokenType::Number;
            t.value = static_cast<int>(v);
            t.line  = line;
            t.col   = col;
            out_.append(t);
            continue;
        }

        // 标识符
        if (c.isLetter() || c == QLatin1Char('_')) {
            QString name;
            while (pos_ < src_.size()) {
                const QChar ch = peek();
                if (!(ch.isLetterOrNumber() || ch == QLatin1Char('_')))
                    break;
                name += advance();
            }
            Token t;
            t.type = TokenType::Ident;
            t.text = name.toLower(); // 关键字/属性名大小写无关
            t.line = line;
            t.col  = col;
            out_.append(t);
            continue;
        }

        // 非法字符
        set_error(QStringLiteral("unexpected character '%1'").arg(c));
        Token t;
        t.type = TokenType::Error;
        t.line = line;
        t.col  = col;
        out_.append(t);
        break;
    }

    Token eof;
    eof.type = TokenType::Eof;
    eof.line = line_;
    eof.col  = col_;
    out_.append(eof);
    return out_;
}

} // namespace dsl
