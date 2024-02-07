/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>

#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
#include "Util.h"
#include "Typing.h"
#include "Evaluator.h" // to initialize the builtins

struct StatementAnalysis
{
    ast::Node *node;
    std::string name;
    std::unique_ptr<ast::TypeExpression> computedType;
    bool declaredAndComputedVerified = false;
    bool declaredAndComputedCompatible = true;
};

struct AnalysisOptions
{
    bool useDeclaration = false;
    bool computeTypes = true;
};

struct Analysis
{
    std::vector<StatementAnalysis> statements;
    AnalysisOptions options;
};

bool nodeIsTerminal(ast::NodeType type)
{
    switch (type)
    {
    case ast::NodeType::BooleanLiteral:
    case ast::NodeType::IntegerLiteral:
    case ast::NodeType::DoubleLiteral:
    case ast::NodeType::ComplexLiteral:
    case ast::NodeType::StringLiteral:
    case ast::NodeType::NullLiteral:
        return true;
    }
    return false;
}

void linearizeAstStatementTree(ast::Node *node, std::vector<ast::Node *> &nodes)
{
    nodes.push_back(node);
    switch (node->type)
    {
    case ast::NodeType::Program:
    {
        auto program = static_cast<ast::Program *>(node);
        for (const auto &statement : program->statements)
            linearizeAstStatementTree(statement.get(), nodes);
    }
    break;
    case ast::NodeType::LetStatement:
    {
        auto letStatement = static_cast<ast::LetStatement *>(node);
        linearizeAstStatementTree(letStatement->value.get(), nodes);
    }
    break;
    case ast::NodeType::BlockStatement:
    {
        auto block = static_cast<ast::BlockStatement *>(node);
        for (const auto &statement : block->statements)
            linearizeAstStatementTree(statement.get(), nodes);
    }
    break;
    case ast::NodeType::WhileExpression:
    {
        auto block = static_cast<ast::WhileExpression *>(node)->statement.get();
        for (const auto &statement : block->statements)
            linearizeAstStatementTree(statement.get(), nodes);
    }
    break;
    case ast::NodeType::IfExpression:
    {
        auto block = static_cast<ast::IfExpression *>(node)->consequence.get();
        for (const auto &statement : block->statements)
            linearizeAstStatementTree(statement.get(), nodes);
        auto block2 = static_cast<ast::IfExpression *>(node)->alternative.get();
        for (const auto &statement : block2->statements)
            linearizeAstStatementTree(statement.get(), nodes);
    }
    break;
    case ast::NodeType::ForExpression:
    {
        auto block = static_cast<ast::ForExpression *>(node)->statement.get();
        for (const auto &statement : block->statements)
            linearizeAstStatementTree(statement.get(), nodes);
    }
    break;
    case ast::NodeType::FunctionLiteral:
    {
        auto block = static_cast<ast::FunctionLiteral *>(node)->body.get();
        for (const auto &statement : block->statements)
            linearizeAstStatementTree(statement.get(), nodes);
    }
    break;
    }
}

void reportAnalysis(const Analysis &analysis)
{
    for (auto &stmt : analysis.statements)
    {
        std::stringstream ss;
        ss << "[" << stmt.node->token.lineNumber << "] ";
        ss << stmt.name << " : ";
        if (stmt.computedType)
            ss << stmt.computedType->text();
        else
            ss << "unknown";
        if (stmt.declaredAndComputedVerified && !stmt.declaredAndComputedCompatible)
            ss << " <incompatible>";
        if (stmt.declaredAndComputedVerified && stmt.declaredAndComputedCompatible)
            ss << " <verified>";
        std::cout << ss.str() << std::endl;
    }
}

void addFunctionDeclarations(Analysis &analysis)
{
    for (auto &stmt : analysis.statements)
    {
        if (stmt.node->type == ast::NodeType::LetStatement)
        {
            auto letStatement = static_cast<ast::LetStatement *>(stmt.node);
            if (letStatement->value->type == ast::NodeType::FunctionLiteral)
            {
                auto funcLiteral = static_cast<ast::FunctionLiteral *>(letStatement->value.get());
                if (stmt.computedType->type == ast::NodeType::TypeFunction)
                {
                    auto computedFuncType = static_cast<ast::TypeFunction *>(stmt.computedType.get());
                    for (size_t i = 0; i < computedFuncType->argTypes.size(); ++i)
                    {
                        if (computedFuncType->argTypes[i]->text() == "all")
                        {
                            // see if we can pick up anything more from the declartion
                            if (i < funcLiteral->argumentTypes.size() && funcLiteral->argumentTypes[i])
                            {
                                computedFuncType->argTypes[i] = funcLiteral->argumentTypes[i]->clone();
                            }
                        }
                    }
                }
            }
        }
    }
}

void addVariableDeclrations(Analysis &analysis)
{
    for (auto &stmt : analysis.statements)
    {
        if (stmt.node->type == ast::NodeType::LetStatement)
        {
            auto letStatement = static_cast<ast::LetStatement *>(stmt.node);
            if (letStatement->valueType)
            {
                stmt.declaredAndComputedVerified = true;
                stmt.declaredAndComputedCompatible = typing::isCompatibleType(letStatement->valueType.get(), stmt.computedType.get());
                stmt.computedType = std::move(letStatement->valueType->clone());
            }
        }
    }
}

void removeUnreachableCode(ast::Statement *statement, std::shared_ptr<typing::AnalysisContext> context, Analysis &analysis)
{
    switch (statement->type)
    {
    case ast::NodeType::BlockStatement:
    {
        // any statement after a return or continue/break is unreachable within a block top level scope
        auto block = static_cast<ast::BlockStatement *>(statement);
        int unreachableIndex = -1;
        for (int i = 0; i < static_cast<int>(block->statements.size()); ++i)
        {
            const auto &stmt(block->statements[i]);
            switch (stmt->type)
            {
            case ast::NodeType::BreakStatement:
            case ast::NodeType::ContinueStatement:
            case ast::NodeType::ReturnStatement:
                unreachableIndex = i + 1;
                break;
            }
            if (unreachableIndex >= 0)
                break;
        }
        if (unreachableIndex > 0)
        {
            block->statements.resize(unreachableIndex);
        }
    }
    case ast::NodeType::ScopeStatement:
    {
        auto newScopeContext = std::make_shared<typing::AnalysisContext>();
        newScopeContext->outer = context;
        auto scopeStatement = static_cast<ast::ScopeStatement *>(statement);
        for (const auto &stmt : scopeStatement->statements)
            removeUnreachableCode(stmt.get(), newScopeContext, analysis);
        break;
    }
    case ast::NodeType::ExpressionStatement:
    {
        auto exprStmt = static_cast<ast::ExpressionStatement *>(statement);
        switch (exprStmt->expression->type)
        {
        case ast::NodeType::WhileExpression:
        {
            auto newScopeContext = std::make_shared<typing::AnalysisContext>();
            newScopeContext->outer = context;
            auto scopeStatement = static_cast<ast::ScopeStatement *>(statement);
            auto block = static_cast<ast::WhileExpression *>(exprStmt->expression.get())->statement.get();
            for (const auto &stmt : block->statements)
                removeUnreachableCode(stmt.get(), newScopeContext, analysis);
        }
        break;
        case ast::NodeType::IfExpression:
        {
            auto block = static_cast<ast::IfExpression *>(exprStmt->expression.get())->consequence.get();
            {
                auto newScopeContext = std::make_shared<typing::AnalysisContext>();
                newScopeContext->outer = context;
                for (const auto &stmt : block->statements)
                    removeUnreachableCode(stmt.get(), newScopeContext, analysis);
            }
            {
                auto newScopeContext = std::make_shared<typing::AnalysisContext>();
                newScopeContext->outer = context;
                auto block2 = static_cast<ast::IfExpression *>(exprStmt->expression.get())->alternative.get();
                for (const auto &stmt : block2->statements)
                    removeUnreachableCode(stmt.get(), newScopeContext, analysis);
            }
        }
        break;
        case ast::NodeType::ForExpression:
        {
            auto newScopeContext = std::make_shared<typing::AnalysisContext>();
            newScopeContext->outer = context;
            auto block = static_cast<ast::ForExpression *>(exprStmt->expression.get())->statement.get();
            for (const auto &stmt : block->statements)
                removeUnreachableCode(stmt.get(), newScopeContext, analysis);
        }
        break;
        case ast::NodeType::FunctionLiteral:
        {
            auto newScopeContext = std::make_shared<typing::AnalysisContext>();
            newScopeContext->outer = context;
            auto block = static_cast<ast::FunctionLiteral *>(exprStmt->expression.get())->body.get();
            for (const auto &stmt : block->statements)
                removeUnreachableCode(stmt.get(), newScopeContext, analysis);
        }
        }
        break;
    }
    case ast::NodeType::LetStatement:
    {
        auto letStmt = static_cast<ast::LetStatement *>(statement);
        switch (letStmt->value->type)
        {
        case ast::NodeType::FunctionLiteral:
        {
            auto funcLiteral = static_cast<ast::FunctionLiteral *>(letStmt->value.get());
            auto block = static_cast<ast::FunctionLiteral *>(letStmt->value.get())->body.get();
            removeUnreachableCode(block, context, analysis);
            break;
        }
        }
        break;
    }
    }
}

void analyzeStatement(ast::Statement *statement, std::shared_ptr<typing::AnalysisContext> context, Analysis &analysis)
{
    const int maxSizeWithEllipsis = 60;
    switch (statement->type)
    {
    case ast::NodeType::BlockStatement:
    {
        // do not create an extra scoped context, any callee will need to do this
        auto block = static_cast<ast::BlockStatement *>(statement);
        for (const auto &stmt : block->statements)
            analyzeStatement(stmt.get(), context, analysis);
        break;
    }
    case ast::NodeType::ScopeStatement:
    {
        auto newScopeContext = std::make_shared<typing::AnalysisContext>();
        newScopeContext->outer = context;
        auto scopeStatement = static_cast<ast::ScopeStatement *>(statement);
        for (const auto &stmt : scopeStatement->statements)
        {
            analyzeStatement(stmt.get(), newScopeContext, analysis);
        }
        break;
    }
    case ast::NodeType::LetStatement:
    {
        analysis.statements.push_back(StatementAnalysis());
        StatementAnalysis &stmtAnalysis = analysis.statements.back();
        stmtAnalysis.node = statement;
        auto letStatement = static_cast<ast::LetStatement *>(statement);
        stmtAnalysis.name = letStatement->name.value;
        if (letStatement->value)
        {
            bool typeIsResolved = false;
            if (analysis.options.useDeclaration)
            {
                if (letStatement->valueType)
                {
                    stmtAnalysis.computedType = letStatement->valueType->clone();
                    context->context[letStatement->name.value] = letStatement->valueType->clone();
                    typeIsResolved = true;
                }
            }

            if (analysis.options.computeTypes && !typeIsResolved)
            {
                auto computedType = typing::computeType(letStatement->value.get(), context);
                if (computedType)
                {
                    stmtAnalysis.computedType = computedType->clone();
                    context->context[letStatement->name.value] = std::move(computedType);
                }
            }

            switch (letStatement->value->type)
            {
            case ast::NodeType::FunctionLiteral:
            {
                auto funcLiteral = static_cast<ast::FunctionLiteral *>(letStatement->value.get());
                auto block = static_cast<ast::FunctionLiteral *>(letStatement->value.get())->body.get();
                // a new scope taking along the existing one to capture the closure
                auto newScopeContext = std::make_shared<typing::AnalysisContext>();
                newScopeContext->outer = context;

                // pass on the arg types into the newScopeContext,
                // at this point rely on the argTypes provided, in a second pass
                // potentially pin the types to the callers, although this is always
                // risky as a function can be exposed in a different context
                for (size_t i = 0; i < funcLiteral->arguments.size(); ++i)
                {
                    if (i < funcLiteral->argumentTypes.size() && funcLiteral->argumentTypes[i])
                        newScopeContext->context.insert_or_assign(funcLiteral->arguments[i].value, funcLiteral->argumentTypes[i]->clone());
                }
                analyzeStatement(block, newScopeContext, analysis);
                return;
            }
            break;
            }
        }
        break;
    }
    case ast::NodeType::TryExceptStatement:
    case ast::NodeType::ImportStatement:
    case ast::NodeType::ExpressionStatement:
    {
        auto exprStmt = static_cast<ast::ExpressionStatement *>(statement);
        switch (exprStmt->expression->type)
        {
        case ast::NodeType::CallExpression:
        {
            auto callExpr = static_cast<ast::CallExpression *>(exprStmt->expression.get());
            for (const auto &arg : callExpr->arguments)
            {
                if (arg->type == ast::NodeType::FunctionLiteral)
                {
                    auto funcLiteral = static_cast<ast::FunctionLiteral *>(arg.get());
                    auto block = static_cast<ast::FunctionLiteral *>(arg.get())->body.get();
                    // a new scope taking along the existing one to capture the closure
                    auto newScopeContext = std::make_shared<typing::AnalysisContext>();
                    newScopeContext->outer = context;

                    // pass on the arg types into the newScopeContext,
                    // at this point rely on the argTypes provided, in a second pass
                    // potentially pin the types to the callers, although this is always
                    // risky as a function can be exposed in a different context
                    for (size_t i = 0; i < funcLiteral->arguments.size(); ++i)
                    {
                        if (i < funcLiteral->argumentTypes.size() && funcLiteral->argumentTypes[i])
                            newScopeContext->context.insert_or_assign(funcLiteral->arguments[i].value, funcLiteral->argumentTypes[i]->clone());
                    }
                    analyzeStatement(block, newScopeContext, analysis);
                }
            }
            break;
        }

        case ast::NodeType::ForExpression:
        {
            analysis.statements.push_back(StatementAnalysis());
            StatementAnalysis &stmtAnalysis = analysis.statements.back();
            stmtAnalysis.node = statement;

            auto forExpr = static_cast<ast::ForExpression *>(exprStmt->expression.get());
            stmtAnalysis.name = forExpr->name.value;
            // a new scope taking along the existing one to capture the closure
            // with a for loop there is also an implicit assignment on each iteration
            // of the loop variable
            auto newScopeContext = std::make_shared<typing::AnalysisContext>();
            newScopeContext->outer = context;

            bool typeIsResolved = false;
            if (analysis.options.useDeclaration)
            {
                if (forExpr->iterType)
                {
                    stmtAnalysis.computedType = forExpr->iterType->clone();
                    context->context[forExpr->name.value] = forExpr->iterType->clone();
                    typeIsResolved = true;
                }
            }

            if (analysis.options.computeTypes && !typeIsResolved)
            {
                auto computedTypeIterable = typing::computeType(forExpr->iterable.get(), context);
                if (computedTypeIterable)
                {
                    auto computedTypeIter = typing::computeIndexedType(computedTypeIterable.get(), context);
                    if (computedTypeIter)
                    {
                        stmtAnalysis.computedType = computedTypeIter->clone();
                        newScopeContext->context[forExpr->name.value] = std::move(computedTypeIter);
                    }
                }
            }
            analyzeStatement(forExpr->statement.get(), newScopeContext, analysis);
        }
        break;

        case ast::NodeType::TypeLiteral:
        {
            auto typeLiteral = static_cast<ast::TypeLiteral *>(exprStmt->expression.get());
            auto typeType = std::make_unique<ast::TypeType>();
            typeType->value = typeLiteral->name;
            context->context[typeLiteral->name] = std::move(typeType);
            context->typeContext[typeLiteral->name] = typeLiteral;
            break;
        }

        default:
            break;
        }
    }
    case ast::NodeType::ReturnStatement:
    case ast::NodeType::BreakStatement:
    {
        return;
    }
    };
}

void removeUnreachableCode(ast::Program *program, Analysis &analysis)
{
    auto context = std::make_shared<typing::AnalysisContext>();
    for (const auto &statement : program->statements)
    {
        removeUnreachableCode(statement.get(), context, analysis);
    }
}

void analyzeProgram(ast::Program *program, Analysis &analysis)
{
    removeUnreachableCode(program, analysis);

    auto context = std::make_shared<typing::AnalysisContext>();
    for (const auto &statement : program->statements)
    {
        analyzeStatement(statement.get(), context, analysis);
    }

    addFunctionDeclarations(analysis);
    addVariableDeclrations(analysis);
}

void analyzeProgramLinear(ast::Program *program)
{
    auto context = std::make_shared<typing::AnalysisContext>();
    const int maxSizeWithEllipsis = 60;
    std::vector<ast::Node *> nodes;
    linearizeAstStatementTree(program, nodes);
    for (const auto &node : nodes)
    {
        if (node->type == ast::NodeType::LetStatement)
        {
            std::cout << "[" << node->token.lineNumber << "] "
                      << "LetStatement: " << util::removeNewLines(util::withEllipsis(node->text(), maxSizeWithEllipsis)) << std::endl;
            auto letStatement = static_cast<ast::LetStatement *>(node);
            if (letStatement->valueType)
            {
                std::cout << "  " << util::removeNewLines(util::withEllipsis(letStatement->valueType->text(), maxSizeWithEllipsis)) << std::endl;
            }
            else
            {
                std::cout << "  no type info provided" << std::endl;
            }
            if (letStatement->value && letStatement->value)
            {
                auto computedType = typing::computeType(letStatement->value.get(), context);
                if (computedType)
                    std::cout
                        << "  computed value-type=" << computedType->text() << std::endl;
                else
                    std::cout << "  computed value-type= unknown" << std::endl;
            }
        }
    }
}

void usage(int argc, char *argv)
{
    std::cout << argv[0] << "\n";
    std::cout << "Usage: \n";
    std::cout << argv[0] << " file_name\n";
    std::cout << "  file_name	analyze the given file_name\n";
}

int main(int argc, char **argv)
try
{
    std::string fileToAnalyze = "";

    int returnValue = 2;
    if (argc == 2)
    {
        fileToAnalyze = std::string(argv[1]);
    }

    initialize();

    if (!fileToAnalyze.empty())
    {
        std::string text;
        std::string line;
        std::ifstream inputf;
        inputf.open(fileToAnalyze);
        if (inputf.is_open())
        {
            while (std::getline(inputf, line))
            {
                text += line + "\n";
            }
        }
        else
        {
            std::cerr << "File " << fileToAnalyze << " cannot be read" << std::endl;
            returnValue = 2;
        }
        auto lexer = createLexer(text, fileToAnalyze);
        auto parser = createParser(std::move(lexer));
        auto program = parser->parseProgram();

        if (!parser->errorMsgs.empty())
        {
            std::stringstream ss;
            for (const auto &msg : parser->errorMsgs)
                ss << msg << std::endl;
            std::cerr << util::color::colorize(ss.str(), util::color::fg::red);
        }
        else
        {
            if (program)
            {
                Analysis analysis;
                analysis.options.useDeclaration = true;
                analyzeProgram(program.get(), analysis);
                reportAnalysis(analysis);
                returnValue = 0;
            }
            else
            {
                std::cerr << "Could not create program tree" << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "No file specified" << std::endl;
    }
    finalize();
    return returnValue;
}
catch (std::runtime_error &e)
{
    std::cerr << util::color::colorize(e.what(), util::color::fg::red);
    return -1;
}