#include "core/dsl/parser.h"

#include <QStringList>

namespace dsl
{

namespace
{
/// 表达式叶子位置不允许出现的结构/修饰关键字(primary 处遇到即报错)。
const QStringList kBlockKeywords = {
    QStringLiteral("sort"),  QStringLiteral("group"), QStringLiteral("bucket"),
    QStringLiteral("if"),    QStringLiteral("elif"),  QStringLiteral("else"),
    QStringLiteral("then"),  QStringLiteral("asc"),   QStringLiteral("desc"),
    QStringLiteral("nulls"), QStringLiteral("first"), QStringLiteral("last"),
};

/// 中缀比较运算符 → 原语名。
QString cmp_op_name(TokenType t)
{
    switch (t) {
    case TokenType::EqEq:
        return QStringLiteral("==");
    case TokenType::NotEq:
        return QStringLiteral("!=");
    case TokenType::Lt:
        return QStringLiteral("<");
    case TokenType::Le:
        return QStringLiteral("<=");
    case TokenType::Gt:
        return QStringLiteral(">");
    case TokenType::Ge:
        return QStringLiteral(">=");
    default:
        return QString();
    }
}
} // namespace

Parser::Parser(const QVector<Token>& tokens) : ts_(tokens) {}

bool Parser::accept(TokenType t)
{
    if (at(t)) {
        ++pos_;
        return true;
    }
    return false;
}

bool Parser::accept_ident(const QString& name)
{
    if (at_ident(name)) {
        ++pos_;
        return true;
    }
    return false;
}

bool Parser::expect(TokenType t, const QString& what)
{
    if (at(t)) {
        ++pos_;
        return true;
    }
    error(QStringLiteral("expected %1, got %2").arg(what, token_type_name(cur().type)), cur());
    return false;
}

bool Parser::expect_ident(const QString& name, const QString& what)
{
    if (at_ident(name)) {
        ++pos_;
        return true;
    }
    error(QStringLiteral("expected %1, got '%2'").arg(what, cur().text), cur());
    return false;
}

void Parser::error(const QString& msg, const Token& at)
{
    if (prog_.ok) {
        prog_.ok         = false;
        prog_.error      = QStringLiteral("%1:%2: %3").arg(at.line).arg(at.col).arg(msg);
        prog_.error_line = at.line;
        prog_.error_col  = at.col;
    }
}

bool Parser::skip_seps()
{
    bool any = false;
    while (at(TokenType::Semicolon) || at(TokenType::Newline)) {
        ++pos_;
        any = true;
    }
    return any;
}

Program Parser::parse()
{
    skip_seps();
    while (!at(TokenType::Eof)) {
        if (at_ident(QStringLiteral("sort"))) {
            if (prog_.has_sort) {
                error(QStringLiteral("duplicate 'sort' section"), cur());
                break;
            }
            if (!parse_sort())
                break;
        } else if (at_ident(QStringLiteral("group"))) {
            if (prog_.has_group) {
                error(QStringLiteral("duplicate 'group' section"), cur());
                break;
            }
            if (!parse_group())
                break;
        } else if (at_ident(QStringLiteral("bucket"))) {
            if (prog_.has_bucket) {
                error(QStringLiteral("duplicate 'bucket' section"), cur());
                break;
            }
            if (!parse_bucket())
                break;
        } else {
            error(QStringLiteral("expected 'sort', 'group' or 'bucket', got '%1'").arg(cur().text),
                  cur());
            break;
        }
        skip_seps();
    }
    return prog_;
}

bool Parser::parse_sort()
{
    prog_.has_sort = true;
    ++pos_; // 'sort'
    if (!expect(TokenType::LBrace, QStringLiteral("'{' after 'sort'")))
        return false;
    skip_seps();
    while (!at(TokenType::RBrace)) {
        if (at(TokenType::Eof)) {
            error(QStringLiteral("unterminated 'sort' block, missing '}'"), cur());
            return false;
        }
        if (!at(TokenType::Ident)) {
            error(QStringLiteral("expected property name in sort item, got %1")
                      .arg(token_type_name(cur().type)),
                  cur());
            return false;
        }

        SortItem item;
        item.property = cur().text;
        item.line     = cur().line;
        item.col      = cur().col;
        ++pos_;

        if (at_ident(QStringLiteral("asc")) || at_ident(QStringLiteral("desc"))) {
            item.desc = (cur().text == QStringLiteral("desc"));
            ++pos_;
        }
        if (at_ident(QStringLiteral("nulls"))) {
            ++pos_;
            if (accept_ident(QStringLiteral("first"))) {
                item.nulls = SortItem::Nulls::First;
            } else if (accept_ident(QStringLiteral("last"))) {
                item.nulls = SortItem::Nulls::Last;
            } else {
                error(QStringLiteral("expected 'first' or 'last' after 'nulls'"), cur());
                return false;
            }
        }
        prog_.sort.append(item);
        skip_seps();
    }
    ++pos_; // '}'
    return true;
}

bool Parser::parse_group()
{
    if (prog_.has_bucket) {
        error(QStringLiteral("'group' is mutually exclusive with 'bucket'"), cur());
        return false;
    }
    prog_.has_group = true;
    ++pos_; // 'group'
    if (!expect(TokenType::LBrace, QStringLiteral("'{' after 'group'")))
        return false;
    skip_seps();
    while (!at(TokenType::RBrace)) {
        if (at(TokenType::Eof)) {
            error(QStringLiteral("unterminated 'group' block, missing '}'"), cur());
            return false;
        }
        if (!at(TokenType::Ident)) {
            error(QStringLiteral("expected property name in group item, got %1")
                      .arg(token_type_name(cur().type)),
                  cur());
            return false;
        }

        GroupItem item;
        item.property = cur().text;
        item.line     = cur().line;
        item.col      = cur().col;
        ++pos_;

        if (at_ident(QStringLiteral("asc")) || at_ident(QStringLiteral("desc"))) {
            item.desc = (cur().text == QStringLiteral("desc"));
            ++pos_;
        }
        prog_.group.append(item);
        skip_seps();
    }
    ++pos_; // '}'
    return true;
}

bool Parser::parse_bucket()
{
    if (prog_.has_group) {
        error(QStringLiteral("'bucket' is mutually exclusive with 'group'"), cur());
        return false;
    }
    prog_.has_bucket = true;
    ++pos_; // 'bucket'
    if (!expect(TokenType::LBrace, QStringLiteral("'{' after 'bucket'")))
        return false;
    skip_seps();

    bool first     = true;
    bool seen_else = false;
    while (!at(TokenType::RBrace)) {
        if (at(TokenType::Eof)) {
            error(QStringLiteral("unterminated 'bucket' block, missing '}'"), cur());
            return false;
        }

        BucketBranch b;
        b.line = cur().line;
        b.col  = cur().col;

        if (at_ident(QStringLiteral("if"))) {
            if (!first) {
                error(QStringLiteral("'if' must be the first branch; use 'elif'"), cur());
                return false;
            }
            b.kind = BucketBranch::Kind::If;
            ++pos_;
            b.cond = parse_expr();
            if (!prog_.ok)
                return false;
            if (!expect_ident(QStringLiteral("then"), QStringLiteral("'then' after if condition")))
                return false;
            b.key = parse_key_expr();
            if (!prog_.ok)
                return false;
            first = false;
        } else if (at_ident(QStringLiteral("elif"))) {
            if (first) {
                error(QStringLiteral("'elif' without preceding 'if'"), cur());
                return false;
            }
            if (seen_else) {
                error(QStringLiteral("'elif' after 'else'"), cur());
                return false;
            }
            b.kind = BucketBranch::Kind::Elif;
            ++pos_;
            b.cond = parse_expr();
            if (!prog_.ok)
                return false;
            if (!expect_ident(QStringLiteral("then"),
                              QStringLiteral("'then' after elif condition")))
                return false;
            b.key = parse_key_expr();
            if (!prog_.ok)
                return false;
        } else if (at_ident(QStringLiteral("else"))) {
            if (first) {
                error(QStringLiteral("'else' without preceding 'if'"), cur());
                return false;
            }
            if (seen_else) {
                error(QStringLiteral("duplicate 'else'"), cur());
                return false;
            }
            b.kind = BucketBranch::Kind::Else;
            ++pos_;
            b.key = parse_key_expr();
            if (!prog_.ok)
                return false;
            seen_else = true;
        } else {
            error(QStringLiteral("expected 'if', 'elif' or 'else' in bucket block, got '%1'")
                      .arg(cur().text),
                  cur());
            return false;
        }
        prog_.bucket.append(b);
        skip_seps();
    }
    ++pos_; // '}'
    return true;
}

Node Parser::parse_key_expr()
{
    if (at(TokenType::String)) {
        Node n = Node::make_string(cur().text, cur().line, cur().col);
        ++pos_;
        return n;
    }
    return parse_expr();
}

// ============================================================================
// 表达式(递归下降)
// ============================================================================

Node Parser::parse_expr()
{
    return parse_ternary();
}

Node Parser::parse_ternary()
{
    Node cond = parse_or();
    if (!prog_.ok)
        return Node{};
    if (accept(TokenType::Question)) {
        Node then_branch = parse_expr();
        if (!prog_.ok)
            return Node{};
        if (!expect(TokenType::Colon, QStringLiteral("':' in ternary expression")))
            return Node{};
        Node else_branch = parse_expr();
        if (!prog_.ok)
            return Node{};
        return Node::make_primitive(QStringLiteral("ternary"), {cond, then_branch, else_branch},
                                    cond.line, cond.col);
    }
    return cond;
}

Node Parser::parse_or()
{
    Node left = parse_and();
    while (prog_.ok && (at(TokenType::OrOr) || at_ident(QStringLiteral("or")))) {
        ++pos_;
        Node right = parse_and();
        if (!prog_.ok)
            return Node{};
        left = Node::make_primitive(QStringLiteral("or"), {left, right}, left.line, left.col);
    }
    return left;
}

Node Parser::parse_and()
{
    Node left = parse_not();
    while (prog_.ok && (at(TokenType::AndAnd) || at_ident(QStringLiteral("and")))) {
        ++pos_;
        Node right = parse_not();
        if (!prog_.ok)
            return Node{};
        left = Node::make_primitive(QStringLiteral("and"), {left, right}, left.line, left.col);
    }
    return left;
}

Node Parser::parse_not()
{
    if (at(TokenType::Bang) || at_ident(QStringLiteral("not"))) {
        const Token tok = cur();
        ++pos_;
        Node operand = parse_not();
        if (!prog_.ok)
            return Node{};
        return Node::make_primitive(QStringLiteral("not"), {operand}, tok.line, tok.col);
    }
    return parse_cmp();
}

Node Parser::parse_cmp()
{
    Node left = parse_add();
    while (prog_.ok) {
        const QString op = cmp_op_name(cur().type);
        if (op.isEmpty())
            break;
        const Token tok = cur();
        ++pos_;
        Node right = parse_add();
        if (!prog_.ok)
            return Node{};
        left = Node::make_primitive(op, {left, right}, tok.line, tok.col);
    }
    return left;
}

Node Parser::parse_add()
{
    Node left = parse_mul();
    while (prog_.ok && (at(TokenType::Plus) || at(TokenType::Minus))) {
        const QString op = at(TokenType::Plus) ? QStringLiteral("+") : QStringLiteral("-");
        const Token tok  = cur();
        ++pos_;
        Node right = parse_mul();
        if (!prog_.ok)
            return Node{};
        left = Node::make_primitive(op, {left, right}, tok.line, tok.col);
    }
    return left;
}

Node Parser::parse_mul()
{
    Node left = parse_unary();
    while (prog_.ok && (at(TokenType::Star) || at(TokenType::Slash) || at(TokenType::Percent))) {
        QString op;
        if (at(TokenType::Star))
            op = QStringLiteral("*");
        else if (at(TokenType::Slash))
            op = QStringLiteral("/");
        else
            op = QStringLiteral("%");
        const Token tok = cur();
        ++pos_;
        Node right = parse_unary();
        if (!prog_.ok)
            return Node{};
        left = Node::make_primitive(op, {left, right}, tok.line, tok.col);
    }
    return left;
}

Node Parser::parse_unary()
{
    if (at(TokenType::Minus)) {
        const Token tok = cur();
        ++pos_;
        Node operand = parse_unary();
        if (!prog_.ok)
            return Node{};
        return Node::make_primitive(QStringLiteral("neg"), {operand}, tok.line, tok.col);
    }
    if (at(TokenType::Plus)) {
        ++pos_;
        return parse_unary();
    }
    return parse_primary();
}

Node Parser::parse_primary()
{
    const Token tok = cur();

    switch (tok.type) {
    case TokenType::Number: {
        ++pos_;
        return Node::make_number(tok.value, tok.line, tok.col);
    }
    case TokenType::String: {
        ++pos_;
        return Node::make_string(tok.text, tok.line, tok.col);
    }
    case TokenType::LParen: {
        ++pos_;
        Node inner = parse_expr();
        if (!prog_.ok)
            return Node{};
        if (!expect(TokenType::RParen, QStringLiteral("')' to close '('")))
            return Node{};
        return inner;
    }
    case TokenType::Ident: {
        const QString name = tok.text;
        if (name == QStringLiteral("true")) {
            ++pos_;
            return Node::make_bool(true, tok.line, tok.col);
        }
        if (name == QStringLiteral("false")) {
            ++pos_;
            return Node::make_bool(false, tok.line, tok.col);
        }
        if (name == QStringLiteral("null")) {
            ++pos_;
            return Node::make_null(tok.line, tok.col);
        }
        if (name == QStringLiteral("and") || name == QStringLiteral("or") ||
            name == QStringLiteral("not") || kBlockKeywords.contains(name)) {
            error(QStringLiteral("unexpected keyword '%1'").arg(name), tok);
            return Node{};
        }
        // 函数调用: ident '(' args ')'
        if (peek(1).type == TokenType::LParen) {
            ++pos_; // ident
            ++pos_; // '('
            QVector<Node> args;
            if (!at(TokenType::RParen)) {
                while (true) {
                    args.append(parse_expr());
                    if (!prog_.ok)
                        return Node{};
                    if (!accept(TokenType::Comma))
                        break;
                }
            }
            if (!expect(TokenType::RParen, QStringLiteral("')' after function arguments")))
                return Node{};
            return Node::make_primitive(name, args, tok.line, tok.col);
        }
        ++pos_;
        return Node::make_property(name, tok.line, tok.col);
    }
    default:
        error(QStringLiteral("expected expression, got %1").arg(token_type_name(tok.type)), tok);
        return Node{};
    }
}

} // namespace dsl
