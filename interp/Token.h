#ifndef GUARDIAN_OF_INCLUSION_TOKEN_H
#define GUARDIAN_OF_INCLUSION_TOKEN_H

#include <string>
#include <memory>

enum class TokenType
{
    NOT_SET = -1,
    ILLEGAL = 0,
    EOF_T = 1,
    IDENT = 2,
    INT = 3,
    DOUBLE = 4,
    STRING = 5,

    ASSIGN = 6,
    PLUS = 7,
    MINUS = 8,
    SLASH = 9,
    ASTERISK = 10,
    DOUBLEASTERISK = 11,
    PERCENT = 12,

    PLUSASSIGN = 13,
    MINUSASSIGN = 14,
    SLASHASSIGN = 15,
    ASTERISKASSIGN = 16,

    EQ = 17,
    N_EQ = 18,

    LT = 19,
    GT = 20,
    LTEQ = 21,
    GTEQ = 22,

    DOUBLEAMPERSAND = 23,
    DOUBLEPIPE = 24,

    BANG = 25,

    COMMA = 26,
    SEMICOLON = 27,
    COLON = 28,
    DOUBLECOLON = 29,

    LPAREN = 30,
    RPAREN = 31,
    LBRACE = 32,
    RBRACE = 33,
    LBRACKET = 34,
    RBRACKET = 35,

    FUNCTION = 36,
    LET = 37,

    IF = 38,
    ELSE = 39,
    RETURN = 40,

    TRUE = 41,
    FALSE = 42,

    NULL_T = 43,

    WHILE = 44,
    BREAK = 45,

    DOUBLEQUOTE = 46,

    CONST = 47,
    ANY = 48,
    ALL = 49,
    OPERATOR = 50,

    ARROW = 51,

    DOT = 52,

    TYPE = 53,

    SCOPE = 54,

    COMMENT = 55,
    DOC = 56,

    FOR = 57,
    IN = 58,

    IMPORT = 59,

    TRY = 60,
    EXCEPT = 61,
};

std::string toString(TokenType tokenType);

struct Token
{
    std::shared_ptr<std::string> fileName;
    TokenType type = TokenType::NOT_SET;
    std::string literal;
    size_t lineNumber = 0;
    size_t columnNumber = 0;

    bool operator==(const Token &other) const;
    bool operator!=(const Token &other) const;
};

#endif