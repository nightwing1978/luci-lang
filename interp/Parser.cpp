/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Parser.h"
#include "Util.h"
#include <sstream>
#include <map>
#include <iostream> // for the output to cerr

namespace
{
    std::map<TokenType, Parser::Precedence> precedences =
        {
            {TokenType::ASSIGN, Parser::Precedence::ASSIGN},
            {TokenType::PLUSASSIGN, Parser::Precedence::ASSIGN},
            {TokenType::MINUSASSIGN, Parser::Precedence::ASSIGN},
            {TokenType::SLASHASSIGN, Parser::Precedence::ASSIGN},
            {TokenType::ASTERISKASSIGN, Parser::Precedence::ASSIGN},

            {TokenType::EQ, Parser::Precedence::EQUALS},
            {TokenType::N_EQ, Parser::Precedence::EQUALS},
            {TokenType::LTEQ, Parser::Precedence::EQUALS},
            {TokenType::GTEQ, Parser::Precedence::EQUALS},
            {TokenType::LT, Parser::Precedence::LESSGREATER},
            {TokenType::GT, Parser::Precedence::LESSGREATER},
            {TokenType::DOUBLEAMPERSAND, Parser::Precedence::LESSGREATER},
            {TokenType::DOUBLEPIPE, Parser::Precedence::LESSGREATER},
            {TokenType::PLUS, Parser::Precedence::SUM},
            {TokenType::MINUS, Parser::Precedence::SUM},
            {TokenType::SLASH, Parser::Precedence::PRODUCT},
            {TokenType::ASTERISK, Parser::Precedence::PRODUCT},
            {TokenType::PERCENT, Parser::Precedence::PRODUCT},
            {TokenType::DOUBLEASTERISK, Parser::Precedence::POWER},

            {TokenType::LPAREN, Parser::Precedence::CALL},
            {TokenType::DOT, Parser::Precedence::CALL},
            {TokenType::DOUBLECOLON, Parser::Precedence::CALL},
            {TokenType::LBRACKET, Parser::Precedence::INDEX},
    };

    std::string extractAndClearCurrentDoc(std::vector<std::string> &docStrings)
    {
        std::vector<std::string> docLines;

        size_t charactersToIgnore = 2; // always ignore the /! to begin with, then take the first line as guidance of the maximum of
                                       // white space to eat

        /*
         *       /! blabla
         *       /!   more blabla
         *       ==>
         *       blabla
         *         more blabla
         *
         *       /!   blabla
         *       /! more blabla
         *       ==>
         *       blabla
         *       more blabla
         */

        if (!docStrings.empty())
        {
            for (size_t i = 2; i < docStrings.front().size(); ++i)
            {
                if (isblank(docStrings.front()[i]))
                {
                    charactersToIgnore = i + 1;
                }
                else
                    break;
            }
        }

        for (const auto &line : docStrings)
        {
            std::string processedLine = line.substr(charactersToIgnore);
            docLines.push_back(processedLine);
        }

        docStrings.clear();
        return util::join(docLines, "\n");
    }
}

std::ostream &operator<<(std::ostream &os, const ParserError &error)
{
    os << "Line " << error.lineNumber << ", column " << error.columnNumber << ": " << error.message;
    return os;
}

std::unique_ptr<Parser> createParser(std::unique_ptr<Lexer> lexer)
{
    return std::make_unique<Parser>(std::move(lexer));
}

Parser::Parser(std::unique_ptr<Lexer> iLexer)
    : lexer(std::move(iLexer))
{
    registerPrefix(TokenType::IDENT, &Parser::parseIdentifier);
    registerPrefix(TokenType::NULL_T, &Parser::parseNull);
    registerPrefix(TokenType::INT, &Parser::parseIntegerLiteral);
    registerPrefix(TokenType::DOUBLE, &Parser::parseDoubleLiteral);
    registerPrefix(TokenType::STRING, &Parser::parseStringLiteral);
    registerPrefix(TokenType::FALSE, &Parser::parseBooleanLiteral);
    registerPrefix(TokenType::TRUE, &Parser::parseBooleanLiteral);
    registerPrefix(TokenType::BANG, &Parser::parsePrefixExpression);
    registerPrefix(TokenType::MINUS, &Parser::parsePrefixExpression);
    registerPrefix(TokenType::LPAREN, &Parser::parseGroupedExpression);
    registerPrefix(TokenType::IF, &Parser::parseIfExpression);
    registerPrefix(TokenType::FUNCTION, &Parser::parseFunctionLiteral);
    registerPrefix(TokenType::TYPE, &Parser::parseTypeLiteral);
    registerPrefix(TokenType::WHILE, &Parser::parseWhileExpression);
    registerPrefix(TokenType::FOR, &Parser::parseForExpression);
    registerPrefix(TokenType::LBRACKET, &Parser::parseArrayLiteral);
    registerPrefix(TokenType::LBRACE, &Parser::parseDictOrSetLiteral);

    registerInfix(TokenType::PLUS, &Parser::parseInfixExpression);
    registerInfix(TokenType::MINUS, &Parser::parseInfixExpression);
    registerInfix(TokenType::SLASH, &Parser::parseInfixExpression);
    registerInfix(TokenType::ASTERISK, &Parser::parseInfixExpression);
    registerInfix(TokenType::DOUBLEASTERISK, &Parser::parseInfixExpression);
    registerInfix(TokenType::PERCENT, &Parser::parseInfixExpression);
    registerInfix(TokenType::EQ, &Parser::parseInfixExpression);
    registerInfix(TokenType::N_EQ, &Parser::parseInfixExpression);
    registerInfix(TokenType::LTEQ, &Parser::parseInfixExpression);
    registerInfix(TokenType::GTEQ, &Parser::parseInfixExpression);
    registerInfix(TokenType::LT, &Parser::parseInfixExpression);
    registerInfix(TokenType::GT, &Parser::parseInfixExpression);
    registerInfix(TokenType::ASSIGN, &Parser::parseInfixExpression);
    registerInfix(TokenType::PLUSASSIGN, &Parser::parseInfixExpression);
    registerInfix(TokenType::MINUSASSIGN, &Parser::parseInfixExpression);
    registerInfix(TokenType::SLASHASSIGN, &Parser::parseInfixExpression);
    registerInfix(TokenType::ASTERISKASSIGN, &Parser::parseInfixExpression);

    registerInfix(TokenType::DOUBLEPIPE, &Parser::parseInfixExpression);
    registerInfix(TokenType::DOUBLEAMPERSAND, &Parser::parseInfixExpression);

    registerInfix(TokenType::LPAREN, &Parser::parseCallExpression);
    registerInfix(TokenType::DOT, &Parser::parseMemberExpression);
    registerInfix(TokenType::DOUBLECOLON, &Parser::parseModuleMemberExpression);
    registerInfix(TokenType::LBRACKET, &Parser::parseIndexExpression);

    ////////////////////

    registerPrefixType(TokenType::IDENT, &Parser::parseTypeIdentifier);
    registerPrefixType(TokenType::NULL_T, &Parser::parseTypeNull);
    registerPrefixType(TokenType::ALL, &Parser::parseTypeAll);
    registerPrefixType(TokenType::ANY, &Parser::parseTypeAny);
    registerPrefixType(TokenType::LT, &Parser::parseTypeChoice);
    registerPrefixType(TokenType::LBRACKET, &Parser::parseTypeArray);
    registerPrefixType(TokenType::LBRACE, &Parser::parseTypeDictionaryOrSet);
    registerPrefixType(TokenType::FUNCTION, &Parser::parseTypeFunction);

    ////////////////////

    nextToken(); // initialize the parser by filling up peekToken
    nextToken(); // initialize the parser by filling up curToken
}

void Parser::registerPrefix(TokenType tt, TPrefixParseFn fn)
{
    prefixParseFns[tt] = fn;
}

void Parser::registerInfix(TokenType tt, TInfixParseFn fn)
{
    infixParseFns[tt] = fn;
}

void Parser::registerPrefixType(TokenType tt, TPrefixParseTypeFn fn)
{
    prefixParseTypeFns[tt] = fn;
}

void Parser::registerInfixType(TokenType tt, TInfixParseTypeFn fn)
{
    infixParseTypeFns[tt] = fn;
}

bool Parser::expectPeek(TokenType expectedToken)
{
    if (peekToken.type == expectedToken)
    {
        nextToken();
        return true;
    }

    peekError(expectedToken);
    return false;
}

Parser::Precedence Parser::curPrecedence() const
{
    auto precedenceIt = precedences.find(curToken.type);
    if (precedenceIt == precedences.end())
        return Parser::Precedence::LOWEST;

    return precedenceIt->second;
}

Parser::Precedence Parser::peekPrecedence() const
{
    auto precedenceIt = precedences.find(peekToken.type);
    if (precedenceIt == precedences.end())
        return Parser::Precedence::LOWEST;

    return precedenceIt->second;
}

void Parser::nextToken()
{
    curToken = peekToken;
    peekToken = ::nextToken(*lexer);
}

void Parser::advanceTokens()
{
    nextToken();
}

std::unique_ptr<ast::Program> Parser::parseProgram()
{
    std::unique_ptr<ast::Program> program = std::make_unique<ast::Program>();

    while (curToken.type != TokenType::EOF_T)
    {
        if (curToken.type == TokenType::ILLEGAL)
        {
            parseError("invalid token " + curToken.literal, curToken);
            return program;
        }

        std::unique_ptr<ast::Statement> statement;

        if (curToken.type == TokenType::LET)
            statement = parseLetStatement();
        else if (curToken.type == TokenType::IMPORT)
            statement = parseImportStatement();
        else if (curToken.type == TokenType::TRY)
            statement = parseTryExceptStatement();
        else if (curToken.type == TokenType::RETURN)
            statement = parseReturnStatement();
        else if (curToken.type == TokenType::BREAK)
            statement = parseBreakStatement();
        else if (curToken.type == TokenType::SCOPE)
            statement = parseScopeStatement();
        else if (curToken.type == TokenType::COMMENT)
        {
            // no nothing
        }
        else if (curToken.type == TokenType::DOC)
        {
            curDoc.push_back(curToken.literal);
        }
        else
            statement = parseExpressionStatement();

        if (statement)
            program->statements.push_back(std::move(statement));

        advanceTokens();
    }

    return program;
}

void Parser::peekError(const TokenType &t)
{
    std::stringstream ss;
    ss << "Expected next token to be " << toString(t) << ", but got " << toString(peekToken.type);
    parseError(ss.str(), peekToken);
}

void Parser::parseError(const std::string &msg, const Token &token)
{
    errorMsgs.push_back(ParserError({token.lineNumber, token.columnNumber, msg}));
}

std::unique_ptr<ast::Expression> Parser::parsePrefixExpression()
{
    std::unique_ptr<ast::PrefixExpression> prefixExpr = std::make_unique<ast::PrefixExpression>();
    prefixExpr->token = curToken;
    prefixExpr->operator_t = curToken;

    advanceTokens();

    prefixExpr->right = std::move(parseExpression(Precedence::PREFIX));
    return prefixExpr;
}

std::unique_ptr<ast::Expression> Parser::parseInfixExpression(std::unique_ptr<ast::Expression> leftExpression)
{
    std::unique_ptr<ast::InfixExpression> infixExpr = std::make_unique<ast::InfixExpression>();
    infixExpr->token = curToken;
    infixExpr->operator_t = curToken;
    infixExpr->left = std::move(leftExpression);

    auto precedence = curPrecedence();
    advanceTokens();
    infixExpr->right = std::move(parseExpression(precedence));

    return infixExpr;
}

std::unique_ptr<ast::Expression> Parser::parseMemberExpression(std::unique_ptr<ast::Expression> expression)
{
    std::unique_ptr<ast::MemberExpression> invokeExpr = std::make_unique<ast::MemberExpression>();
    invokeExpr->token = curToken;
    invokeExpr->expr = std::move(expression);
    if (!expectPeek(TokenType::IDENT))
        return nullptr;
    auto identifier = parseIdentifier();
    invokeExpr->value = *static_cast<ast::Identifier *>(identifier.get());
    return invokeExpr;
}

std::unique_ptr<ast::Expression> Parser::parseModuleMemberExpression(std::unique_ptr<ast::Expression> expression)
{
    std::unique_ptr<ast::ModuleMemberExpression> invokeExpr = std::make_unique<ast::ModuleMemberExpression>();
    invokeExpr->token = curToken;
    invokeExpr->expr = std::move(expression);
    if (!expectPeek(TokenType::IDENT))
        return nullptr;
    auto identifier = parseIdentifier();
    invokeExpr->value = *static_cast<ast::Identifier *>(identifier.get());
    return invokeExpr;
}

std::unique_ptr<ast::Expression> Parser::parseCallExpression(std::unique_ptr<ast::Expression> expression)
{
    std::unique_ptr<ast::CallExpression> callExpr = std::make_unique<ast::CallExpression>();
    callExpr->token = curToken;
    callExpr->function = std::move(expression);
    callExpr->arguments = parseCallArguments();
    return callExpr;
}

std::unique_ptr<ast::Expression> Parser::parseIndexExpression(std::unique_ptr<ast::Expression> expression)
{
    std::unique_ptr<ast::IndexExpression> indexExpr = std::make_unique<ast::IndexExpression>();
    indexExpr->token = curToken;
    indexExpr->expression = std::move(expression);
    advanceTokens();
    indexExpr->index = parseExpression(Precedence::LOWEST);
    if (!expectPeek(TokenType::RBRACKET))
        return nullptr;

    return indexExpr;
}

std::unique_ptr<ast::Expression> Parser::parseIdentifier()
{
    std::unique_ptr<ast::Identifier> identifier = std::make_unique<ast::Identifier>();
    if (curToken.type != TokenType::IDENT)
        return nullptr;
    identifier->token = curToken;
    identifier->value = curToken.literal;
    return identifier;
}

std::unique_ptr<ast::Expression> Parser::parseModuleIdentifier()
{
    std::unique_ptr<ast::ModuleIdentifier> identifier = std::make_unique<ast::ModuleIdentifier>();
    identifier->token = curToken;
    identifier->path.push_back(curToken.literal);
    while (peekToken.type == TokenType::DOUBLECOLON)
    {
        advanceTokens();
        if (!expectPeek(TokenType::IDENT))
            return nullptr;
        identifier->path.push_back(curToken.literal);
    };

    return identifier;
}

std::unique_ptr<ast::Expression> Parser::parseNull()
{
    std::unique_ptr<ast::NullLiteral> nullLiteral = std::make_unique<ast::NullLiteral>();
    nullLiteral->token = curToken;
    return nullLiteral;
}

std::unique_ptr<ast::Expression> Parser::parseIntegerLiteral()
{
    std::unique_ptr<ast::IntegerLiteral> integerLiteral = std::make_unique<ast::IntegerLiteral>();
    integerLiteral->token = curToken;
    char *pEnd = 0;
    integerLiteral->value = std::strtoll(integerLiteral->token.literal.c_str(), &pEnd, 10);
    return integerLiteral;
}

std::unique_ptr<ast::Expression> Parser::parseDoubleLiteral()
{
    std::unique_ptr<ast::DoubleLiteral> doubleLiteral = std::make_unique<ast::DoubleLiteral>();
    doubleLiteral->token = curToken;
    doubleLiteral->value = atof(doubleLiteral->token.literal.c_str());
    return doubleLiteral;
}

std::unique_ptr<ast::Expression> Parser::parseStringLiteral()
{
    std::unique_ptr<ast::StringLiteral> stringLiteral = std::make_unique<ast::StringLiteral>();
    stringLiteral->token = curToken;
    stringLiteral->value = stringLiteral->token.literal.substr(1, stringLiteral->token.literal.size() - 2);
    return stringLiteral;
}

std::unique_ptr<ast::Expression> Parser::parseBooleanLiteral()
{
    std::unique_ptr<ast::BooleanLiteral> boolLiteral = std::make_unique<ast::BooleanLiteral>();
    boolLiteral->token = curToken;
    boolLiteral->value = boolLiteral->token.type == TokenType::TRUE ? true : false;
    return boolLiteral;
}

std::unique_ptr<ast::Expression> Parser::parseFunctionLiteral()
{
    std::unique_ptr<ast::FunctionLiteral> funcLiteral = std::make_unique<ast::FunctionLiteral>();
    funcLiteral->doc = extractAndClearCurrentDoc(curDoc);
    funcLiteral->token = curToken;
    funcLiteral->value = curToken.literal;

    if (!expectPeek(TokenType::LPAREN))
        return nullptr;

    auto argumentsAndTypes = parseFunctionArguments();
    funcLiteral->arguments = argumentsAndTypes.first;
    funcLiteral->argumentTypes = std::move(argumentsAndTypes.second);

    if (peekToken.type == TokenType::ARROW)
    {
        advanceTokens(); // eat the arrow
        advanceTokens(); // bring to the identifier or type expression
        auto type = parseTypeExpression(Precedence::LOWEST);
        funcLiteral->returnType = std::move(type);
    }

    if (!expectPeek(TokenType::LBRACE))
        return nullptr;

    funcLiteral->body = parseBlockStatement();

    return funcLiteral;
}

std::unique_ptr<ast::Expression> Parser::parseTypeLiteral()
{
    std::unique_ptr<ast::TypeLiteral> typeLiteral = std::make_unique<ast::TypeLiteral>();
    typeLiteral->token = curToken;
    typeLiteral->doc = extractAndClearCurrentDoc(curDoc);

    advanceTokens();

    auto typeIdentifier = parseIdentifier();
    if (!typeIdentifier)
        return nullptr;

    typeLiteral->name = static_cast<ast::Identifier *>(typeIdentifier.get())->value;

    if (!expectPeek(TokenType::LBRACE))
        return nullptr;

    advanceTokens();

    while ((curToken.type != TokenType::RBRACE) && (curToken.type != TokenType::EOF_T))
    {
        std::unique_ptr<ast::TypeStatement> statement = parseTypeStatement();
        if (statement)
            typeLiteral->definitions.push_back(std::move(statement));

        advanceTokens();
    };

    return typeLiteral;
}

std::unique_ptr<ast::Expression> Parser::parseArrayLiteral()
{
    std::unique_ptr<ast::ArrayLiteral> arrayLiteral = std::make_unique<ast::ArrayLiteral>();
    arrayLiteral->token = curToken;
    arrayLiteral->elements = parseExpressionList(TokenType::RBRACKET);
    arrayLiteral->token.literal = arrayLiteral->text();

    //
    // verify if the array can be re-typed to an ArrayDouble or ArrayComplex
    // the eventual decision on the type is handled by the evaluator as during parsing
    // it is difficult to get all the necessary context information
    // let a = [0.0,1.0];                   // should give regular array
    // let a : [double] = [0.0,1.0];        // should give a ArrayDouble
    //

    if (!arrayLiteral->elements.empty())
    {
        bool allDoubles = true;
        for (const auto &element : arrayLiteral->elements)
        {
            if (element->type != ast::NodeType::DoubleLiteral)
            {
                allDoubles = false;
                break;
            }
        }

        if (allDoubles)
        {
            std::unique_ptr<ast::ArrayDoubleLiteral> arrayDoubleLiteral = std::make_unique<ast::ArrayDoubleLiteral>();
            arrayDoubleLiteral->token = curToken;
            arrayDoubleLiteral->elements.reserve(arrayLiteral->elements.size());
            for (const auto &element : arrayLiteral->elements)
                arrayDoubleLiteral->elements.push_back(static_cast<ast::DoubleLiteral *>(element.get())->value);
            return arrayDoubleLiteral;
        }

        bool allComplex = true;
        for (const auto &element : arrayLiteral->elements)
        {
            if (element->type != ast::NodeType::ComplexLiteral)
            {
                allComplex = false;
                break;
            }
        }

        if (allComplex)
        {
            std::unique_ptr<ast::ArrayComplexLiteral> arrayComplexLiteral = std::make_unique<ast::ArrayComplexLiteral>();
            arrayComplexLiteral->token = curToken;
            arrayComplexLiteral->elements.reserve(arrayLiteral->elements.size());
            for (const auto &element : arrayLiteral->elements)
                arrayComplexLiteral->elements.push_back(static_cast<ast::ComplexLiteral *>(element.get())->value);
            return arrayComplexLiteral;
        }
    }

    return arrayLiteral;
}

namespace
{
    enum class DictSetParsingState
    {
        Undecided = -1,
        Dict = 0,
        Set = 1,
    };
}

std::unique_ptr<ast::Expression> Parser::parseDictOrSetLiteral()
{
    std::unique_ptr<ast::DictLiteral> dictLiteral = std::make_unique<ast::DictLiteral>();
    std::unique_ptr<ast::SetLiteral> setLiteral = std::make_unique<ast::SetLiteral>();
    dictLiteral->token = curToken;
    setLiteral->token = curToken;

    DictSetParsingState parseState = DictSetParsingState::Undecided;

    while (peekToken.type != TokenType::RBRACE && peekToken.type != TokenType::EOF_T)
    {
        advanceTokens();
        auto key = parseExpression(Precedence::LOWEST);

        if (parseState == DictSetParsingState::Undecided)
        {
            if (peekToken.type == TokenType::COLON)
                parseState = DictSetParsingState::Dict;
            else
                parseState = DictSetParsingState::Set;
        }

        switch (parseState)
        {
        case DictSetParsingState::Dict:
        {
            if (!expectPeek(TokenType::COLON))
            {
                return nullptr;
            }
            advanceTokens();

            auto value = parseExpression(Precedence::LOWEST);
            dictLiteral->elements.insert(std::make_pair(std::move(key), std::move(value)));
            break;
        }
        case DictSetParsingState::Set:
        {
            setLiteral->elements.insert(std::move(key));
            break;
        }
        case DictSetParsingState::Undecided:
            throw std::runtime_error("Invalid condition while parsing dict or set literal");
        }

        if (peekToken.type != TokenType::RBRACE && !expectPeek(TokenType::COMMA))
        {
            return nullptr;
        }
    }

    if (!expectPeek(TokenType::RBRACE))
    {
        return nullptr;
    }

    if (parseState == DictSetParsingState::Dict)
        return dictLiteral;

    if (parseState == DictSetParsingState::Set)
        return setLiteral;

    // an empty declaration is always seen as a dictionary
    return dictLiteral;
}

std::pair<
    std::vector<ast::Identifier>,
    std::vector<std::unique_ptr<ast::TypeExpression>>>
Parser::parseFunctionArguments()
{
    std::vector<ast::Identifier> identifiers;
    std::vector<std::unique_ptr<ast::TypeExpression>> identifierTypes;
    if (peekToken.type == TokenType::RPAREN)
    {
        advanceTokens();
        return std::make_pair(identifiers, std::move(identifierTypes));
    }

    advanceTokens();

    ast::Identifier identifier;
    identifier.token = curToken;
    identifier.value = curToken.literal;
    identifiers.push_back(std::move(identifier));

    if (peekToken.type == TokenType::COLON)
    {
        advanceTokens(); // eat the colon
        advanceTokens(); // bring to the identifier or type expression
        auto type = parseTypeExpression(Precedence::LOWEST);
        identifierTypes.push_back(std::move(type));
    }
    else
        identifierTypes.push_back(nullptr);

    while (peekToken.type == TokenType::COMMA)
    {
        advanceTokens(); // eat the comma
        advanceTokens(); // bring to the identifier
        ast::Identifier identifier;
        identifier.token = curToken;
        identifier.value = curToken.literal;
        identifiers.push_back(std::move(identifier));

        if (peekToken.type == TokenType::COLON)
        {
            advanceTokens(); // eat the colon
            advanceTokens(); // bring to the identifier or type expression
            auto type = parseTypeExpression(Precedence::LOWEST);
            identifierTypes.push_back(std::move(type));
        }
        else
            identifierTypes.push_back(nullptr);
    }

    if (!expectPeek(TokenType::RPAREN))
        return std::make_pair(identifiers, std::move(identifierTypes));

    return std::make_pair(identifiers, std::move(identifierTypes));
}

std::vector<std::unique_ptr<ast::Expression>> Parser::parseCallArguments()
{
    return parseExpressionList(TokenType::RPAREN);
}

std::vector<std::unique_ptr<ast::Expression>> Parser::parseExpressionList(const TokenType &endToken)
{
    std::vector<std::unique_ptr<ast::Expression>> arguments;
    if (peekToken.type == endToken)
    {
        advanceTokens();
        return arguments;
    }

    advanceTokens();

    auto firstElement = parseExpression(Precedence::LOWEST);
    if (!firstElement)
        return arguments;

    arguments.push_back(std::move(firstElement));

    while (peekToken.type == TokenType::COMMA)
    {
        advanceTokens(); // eat the comma
        advanceTokens(); // bring to the identifier
        arguments.push_back(std::move(parseExpression(Precedence::LOWEST)));
    }

    if (!expectPeek(endToken))
        return arguments;

    return arguments;
}

std::vector<std::unique_ptr<ast::TypeExpression>> Parser::parseTypeExpressionList(const TokenType &endToken)
{
    std::vector<std::unique_ptr<ast::TypeExpression>> arguments;
    if (peekToken.type == endToken)
    {
        advanceTokens();
        return arguments;
    }

    advanceTokens();

    arguments.push_back(parseTypeExpression(Precedence::LOWEST));

    while (peekToken.type == TokenType::COMMA)
    {
        advanceTokens(); // eat the comma
        advanceTokens(); // bring to the identifier
        arguments.push_back(std::move(parseTypeExpression(Precedence::LOWEST)));
    }

    if (!expectPeek(endToken))
        return arguments;

    return arguments;
}

std::unique_ptr<ast::Expression> Parser::parseGroupedExpression()
{
    advanceTokens();
    std::unique_ptr<ast::Expression> groupedExpression = parseExpression(Precedence::LOWEST);

    if (!expectPeek(TokenType::RPAREN))
        return nullptr;

    return groupedExpression;
}

std::unique_ptr<ast::Expression> Parser::parseIfExpression()
{
    std::unique_ptr<ast::IfExpression> ifExpression = std::make_unique<ast::IfExpression>();

    if (!expectPeek(TokenType::LPAREN))
        return nullptr;

    advanceTokens();
    ifExpression->condition = parseExpression(Precedence::LOWEST);

    if (!expectPeek(TokenType::RPAREN))
        return nullptr;

    if (!expectPeek(TokenType::LBRACE))
        return nullptr;

    ifExpression->consequence = parseBlockStatement();

    if (peekToken.type == TokenType::ELSE)
    {
        advanceTokens();

        if (!expectPeek(TokenType::LBRACE))
            return nullptr;

        ifExpression->alternative = parseBlockStatement();
    }
    return ifExpression;
}

std::unique_ptr<ast::Expression> Parser::parseWhileExpression()
{
    std::unique_ptr<ast::WhileExpression> whileExpression = std::make_unique<ast::WhileExpression>();

    if (!expectPeek(TokenType::LPAREN))
        return nullptr;

    advanceTokens();
    whileExpression->condition = parseExpression(Precedence::LOWEST);

    if (!expectPeek(TokenType::RPAREN))
        return nullptr;

    if (!expectPeek(TokenType::LBRACE))
        return nullptr;

    whileExpression->statement = parseBlockStatement();

    return whileExpression;
}

std::unique_ptr<ast::Expression> Parser::parseForExpression()
{
    std::unique_ptr<ast::ForExpression> forExpression = std::make_unique<ast::ForExpression>();

    if (!expectPeek(TokenType::LPAREN))
        return nullptr;

    advanceTokens();

    if (curToken.type == TokenType::CONST)
    {
        forExpression->constant = true;
        advanceTokens();
    }

    auto identifier = parseIdentifier();
    forExpression->name = *(static_cast<ast::Identifier *>(identifier.get()));

    advanceTokens();

    if (curToken.type == TokenType::COLON)
    {
        // read the type information into the statement
        advanceTokens(); // eat the colon

        // the type definition is either a list of types or just a single type
        auto iterType = parseTypeExpression(Precedence::LOWEST);
        forExpression->iterType = std::move(iterType);
        advanceTokens();
    }

    if (curToken.type != TokenType::IN)
    {
        parseError("Expected " + toString(TokenType::IN), curToken);
        return NULL;
    }

    advanceTokens();

    forExpression->iterable = parseExpression(Precedence::LOWEST);

    if (!expectPeek(TokenType::RPAREN))
        return nullptr;

    if (!expectPeek(TokenType::LBRACE))
        return nullptr;

    forExpression->statement = parseBlockStatement();

    return forExpression;
}

std::unique_ptr<ast::Expression> Parser::parseExpression(Precedence precedence)
{
    std::unique_ptr<ast::Expression> expression;
    TPrefixParseFn prefix = prefixParseFns[curToken.type];
    if (!prefix)
    {
        parseError("no prefix parse function found for " + curToken.literal, curToken);
        return expression;
    }

    std::unique_ptr<ast::Expression> leftExp = std::invoke(prefix, this);
    while (!(peekToken.type == TokenType::SEMICOLON) && (precedence < peekPrecedence()))
    {
        TInfixParseFn infix = infixParseFns[peekToken.type];
        if (!infix)
            return leftExp;

        advanceTokens();
        leftExp = std::invoke(infix, this, std::move(leftExp));
    }
    return leftExp;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeExpression(Precedence precedence)
{
    std::unique_ptr<ast::TypeExpression> expression;
    TPrefixParseTypeFn prefix = prefixParseTypeFns[curToken.type];
    if (!prefix)
    {
        parseError("no prefix parse type function found for " + curToken.literal, curToken);
        return expression;
    }

    std::unique_ptr<ast::TypeExpression> leftExp = std::invoke(prefix, this);
    while (!(peekToken.type == TokenType::SEMICOLON) && (precedence < peekPrecedence()))
    {
        TInfixParseTypeFn infix = infixParseTypeFns[peekToken.type];
        if (!infix)
            return leftExp;

        advanceTokens();
        leftExp = std::invoke(infix, this, std::move(leftExp));
    }
    return leftExp;
}

std::unique_ptr<ast::ExpressionStatement> Parser::parseExpressionStatement()
{
    std::unique_ptr<ast::ExpressionStatement> exprStatement = std::make_unique<ast::ExpressionStatement>();
    exprStatement->token = curToken;
    exprStatement->expression = parseExpression(Precedence::LOWEST);

    if (peekToken.type == TokenType::SEMICOLON)
    {
        nextToken();
    }
    return exprStatement;
}

std::unique_ptr<ast::Statement> Parser::parseStatement()
{
    if (curToken.type == TokenType::LET)
        return parseLetStatement();
    else if (curToken.type == TokenType::RETURN)
        return parseReturnStatement();
    else if (curToken.type == TokenType::BREAK)
        return parseBreakStatement();
    else if (curToken.type == TokenType::TRY)
        return parseTryExceptStatement();
    else if (curToken.type == TokenType::SCOPE)
        return parseScopeStatement();
    else if (curToken.type == TokenType::COMMENT)
        return nullptr;
    else if (curToken.type == TokenType::DOC)
    {
        curDoc.push_back(curToken.literal);
        return nullptr;
    }
    else
        return parseExpressionStatement();

    return nullptr;
}

std::unique_ptr<ast::LetStatement> Parser::parseLetStatement()
{
    std::unique_ptr<ast::LetStatement> letStatement = std::make_unique<ast::LetStatement>();
    letStatement->token = curToken;

    advanceTokens();

    if (curToken.type == TokenType::CONST)
    {
        letStatement->constant = true;
        advanceTokens();
    }

    auto identifier = parseIdentifier();

    advanceTokens();

    if (curToken.type == TokenType::COLON)
    {
        // read the type information into the statement
        advanceTokens(); // eat the colon

        // the type definition is either a list of types or just a single type
        auto valueType = parseTypeExpression(Precedence::LOWEST);
        letStatement->valueType = std::move(valueType);
        advanceTokens();
    }

    if (curToken.type != TokenType::ASSIGN)
    {
        parseError("Expected " + toString(TokenType::ASSIGN), curToken);
        return NULL;
    }

    advanceTokens();

    std::unique_ptr<ast::Expression> value = parseExpression(Precedence::LOWEST);
    letStatement->name = *(static_cast<ast::Identifier *>(identifier.get()));
    letStatement->value = std::move(value);

    if (peekToken.type == TokenType::SEMICOLON)
        advanceTokens();

    return letStatement;
}

std::unique_ptr<ast::ImportStatement> Parser::parseImportStatement()
{
    std::unique_ptr<ast::ImportStatement> useStatement = std::make_unique<ast::ImportStatement>();
    useStatement->token = curToken;

    advanceTokens();

    auto identifier = parseModuleIdentifier();
    useStatement->name = *(static_cast<ast::ModuleIdentifier *>(identifier.get()));

    if (peekToken.type == TokenType::SEMICOLON)
        advanceTokens();

    return useStatement;
}

std::unique_ptr<ast::ScopeStatement> Parser::parseScopeStatement()
{
    std::unique_ptr<ast::ScopeStatement> scopeStatement = std::make_unique<ast::ScopeStatement>();
    scopeStatement->token = curToken;

    advanceTokens(); // advance beyond the SCOPE lexeme
    advanceTokens();

    while ((curToken.type != TokenType::RBRACE) && (curToken.type != TokenType::EOF_T))
    {
        std::unique_ptr<ast::Statement> statement = parseStatement();
        if (statement)
            scopeStatement->statements.push_back(std::move(statement));

        advanceTokens();
    };

    return scopeStatement;
}

std::unique_ptr<ast::TypeStatement> Parser::parseTypeStatement()
{
    std::unique_ptr<ast::TypeStatement> typeStatement = std::make_unique<ast::TypeStatement>();
    typeStatement->token = curToken;

    // parse/eat any comments and doc strings
    while (curToken.type == TokenType::COMMENT || curToken.type == TokenType::DOC)
    {
        if (curToken.type == TokenType::DOC)
            curDoc.push_back(curToken.literal);

        advanceTokens();
    };

    if (curToken.type == TokenType::CONST)
    {
        typeStatement->constant = true;
        advanceTokens();
    }

    auto identifier = parseIdentifier();

    advanceTokens();

    if (curToken.type == TokenType::COLON)
    {
        // read the type information into the statement
        advanceTokens(); // eat the colon

        // the type definition is either a list of types or just a single type
        auto type = parseTypeExpression(Precedence::LOWEST);
        typeStatement->exprType = std::move(type);
        advanceTokens();
    }

    if (curToken.type != TokenType::ASSIGN)
    {
        parseError("Expected " + toString(TokenType::ASSIGN), curToken);
        return NULL;
    }

    advanceTokens();

    std::unique_ptr<ast::Expression> value = parseExpression(Precedence::LOWEST);
    typeStatement->name = *(static_cast<ast::Identifier *>(identifier.get()));
    typeStatement->value = std::move(value);

    if (peekToken.type == TokenType::SEMICOLON)
        advanceTokens();

    return typeStatement;
}

std::unique_ptr<ast::ReturnStatement> Parser::parseReturnStatement()
{
    std::unique_ptr<ast::ReturnStatement> returnStatement = std::make_unique<ast::ReturnStatement>();
    returnStatement->token = curToken;

    advanceTokens();

    std::unique_ptr<ast::Expression> value = parseExpression(Precedence::LOWEST);
    returnStatement->returnValue = std::move(value);

    if (peekToken.type == TokenType::SEMICOLON)
        advanceTokens();

    return returnStatement;
}

std::unique_ptr<ast::BreakStatement> Parser::parseBreakStatement()
{
    std::unique_ptr<ast::BreakStatement> breakStatement = std::make_unique<ast::BreakStatement>();
    breakStatement->token = curToken;

    advanceTokens();

    if (peekToken.type == TokenType::SEMICOLON)
        advanceTokens();

    return breakStatement;
}

std::unique_ptr<ast::TryExceptStatement> Parser::parseTryExceptStatement()
{
    std::unique_ptr<ast::TryExceptStatement> tryExceptStatement = std::make_unique<ast::TryExceptStatement>();
    tryExceptStatement->token = curToken;

    if (!expectPeek(TokenType::LBRACE))
        return nullptr;

    tryExceptStatement->statement = parseBlockStatement();

    if (!expectPeek(TokenType::EXCEPT))
        return nullptr;

    if (!expectPeek(TokenType::LPAREN))
        return nullptr;

    advanceTokens();

    auto identifier = parseIdentifier();
    if (identifier)
        tryExceptStatement->name = *(static_cast<ast::Identifier *>(identifier.get()));

    if (!expectPeek(TokenType::RPAREN))
        return nullptr;

    if (!expectPeek(TokenType::LBRACE))
        return nullptr;

    tryExceptStatement->except = parseBlockStatement();
    if (peekToken.type == TokenType::SEMICOLON)
        advanceTokens();

    return tryExceptStatement;
}

std::unique_ptr<ast::BlockStatement> Parser::parseBlockStatement()
{
    std::unique_ptr<ast::BlockStatement> blockStatement = std::make_unique<ast::BlockStatement>();
    blockStatement->token = curToken;

    advanceTokens();

    while ((curToken.type != TokenType::RBRACE) && (curToken.type != TokenType::EOF_T))
    {
        std::unique_ptr<ast::Statement> statement = parseStatement();
        if (statement)
            blockStatement->statements.push_back(std::move(statement));

        advanceTokens();
    };

    return blockStatement;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeIdentifier()
{
    std::unique_ptr<ast::TypeIdentifier> identifier = std::make_unique<ast::TypeIdentifier>();
    identifier->token = curToken;
    identifier->value = curToken.literal;
    return identifier;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeNull()
{
    std::unique_ptr<ast::TypeNull> nullLiteral = std::make_unique<ast::TypeNull>();
    nullLiteral->token = curToken;
    return nullLiteral;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeAny()
{
    std::unique_ptr<ast::TypeAny> anyLiteral = std::make_unique<ast::TypeAny>();
    anyLiteral->token = curToken;
    return anyLiteral;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeAll()
{
    std::unique_ptr<ast::TypeAll> allLiteral = std::make_unique<ast::TypeAll>();
    allLiteral->token = curToken;
    return allLiteral;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeChoice()
{
    std::unique_ptr<ast::TypeChoice> choice = std::make_unique<ast::TypeChoice>();
    choice->token = curToken;
    choice->choices = parseTypeExpressionList(TokenType::GT);

    return choice;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeArray()
{
    std::unique_ptr<ast::TypeArray> arrayType = std::make_unique<ast::TypeArray>();
    arrayType->token = curToken;

    advanceTokens();
    if (curToken.type == TokenType::RBRACKET)
    {
        parseError("array type expression cannot be empty", curToken);
        return nullptr;
    }

    arrayType->elementType = parseTypeExpression(Precedence::LOWEST);
    if (!arrayType->elementType)
        return nullptr;
    if (!expectPeek(TokenType::RBRACKET))
        return nullptr;

    return arrayType;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeDictionaryOrSet()
{
    advanceTokens();
    auto keyType = parseTypeExpression(Precedence::LOWEST);

    if (peekToken.type != TokenType::COLON)
    {
        std::unique_ptr<ast::TypeSet> setType = std::make_unique<ast::TypeSet>();
        setType->token = curToken;
        setType->elementType = std::move(keyType);
        if (!expectPeek(TokenType::RBRACE))
            return nullptr;
        return setType;
    }

    std::unique_ptr<ast::TypeDictionary> dictType = std::make_unique<ast::TypeDictionary>();
    dictType->token = curToken;
    dictType->keyType = std::move(keyType);

    if (!expectPeek(TokenType::COLON))
        return nullptr;

    advanceTokens();
    dictType->valueType = parseTypeExpression(Precedence::LOWEST);

    if (!expectPeek(TokenType::RBRACE))
        return nullptr;

    return dictType;
}

std::unique_ptr<ast::TypeExpression> Parser::parseTypeFunction()
{
    std::unique_ptr<ast::TypeFunction> funcType = std::make_unique<ast::TypeFunction>();
    funcType->token = curToken;

    if (!expectPeek(TokenType::LPAREN))
        return nullptr;

    funcType->argTypes = parseTypeExpressionList(TokenType::RPAREN);

    if (!expectPeek(TokenType::ARROW))
        return nullptr;

    advanceTokens();
    funcType->returnType = parseTypeExpression(Precedence::LOWEST);
    return funcType;
}

void checkParserErrors(const Parser &parser, size_t expectedErrors)
{
    if (parser.errorMsgs.size() == expectedErrors)
        return;

    std::stringstream ss;
    ss << "Parser had " << parser.errorMsgs.size() << " errors";
    if (expectedErrors == 0)
        ss << std::endl;
    else
        ss << ", but expected " << expectedErrors << std::endl;

    for (const auto &msg : parser.errorMsgs)
        ss << msg << std::endl;
    std::cerr << ss.str();

    throw std::runtime_error(ss.str());
}