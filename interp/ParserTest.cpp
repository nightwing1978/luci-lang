/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include "Parser.h"

void testLetStatement()
{
    std::string input = "let variable = 5";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);
}

void testLetStatementMissingAssign()
{
    std::string input = "let variable = 5;\nlet x = 10;\nlet 7;\nlet test;";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 2);
}

void testReturnStatement()
{
    std::string input = "return bla;return boo;return bar";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    if (program->statements.size() != 3)
    {
        std::stringstream ss;
        ss << "Expected 3 statements, got " << program->statements.size();
        int statementCnt = 0;
        for (const auto &statement : program->statements)
        {
            ss << "[" << statementCnt << "]: " << statement->text() << "\n";
            ++statementCnt;
        }
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }
}

void testProgramStringify()
{
    std::string input = "let var = anothervar;\n";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    auto regeneratedSource = program->text();
    if (regeneratedSource != input)
    {
        std::stringstream ss;
        ss << "Expected generated source as " << input << " [size=" << input.size() << "] but got " << regeneratedSource << " [size=" << regeneratedSource.size() << "]";
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }
}

void testIdentifierExpression()
{
    std::string input = "foobar;";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    if (program->statements.size() != 1)
    {
        std::stringstream ss;
        ss << "Expected 1 statements, got " << program->statements.size() << std::endl;
        for (const auto &statement : program->statements)
        {
            ss << statement->text() << "\n";
        }
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }
}

void testIntegerLiteralExpression()
{
    std::string input = "5;";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    if (program->statements.size() != 1)
    {
        std::stringstream ss;
        ss << "Expected 1 statements, got " << program->statements.size() << std::endl;
        for (const auto &statement : program->statements)
        {
            ss << statement->text() << "\n";
        }
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }

    auto exprStatement = dynamic_cast<ast::ExpressionStatement *>(program->statements[0].get());
    if (!exprStatement)
        throw std::runtime_error("Expected expression statement");

    auto integerLiteral = dynamic_cast<ast::IntegerLiteral *>(exprStatement->expression.get());
    if (!integerLiteral)
        throw std::runtime_error("Expected integer literal");
}

void testBooleanLiteralExpression()
{
    std::string input = "true;";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    if (program->statements.size() != 1)
    {
        std::stringstream ss;
        ss << "Expected 1 statements, got " << program->statements.size() << std::endl;
        for (const auto &statement : program->statements)
        {
            ss << statement->text() << "\n";
        }
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }

    auto exprStatement = dynamic_cast<ast::ExpressionStatement *>(program->statements[0].get());
    if (!exprStatement)
        throw std::runtime_error("Expected expression statement");

    auto boolLiteral = dynamic_cast<ast::BooleanLiteral *>(exprStatement->expression.get());
    if (!boolLiteral)
        throw std::runtime_error("Expected Boolean literal");
}

void testPrefixExpression()
{
    std::string input = "!5;";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    if (program->statements.size() != 1)
    {
        std::stringstream ss;
        ss << "Expected 1 statements, got " << program->statements.size() << std::endl;
        for (const auto &statement : program->statements)
        {
            ss << statement->text() << "\n";
        }
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }

    auto exprStatement = dynamic_cast<ast::ExpressionStatement *>(program->statements[0].get());
    if (!exprStatement)
        throw std::runtime_error("Expected expression statement");

    auto prefixExpr = dynamic_cast<ast::PrefixExpression *>(exprStatement->expression.get());
    if (!prefixExpr)
        throw std::runtime_error("Expected prefix expression");
}

void testInfixExpression()
{
    struct InfixResult
    {
        int left;
        std::string operator_t;
        int right;
    };

    std::map<std::string, InfixResult> tests = {
        {"5+5", {5, "+", 5}},
        {"5-5", {5, "-", 5}},
        {"5*5", {5, "*", 5}},
        {"5/5", {5, "/", 5}},
        {"5>5", {5, ">", 5}},
        {"5<5", {5, "<", 5}},
        {"5==5", {5, "==", 5}},
        {"5!=5", {5, "!=", 5}},
    };

    for (const auto &test : tests)
    {
        const std::string &input = test.first;
        auto lexer = createLexer(input, "");
        auto parser = createParser(std::move(lexer));
        auto program = parser->parseProgram();
        checkParserErrors(*parser, 0);

        if (program->statements.size() != 1)
        {
            std::stringstream ss;
            ss << "Expected 1 statements, got " << program->statements.size() << std::endl;
            for (const auto &statement : program->statements)
            {
                ss << statement->text() << "\n";
            }
            std::cerr << ss.str();
            throw std::runtime_error(ss.str());
        }

        auto exprStatement = dynamic_cast<ast::ExpressionStatement *>(program->statements[0].get());
        if (!exprStatement)
            throw std::runtime_error("Expected expression statement");

        auto infixExpr = dynamic_cast<ast::InfixExpression *>(exprStatement->expression.get());
        if (!infixExpr)
            throw std::runtime_error("Expected infix expression");

        if (infixExpr->operator_t.literal != test.second.operator_t)
        {
            std::cerr << "Operator found " << infixExpr->operator_t.literal << ", expecting " << test.second.operator_t << std::endl;
            throw std::runtime_error("Wrong operator found");
        }

        auto intLeft = dynamic_cast<ast::IntegerLiteral *>(infixExpr->left.get());
        if (!intLeft)
            throw std::runtime_error("Expected integer literal");

        if (intLeft->value != test.second.left)
            throw std::runtime_error("Expected value of literal to be 5");

        auto intRight = dynamic_cast<ast::IntegerLiteral *>(infixExpr->right.get());
        if (!intRight)
            throw std::runtime_error("Expected integer literal");

        if (intRight->value != test.second.right)
            throw std::runtime_error("Expected value of literal to be 5");
    }
}

void testOperatorPrecedence()
{
    std::vector<std::pair<std::string, std::string>> tests =
        {
            {"-a * b", "((-a) * b);"},
            {"!-a", "(!(-a));"},
            {"a + b", "(a + b);"},
            {"a + b + c", "((a + b) + c);"},
            {"a + b - c", "((a + b) - c);"},
            {"a * b * c", "((a * b) * c);"},
            {"a * b / c", "((a * b) / c);"},
            {"a + b * c", "(a + (b * c));"},
            {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f);"},
            {"(2 + 3)", "(2 + 3);"},
            {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4);"},
            {"(5 + 5) * 2", "((5 + 5) * 2);"},
            {"-(5 + 5)", "(-(5 + 5));"},
            {"2**-3", "(2 ** (-3));"},
        };

    for (const auto &test : tests)
    {
        const std::string &input = test.first;
        auto lexer = createLexer(input, "");
        auto parser = createParser(std::move(lexer));
        auto program = parser->parseProgram();
        checkParserErrors(*parser, 0);

        auto regeneratedSource = program->text();
        if (regeneratedSource != test.second + "\n")
        {
            std::stringstream ss;
            ss << "Expected " << test.second << " but got " << regeneratedSource;
            ss << "Expected " << test.second.size() << " but got " << regeneratedSource.size();
            std::cerr << ss.str() << std::endl;
            throw std::runtime_error(ss.str());
        }
    }
}

void testIfExpression()
{
    std::string input = "if (x < y) { x } else { y }";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    if (program->statements.size() != 1)
    {
        std::stringstream ss;
        ss << "Expected 1 statements, got " << program->statements.size() << std::endl;
        for (const auto &statement : program->statements)
        {
            ss << statement->text() << "\n";
        }
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }

    auto exprStatement = dynamic_cast<ast::ExpressionStatement *>(program->statements[0].get());
    if (!exprStatement)
        throw std::runtime_error("Expected expression statement");

    auto ifExpr = dynamic_cast<ast::IfExpression *>(exprStatement->expression.get());
    if (!ifExpr)
        throw std::runtime_error("Expected if-expression");
}

void testFunctionLiteralParsing()
{
    std::string input = "fn (x, y) { x + y; }";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    if (program->statements.size() != 1)
    {
        std::stringstream ss;
        ss << "Expected 1 statements, got " << program->statements.size() << std::endl;
        for (const auto &statement : program->statements)
        {
            ss << statement->text() << "\n";
        }
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }

    auto exprStatement = dynamic_cast<ast::ExpressionStatement *>(program->statements[0].get());
    if (!exprStatement)
        throw std::runtime_error("Expected expression statement");

    auto fnLiteral = dynamic_cast<ast::FunctionLiteral *>(exprStatement->expression.get());
    if (!fnLiteral)
        throw std::runtime_error("Expected function literal");

    if (fnLiteral->arguments.size() != 2)
        throw std::runtime_error("Expected 2 arguments");
}

void testCallExpression()
{
    std::string input = "add(5,3,7+3)";
    auto lexer = createLexer(input, "");
    auto parser = createParser(std::move(lexer));
    auto program = parser->parseProgram();
    checkParserErrors(*parser, 0);

    if (program->statements.size() != 1)
    {
        std::stringstream ss;
        ss << "Expected 1 statements, got " << program->statements.size() << std::endl;
        for (const auto &statement : program->statements)
        {
            ss << statement->text() << "\n";
        }
        std::cerr << ss.str();
        throw std::runtime_error(ss.str());
    }

    auto exprStatement = dynamic_cast<ast::ExpressionStatement *>(program->statements[0].get());
    if (!exprStatement)
        throw std::runtime_error("Expected expression statement");

    auto callExpr = dynamic_cast<ast::CallExpression *>(exprStatement->expression.get());
    if (!callExpr)
        throw std::runtime_error("Expected call expression");

    if (callExpr->arguments.size() != 3)
        throw std::runtime_error("Expected 3 arguments");
}

int main()
{
    try
    {
        testLetStatement();
        testLetStatementMissingAssign();
        testReturnStatement();
        testProgramStringify();
        testIdentifierExpression();
        testIntegerLiteralExpression();
        testBooleanLiteralExpression();
        testPrefixExpression();
        testInfixExpression();
        testOperatorPrecedence();
        testIfExpression();
        testFunctionLiteralParsing();
        testCallExpression();
        std::cerr << "All tests passed" << std::endl;
        return 0;
    }
    catch (std::runtime_error &e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }
}