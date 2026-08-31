#include "core/dsl/dsl_token.h"
#include "core/dsl/lexer.h"

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

static QVector<Token> run(const QString& src, bool* ok = nullptr)
{
    Lexer lexer{QStringView(src)};
    auto tokens = lexer.tokenize();
    if (ok)
        *ok = !lexer.has_error();
    return tokens;
}

int main()
{
    // 1) 空输入 → 仅 Eof
    {
        bool ok = false;
        auto ts = run(QString(), &ok);
        CHECK(ok);
        CHECK(ts.size() == 1);
        CHECK(ts[0].type == TokenType::Eof);
    }

    // 2) 注释被忽略, 换行保留
    {
        bool ok = false;
        auto ts = run(QStringLiteral("# hello\nsort { }\n"), &ok);
        CHECK(ok);
        // Newline, Ident(sort), LBrace, RBrace, Newline, Eof
        CHECK(ts.size() == 6);
        CHECK(ts[0].type == TokenType::Newline);
        CHECK(ts[1].type == TokenType::Ident && ts[1].text == QStringLiteral("sort"));
        CHECK(ts[2].type == TokenType::LBrace);
        CHECK(ts[3].type == TokenType::RBrace);
        CHECK(ts[5].type == TokenType::Eof);
    }

    // 3) 完整 DSL: 标识符/属性/方向, 关键字大小写归一
    {
        bool ok = false;
        auto ts = run(QStringLiteral("SORT { Artist ASC; YEAR desc nulls last }"), &ok);
        CHECK(ok);
        CHECK(ts[0].type == TokenType::Ident && ts[0].text == QStringLiteral("sort"));
        CHECK(ts[2].type == TokenType::Ident && ts[2].text == QStringLiteral("artist"));
        CHECK(ts[3].type == TokenType::Ident && ts[3].text == QStringLiteral("asc"));
        CHECK(ts[4].type == TokenType::Semicolon);
        CHECK(ts[5].type == TokenType::Ident && ts[5].text == QStringLiteral("year"));
        CHECK(ts[6].type == TokenType::Ident && ts[6].text == QStringLiteral("desc"));
        CHECK(ts[8].type == TokenType::Ident && ts[8].text == QStringLiteral("last"));
        CHECK(ts[9].type == TokenType::RBrace);
    }

    // 4) bucket 块: if / elif / else / then / 比较 / 逻辑
    {
        bool ok = false;
        auto ts = run(
            QStringLiteral("bucket { if year >= 2010 and genre == \"摇滚\" then \"现代\" }"), &ok);
        CHECK(ok);
        CHECK(ts[0].type == TokenType::Ident && ts[0].text == QStringLiteral("bucket"));
        CHECK(ts[2].type == TokenType::Ident && ts[2].text == QStringLiteral("if"));
        CHECK(ts[4].type == TokenType::Ge);
        CHECK(ts[6].type == TokenType::Ident && ts[6].text == QStringLiteral("and"));
        CHECK(ts[8].type == TokenType::EqEq);
        CHECK(ts[9].type == TokenType::String && ts[9].text == QStringLiteral("摇滚"));
        CHECK(ts[10].type == TokenType::Ident && ts[10].text == QStringLiteral("then"));
        CHECK(ts[11].type == TokenType::String && ts[11].text == QStringLiteral("现代"));
    }

    // 5) 字符串转义
    {
        bool ok = false;
        auto ts = run(QStringLiteral("\"a\\\"b\\n\\t'c'\""), &ok);
        CHECK(ok);
        CHECK(ts[0].type == TokenType::String);
        CHECK(ts[0].text == QStringLiteral("a\"b\n\t'c'"));
    }

    // 6) 运算符与符号
    {
        bool ok = false;
        auto ts = run(QStringLiteral("== != < <= > >= && || ! ? : + - * / % ( ) { } , ;"), &ok);
        CHECK(ok);
        const TokenType expected[] = {
            TokenType::EqEq,   TokenType::NotEq,    TokenType::Lt,     TokenType::Le,
            TokenType::Gt,     TokenType::Ge,       TokenType::AndAnd, TokenType::OrOr,
            TokenType::Bang,   TokenType::Question, TokenType::Colon,  TokenType::Plus,
            TokenType::Minus,  TokenType::Star,     TokenType::Slash,  TokenType::Percent,
            TokenType::LParen, TokenType::RParen,   TokenType::LBrace, TokenType::RBrace,
            TokenType::Comma,  TokenType::Semicolon};
        CHECK(ts.size() == static_cast<int>(sizeof(expected) / sizeof(expected[0])) + 1);
        for (int i = 0; i < static_cast<int>(sizeof(expected) / sizeof(expected[0])); ++i) {
            if (ts[i].type != expected[i]) {
                std::printf("FAIL: token[%d] = %s, expected %s\n", i,
                            qPrintable(token_type_name(ts[i].type)),
                            qPrintable(token_type_name(expected[i])));
                ++g_failures;
            }
            ++g_CHECKs;
        }
    }

    // 7) 数字与负数符号(负号是 Minus, 由 parser 处理)
    {
        bool ok = false;
        auto ts = run(QStringLiteral("2026 -3 0"), &ok);
        CHECK(ok);
        CHECK(ts[0].type == TokenType::Number && ts[0].value == 2026);
        CHECK(ts[1].type == TokenType::Minus);
        CHECK(ts[2].type == TokenType::Number && ts[2].value == 3);
        CHECK(ts[3].type == TokenType::Number && ts[3].value == 0);
    }

    // 8) 未闭合字符串 → 错误
    {
        bool ok = true;
        auto ts = run(QStringLiteral("sort { \"oops }"), &ok);
        CHECK(!ok);
        CHECK(ts.size() >= 1 && ts.back().type == TokenType::Eof);
    }

    // 9) 非法字符 → 错误
    {
        bool ok = true;
        auto ts = run(QStringLiteral("artist @"), &ok);
        CHECK(!ok);
        CHECK(ts.size() >= 1 && ts.back().type == TokenType::Eof);
    }

    // 10) 行/列号定位
    {
        bool ok = false;
        auto ts = run(QStringLiteral("a\nb\nc"), &ok);
        CHECK(ok);
        // a (1,1), newline, b (2,1), newline, c (3,1)
        CHECK(ts[0].line == 1 && ts[0].col == 1);
        CHECK(ts[2].line == 2 && ts[2].col == 1);
        CHECK(ts[4].line == 3 && ts[4].col == 1);
    }

    std::printf("tb_dsl_lexer: %d checks, %d failures\n", g_CHECKs, g_failures);
    return g_failures == 0 ? 0 : 1;
}
