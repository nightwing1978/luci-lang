#include "Token.h"

namespace TokenTypeStr
{
    const std::string NOT_SET = "NOT_SET";

    const std::string ILLEGAL = "ILLEGAL";
    const std::string EOF_T = "EOF";

    const std::string IDENT = "IDENT";

    const std::string INT = "INT";
    const std::string DOUBLE = "DOUBLE";

    const std::string STRING = "STRING";

    const std::string ASSIGN = "=";
    const std::string PLUS = "+";
    const std::string MINUS = "-";
    const std::string SLASH = "/";
    const std::string ASTERISK = "*";
    const std::string DOUBLEASTERISK = "**";
    const std::string PERCENT = "%";

    const std::string PLUSASSIGN = "+=";
    const std::string MINUSASSIGN = "-=";
    const std::string SLASHASSIGN = "/=";
    const std::string ASTERISKASSIGN = "*=";

    const std::string EQ = "==";
    const std::string N_EQ = "!=";

    const std::string LT = "<";
    const std::string GT = ">";
    const std::string LTEQ = "<=";
    const std::string GTEQ = ">=";

    const std::string DOUBLEAMPERSAND = "&&";
    const std::string DOUBLEPIPE = "||";

    const std::string BANG = "!";

    const std::string COMMA = ",";
    const std::string SEMICOLON = ";";
    const std::string COLON = ":";
    const std::string DOUBLECOLON = "::";

    const std::string LPAREN = "(";
    const std::string RPAREN = ")";
    const std::string LBRACE = "{";
    const std::string RBRACE = "}";
    const std::string LBRACKET = "[";
    const std::string RBRACKET = "]";

    const std::string FUNCTION = "FN";
    const std::string LET = "LET";
    const std::string IMPORT = "IMPORT";

    const std::string IF = "IF";
    const std::string ELSE = "ELSE";
    const std::string RETURN = "RETURN";

    const std::string TRUE = "TRUE";
    const std::string FALSE = "FALSE";

    const std::string NULL_T = "NULL";

    const std::string WHILE = "WHILE";
    const std::string BREAK = "BREAK";

    const std::string DOUBLEQUOTE = "\"";

    const std::string CONST = "CONST";
    const std::string ALL = "ALL";
    const std::string ANY = "ANY";
    const std::string OPERATOR = "OPERATOR";

    const std::string ARROW = "->";

    const std::string DOT = ".";

    const std::string TYPE = "TYPE";

    const std::string SCOPE = "SCOPE";

    const std::string COMMENT = "COMMENT";
    const std::string DOC = "DOC";

    const std::string FOR = "FOR";
    const std::string IN = "IN";
}

bool Token::operator==(const Token &other) const
{
    return (
        columnNumber == other.columnNumber && lineNumber == other.lineNumber && type == other.type && literal == other.literal);
}

bool Token::operator!=(const Token &other) const
{
    return !(*this == other);
}

std::string toString(TokenType tokenType)
{
    switch (tokenType)
    {
    case TokenType::NOT_SET:
        return TokenTypeStr::NOT_SET;
    case TokenType::ILLEGAL:
        return TokenTypeStr::ILLEGAL;
    case TokenType::EOF_T:
        return TokenTypeStr::EOF_T;
    case TokenType::IDENT:
        return TokenTypeStr::IDENT;
    case TokenType::INT:
        return TokenTypeStr::INT;
    case TokenType::DOUBLE:
        return TokenTypeStr::DOUBLE;
    case TokenType::STRING:
        return TokenTypeStr::STRING;
    case TokenType::ASSIGN:
        return TokenTypeStr::ASSIGN;
    case TokenType::PLUS:
        return TokenTypeStr::PLUS;
    case TokenType::MINUS:
        return TokenTypeStr::MINUS;
    case TokenType::SLASH:
        return TokenTypeStr::SLASH;
    case TokenType::ASTERISK:
        return TokenTypeStr::ASTERISK;
    case TokenType::DOUBLEASTERISK:
        return TokenTypeStr::DOUBLEASTERISK;
    case TokenType::PERCENT:
        return TokenTypeStr::PERCENT;
    case TokenType::PLUSASSIGN:
        return TokenTypeStr::PLUSASSIGN;
    case TokenType::MINUSASSIGN:
        return TokenTypeStr::MINUSASSIGN;
    case TokenType::SLASHASSIGN:
        return TokenTypeStr::SLASHASSIGN;
    case TokenType::ASTERISKASSIGN:
        return TokenTypeStr::ASTERISKASSIGN;
    case TokenType::EQ:
        return TokenTypeStr::EQ;
    case TokenType::N_EQ:
        return TokenTypeStr::N_EQ;
    case TokenType::LT:
        return TokenTypeStr::LT;
    case TokenType::GT:
        return TokenTypeStr::GT;
    case TokenType::LTEQ:
        return TokenTypeStr::LTEQ;
    case TokenType::GTEQ:
        return TokenTypeStr::GTEQ;
    case TokenType::DOUBLEAMPERSAND:
        return TokenTypeStr::DOUBLEAMPERSAND;
    case TokenType::DOUBLEPIPE:
        return TokenTypeStr::DOUBLEPIPE;
    case TokenType::BANG:
        return TokenTypeStr::BANG;
    case TokenType::COMMA:
        return TokenTypeStr::COMMA;
    case TokenType::SEMICOLON:
        return TokenTypeStr::SEMICOLON;
    case TokenType::COLON:
        return TokenTypeStr::COLON;
    case TokenType::DOUBLECOLON:
        return TokenTypeStr::DOUBLECOLON;
    case TokenType::LPAREN:
        return TokenTypeStr::LPAREN;
    case TokenType::RPAREN:
        return TokenTypeStr::RPAREN;
    case TokenType::LBRACE:
        return TokenTypeStr::LBRACE;
    case TokenType::RBRACE:
        return TokenTypeStr::RBRACE;
    case TokenType::LBRACKET:
        return TokenTypeStr::LBRACKET;
    case TokenType::RBRACKET:
        return TokenTypeStr::RBRACKET;
    case TokenType::FUNCTION:
        return TokenTypeStr::FUNCTION;
    case TokenType::LET:
        return TokenTypeStr::LET;
    case TokenType::IF:
        return TokenTypeStr::IF;
    case TokenType::ELSE:
        return TokenTypeStr::ELSE;
    case TokenType::RETURN:
        return TokenTypeStr::RETURN;
    case TokenType::TRUE:
        return TokenTypeStr::TRUE;
    case TokenType::FALSE:
        return TokenTypeStr::FALSE;
    case TokenType::NULL_T:
        return TokenTypeStr::NULL_T;
    case TokenType::WHILE:
        return TokenTypeStr::WHILE;
    case TokenType::BREAK:
        return TokenTypeStr::BREAK;
    case TokenType::DOUBLEQUOTE:
        return TokenTypeStr::DOUBLEQUOTE;
    case TokenType::CONST:
        return TokenTypeStr::CONST;
    case TokenType::ANY:
        return TokenTypeStr::ANY;
    case TokenType::ALL:
        return TokenTypeStr::ALL;
    case TokenType::OPERATOR:
        return TokenTypeStr::OPERATOR;
    case TokenType::ARROW:
        return TokenTypeStr::ARROW;
    case TokenType::DOT:
        return TokenTypeStr::DOT;
    case TokenType::TYPE:
        return TokenTypeStr::TYPE;
    case TokenType::SCOPE:
        return TokenTypeStr::SCOPE;
    case TokenType::COMMENT:
        return TokenTypeStr::COMMENT;
    case TokenType::DOC:
        return TokenTypeStr::DOC;
    case TokenType::FOR:
        return TokenTypeStr::FOR;
    case TokenType::IN:
        return TokenTypeStr::IN;
    case TokenType::IMPORT:
        return TokenTypeStr::IMPORT;
    };
    return TokenTypeStr::NOT_SET;
}
