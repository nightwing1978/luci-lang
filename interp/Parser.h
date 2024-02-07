/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#ifndef GUARDIAN_OF_INCLUSION_PARSER_H
#define GUARDIAN_OF_INCLUSION_PARSER_H

#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <ostream>

#include "Token.h"
#include "Lexer.h"
#include "Ast.h"

struct ParserError
{
    size_t lineNumber;
    size_t columnNumber;
    std::string message;
};

std::ostream &operator<<(std::ostream &os, const ParserError &error);

struct Parser
{
    // Pratt parser precedence order
    enum class Precedence : int
    {
        LOWEST,
        ASSIGN,      // =
        EQUALS,      // ==
        LESSGREATER, // > or <
        SUM,         // + or -
        PRODUCT,     // * or /
        POWER,       // **
        PREFIX,      // -x or !x
        CALL,        // func(y)
        INDEX        // a[y]
    };

    Parser(std::unique_ptr<Lexer> lexer);
    std::unique_ptr<ast::Program> parseProgram();

    std::unique_ptr<Lexer> lexer;
    void nextToken();
    void advanceTokens();

    Token curToken;
    Token peekToken;
    Token peek2Token;
    std::vector<std::string> curDoc;
    bool expectPeek(TokenType expectedToken); /*< expect the peek to be as defined and request next token, otherwise error */
    Precedence curPrecedence() const;
    Precedence peekPrecedence() const;

    void peekError(const TokenType &t);
    void parseError(const std::string &msg, const Token &token);
    std::vector<ParserError> errorMsgs;

    std::unique_ptr<ast::ExpressionStatement> parseExpressionStatement();
    std::unique_ptr<ast::Statement> parseStatement();
    std::unique_ptr<ast::BlockStatement> parseBlockStatement();
    std::unique_ptr<ast::LetStatement> parseLetStatement();
    std::unique_ptr<ast::ImportStatement> parseImportStatement();
    std::unique_ptr<ast::ScopeStatement> parseScopeStatement();
    std::unique_ptr<ast::TypeStatement> parseTypeStatement();
    std::unique_ptr<ast::ReturnStatement> parseReturnStatement();
    std::unique_ptr<ast::BreakStatement> parseBreakStatement();
    std::unique_ptr<ast::ContinueStatement> parseContinueStatement();
    std::unique_ptr<ast::TryExceptStatement> parseTryExceptStatement();

    // Pratt parser functions
    typedef std::unique_ptr<ast::Expression> (Parser::*TPrefixParseFn)();
    typedef std::unique_ptr<ast::Expression> (Parser::*TInfixParseFn)(std::unique_ptr<ast::Expression>);

    std::map<TokenType, TPrefixParseFn> prefixParseFns;
    std::map<TokenType, TInfixParseFn> infixParseFns;

    void registerPrefix(TokenType tt, TPrefixParseFn fn);
    void registerInfix(TokenType tt, TInfixParseFn fn);

    std::unique_ptr<ast::Expression> parseIdentifier();
    std::unique_ptr<ast::Expression> parseModuleIdentifier();
    std::unique_ptr<ast::Expression> parseNull();
    std::unique_ptr<ast::Expression> parsePrefixExpression();
    std::unique_ptr<ast::Expression> parseExpression(Precedence precedence);
    std::unique_ptr<ast::Expression> parseIntegerOrRangeLiteral();
    std::unique_ptr<ast::Expression> parseRangeLiteral();
    std::unique_ptr<ast::Expression> parseDoubleLiteral();
    std::unique_ptr<ast::Expression> parseStringLiteral();
    std::unique_ptr<ast::Expression> parseBooleanLiteral();
    std::unique_ptr<ast::Expression> parseFunctionLiteral();
    std::unique_ptr<ast::Expression> parseTypeLiteral();
    std::unique_ptr<ast::Expression> parseArrayLiteral();
    std::unique_ptr<ast::Expression> parseDictOrSetLiteral();
    std::unique_ptr<ast::Expression> parseGroupedExpression();
    std::unique_ptr<ast::Expression> parseIfExpression();
    std::unique_ptr<ast::Expression> parseWhileExpression();
    std::unique_ptr<ast::Expression> parseForExpression();

    std::unique_ptr<ast::Expression> parseInfixExpression(std::unique_ptr<ast::Expression> expression);
    std::unique_ptr<ast::Expression> parseCallExpression(std::unique_ptr<ast::Expression> expression);
    std::unique_ptr<ast::Expression> parseMemberExpression(std::unique_ptr<ast::Expression> expression);
    std::unique_ptr<ast::Expression> parseModuleMemberExpression(std::unique_ptr<ast::Expression> expression);
    std::unique_ptr<ast::Expression> parseIndexExpression(std::unique_ptr<ast::Expression> expression);

    std::pair<std::vector<ast::Identifier>, std::vector<std::unique_ptr<ast::TypeExpression>>> parseFunctionArguments();
    std::vector<std::unique_ptr<ast::Expression>> parseCallArguments();
    std::vector<std::unique_ptr<ast::Expression>> parseExpressionList(const TokenType &endToken);

    /* parsing types has its own set of rules */
    typedef std::unique_ptr<ast::TypeExpression> (Parser::*TPrefixParseTypeFn)();
    typedef std::unique_ptr<ast::TypeExpression> (Parser::*TInfixParseTypeFn)(std::unique_ptr<ast::TypeExpression>);

    std::map<TokenType, TPrefixParseTypeFn> prefixParseTypeFns;
    std::map<TokenType, TInfixParseTypeFn> infixParseTypeFns;
    void registerPrefixType(TokenType tt, TPrefixParseTypeFn fn);
    void registerInfixType(TokenType tt, TInfixParseTypeFn fn);

    std::unique_ptr<ast::TypeExpression> parseTypeIdentifier();
    std::unique_ptr<ast::TypeExpression> parseTypeNull();
    std::unique_ptr<ast::TypeExpression> parseTypeAny();
    std::unique_ptr<ast::TypeExpression> parseTypeAll();
    std::unique_ptr<ast::TypeExpression> parseTypeChoice();
    std::unique_ptr<ast::TypeExpression> parseTypeArray();
    std::unique_ptr<ast::TypeExpression> parseTypeDictionaryOrSet();
    std::unique_ptr<ast::TypeExpression> parseTypeFunction();

    std::unique_ptr<ast::TypeExpression> parseTypeExpression(Precedence precedence);
    std::vector<std::unique_ptr<ast::TypeExpression>> parseTypeExpressionList(const TokenType &endToken);
};

std::unique_ptr<Parser> createParser(std::unique_ptr<Lexer> lexer);
void checkParserErrors(const Parser &parser, size_t expectedErrors);

#endif