/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Typing.h"
#include "Lexer.h"
#include "Parser.h"
#include "Evaluator.h" // to get access to the builtins

namespace typing
{
    ast::TypeExpression *AnalysisContext::findType(const std::string &name) const
    {
        auto foundType = context.find(name);
        if (foundType != context.end())
            return foundType->second.get();

        if (!outer)
            return nullptr;
        return outer->findType(name);
    }

    ast::TypeLiteral *AnalysisContext::findTypeDefinition(const std::string &name) const
    {
        auto foundType = typeContext.find(name);
        if (foundType != typeContext.end())
            return foundType->second;

        if (!outer)
            return nullptr;
        return outer->findTypeDefinition(name);
    }

    bool CompareTypes::operator()(ast::TypeExpression *a, ast::TypeExpression *b) const
    {
        if (a == b)
            return true;

        if (a->type != b->type)
            return false;

        return a->text() < b->text();
    };

    // sort the choices in alfabetical order, to make stringified comparisons easier
    void makeCanonical(ast::TypeChoice *choice)
    {
        std::sort(choice->choices.begin(), choice->choices.end(), [](const auto &typeA, const auto &typeB)
                  { return CompareTypes()(typeA.get(), typeB.get()); });
    }

    void removeRedundant(ast::TypeChoice *choice)
    {
        std::set<std::string> occurence;
        int redundantElements = 0;
        int length = static_cast<int>(choice->choices.size());
        for (int i = 0; i < (length - redundantElements); ++i)
        {
            auto typeStr = choice->choices[i]->text();
            if (occurence.find(typeStr) != occurence.end())
            {
                std::swap(choice->choices[i], choice->choices[length - 1 - redundantElements]);
                redundantElements += 1;
                i -= 1; // revisit the current element as we swapped the last element to it
            }
            else
            {
                occurence.insert(typeStr);
            }
        }
        choice->choices.resize(length - redundantElements);
        makeCanonical(choice);
    }

    std::unique_ptr<ast::TypeExpression>
    mergeTypes(std::unique_ptr<ast::TypeExpression> a, std::unique_ptr<ast::TypeExpression> b)
    {
        if (!a)
            return b;

        if (!b)
            return a;

        if (a->text() == b->text())
            return a;

        switch (a->type)
        {
        case ast::NodeType::TypeChoice:
        {
            auto aChoice = static_cast<ast::TypeChoice *>(a.get());
            switch (b->type)
            {
            case ast::NodeType::TypeChoice:
            {
                auto bChoice = static_cast<ast::TypeChoice *>(b.get());
                for (auto &element : bChoice->choices)
                    aChoice->choices.push_back(std::move(element));
                removeRedundant(aChoice);
                return a;
            }
            default:
            {
                aChoice->choices.push_back(std::move(b));
                removeRedundant(aChoice);
                return a;
            }
            }
        }
        }
        auto choiceType = std::make_unique<ast::TypeChoice>();
        choiceType->choices.push_back(std::move(a));
        choiceType->choices.push_back(std::move(b));
        makeCanonical(choiceType.get());
        return choiceType;
    }

    std::unique_ptr<ast::TypeExpression> computeTypeInfixExpression(ast::InfixExpression *node, std::shared_ptr<AnalysisContext> context)
    {
        switch (node->operator_t.type)
        {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::SLASH:
        case TokenType::ASTERISK:
        case TokenType::DOUBLEASTERISK:
        case TokenType::PLUSASSIGN:
        case TokenType::MINUSASSIGN:
        case TokenType::SLASHASSIGN:
        case TokenType::ASTERISKASSIGN:
        {
            // expect left and right to be of same type
            auto leftType = computeType(node->left.get(), context);
            auto rightType = computeType(node->right.get(), context);
            if (leftType && rightType && leftType->text() == rightType->text())
            {
                return leftType;
            }
        }
        case TokenType::PERCENT:
        {
            // expect left and right to be of same type
            auto leftType = computeType(node->left.get(), context);
            auto rightType = computeType(node->right.get(), context);
            if (leftType && rightType && leftType->text() == rightType->text() && leftType->text() == "int")
            {
                return leftType;
            }
        }
        case TokenType::EQ:
        case TokenType::N_EQ:
        case TokenType::LTEQ:
        case TokenType::GTEQ:
        case TokenType::LT:
        case TokenType::GT:
        {
            // expect left and right to be of same type
            auto leftType = computeType(node->left.get(), context);
            auto rightType = computeType(node->right.get(), context);
            if (leftType && rightType && leftType->text() == rightType->text())
            {
                return std::make_unique<ast::TypeIdentifier>("bool");
            }
        }
        case TokenType::DOUBLEPIPE:
        case TokenType::DOUBLEAMPERSAND:
        {
            // expect left and right to be of same type
            auto leftType = computeType(node->left.get(), context);
            auto rightType = computeType(node->right.get(), context);
            if (leftType && rightType && leftType->text() == rightType->text() && leftType->text() == "bool")
            {
                return std::make_unique<ast::TypeIdentifier>("bool");
            }
        }
        };

        return nullptr;
    }

    std::unique_ptr<ast::TypeExpression> computeReturnType(ast::BlockStatement *node, std::shared_ptr<AnalysisContext> context, bool lastStatementIsImplicitReturn)
    {
        if (!node)
            return nullptr;

        std::unique_ptr<ast::TypeExpression> returnType = nullptr;
        for (auto &stmt : node->statements)
        {
            switch (stmt->type)
            {
            case ast::NodeType::ReturnStatement:
            {
                auto returnStatement = static_cast<ast::ReturnStatement *>(stmt.get());
                auto thisReturnType = computeType(returnStatement->returnValue.get(), context);
                returnType = mergeTypes(std::move(returnType), std::move(thisReturnType));
                break;
            }
            case ast::NodeType::LetStatement:
            {
                auto letStatement = static_cast<ast::LetStatement *>(stmt.get());
                if (letStatement->valueType)
                {
                    context->context[letStatement->name.value] = letStatement->valueType->clone();
                }
                else
                {
                    auto computedType = typing::computeType(letStatement->value.get(), context);
                    if (computedType)
                        context->context[letStatement->name.value] = std::move(computedType);
                }
                break;
            }
            case ast::NodeType::BlockStatement:
            {
                auto newScopeContext = std::make_shared<typing::AnalysisContext>();
                newScopeContext->outer = context;
                auto block = static_cast<ast::BlockStatement *>(stmt.get());
                auto thisReturnType = computeReturnType(block, newScopeContext, false);
                returnType = mergeTypes(std::move(returnType), std::move(thisReturnType));
                break;
            }
            case ast::NodeType::ExpressionStatement:
            {
                auto exprStmt = static_cast<ast::ExpressionStatement *>(stmt.get());
                switch (exprStmt->expression->type)
                {
                case ast::NodeType::IfExpression:
                {
                    auto newScopeContext = std::make_shared<typing::AnalysisContext>();
                    newScopeContext->outer = context;
                    auto ifExpr = static_cast<ast::IfExpression *>(exprStmt->expression.get());
                    auto consequenceReturnType = computeReturnType(ifExpr->consequence.get(), newScopeContext, false);
                    returnType = mergeTypes(std::move(returnType), std::move(consequenceReturnType));

                    newScopeContext = std::make_shared<typing::AnalysisContext>();
                    newScopeContext->outer = context;
                    auto alternativeReturnType = computeReturnType(ifExpr->alternative.get(), context, false);
                    returnType = mergeTypes(std::move(returnType), std::move(alternativeReturnType));
                    break;
                }
                case ast::NodeType::ForExpression:
                {
                    auto newScopeContext = std::make_shared<typing::AnalysisContext>();
                    newScopeContext->outer = context;
                    auto forExpr = static_cast<ast::ForExpression *>(exprStmt->expression.get());

                    auto computedTypeIterable = typing::computeType(forExpr->iterable.get(), context);
                    if (computedTypeIterable)
                    {
                        auto computedTypeIter = typing::computeIndexedType(computedTypeIterable.get(), context);
                        if (computedTypeIter)
                        {
                            newScopeContext->context[forExpr->name.value] = std::move(computedTypeIter);
                        }
                    }
                    auto forReturnType = computeReturnType(forExpr->statement.get(), newScopeContext, false);
                    returnType = mergeTypes(std::move(returnType), std::move(forReturnType));
                    break;
                }
                case ast::NodeType::WhileExpression:
                {
                    auto newScopeContext = std::make_shared<typing::AnalysisContext>();
                    newScopeContext->outer = context;
                    auto whileExpr = static_cast<ast::WhileExpression *>(exprStmt->expression.get());
                    auto whileReturnType = computeReturnType(whileExpr->statement.get(), newScopeContext, false);
                    returnType = mergeTypes(std::move(returnType), std::move(whileReturnType));
                    break;
                }
                }
                auto returnStatement = static_cast<ast::ReturnStatement *>(stmt.get());
                break;
            }
            }
        }

        if (!lastStatementIsImplicitReturn)
            return returnType;

        // the last statement, can also be an expression statement and that is also valid as return value
        if (!node->statements.empty())
        {
            switch (node->statements.back()->type)
            {
            case ast::NodeType::ExpressionStatement:
            {
                auto exprStmt = static_cast<ast::ExpressionStatement *>(node->statements.back().get());
                auto lastStmtType = computeType(exprStmt->expression.get(), context);
                returnType = mergeTypes(std::move(returnType), std::move(lastStmtType));
                break;
            }
            case ast::NodeType::LetStatement:
            {
                auto lastStmtType = std::make_unique<ast::TypeNull>();
                returnType = mergeTypes(std::move(returnType), std::move(lastStmtType));
                break;
            }
            }
        }
        return returnType;
    }

    std::unique_ptr<ast::TypeExpression> computeIndexedType(ast::TypeExpression *type, std::shared_ptr<AnalysisContext> context)
    {
        switch (type->type)
        {
        case ast::NodeType::TypeArray:
        {
            auto arrayType = static_cast<ast::TypeArray *>(type);
            return arrayType->elementType->clone();
        }
        case ast::NodeType::TypeDictionary:
        {
            auto dictType = static_cast<ast::TypeDictionary *>(type);
            return dictType->valueType->clone();
        }
        case ast::NodeType::TypeSet:
        {
            auto setType = static_cast<ast::TypeSet *>(type);
            return setType->elementType->clone();
        }
        case ast::NodeType::TypeIdentifier:
        {
            auto identifierType = static_cast<ast::TypeIdentifier *>(type);
            if (identifierType->value == "range")
                return std::make_unique<ast::TypeIdentifier>("int");
            break;
        }
        }
        return nullptr;
    }

    std::unique_ptr<ast::TypeExpression> computeType(ast::Node *node, std::shared_ptr<AnalysisContext> context)
    {
        if (!node)
            return nullptr;

        switch (node->type)
        {
        case ast::NodeType::Identifier:
        {
            auto identifierNode = static_cast<ast::Identifier *>(node);
            auto foundType = context->findType(identifierNode->value);
            if (foundType)
                return foundType->clone();
            return nullptr;
        }

        case ast::NodeType::BooleanLiteral:
            return std::make_unique<ast::TypeIdentifier>("bool");
        case ast::NodeType::IntegerLiteral:
            return std::make_unique<ast::TypeIdentifier>("int");
        case ast::NodeType::DoubleLiteral:
            return std::make_unique<ast::TypeIdentifier>("double");
        case ast::NodeType::ComplexLiteral:
            return std::make_unique<ast::TypeIdentifier>("complex");
        case ast::NodeType::StringLiteral:
            return std::make_unique<ast::TypeIdentifier>("str");
        case ast::NodeType::NullLiteral:
            return std::make_unique<ast::TypeIdentifier>("null");
        case ast::NodeType::ArrayComplexLiteral:
        {
            auto arrayType = std::make_unique<ast::TypeArray>();
            arrayType->elementType = std::make_unique<ast::TypeIdentifier>("complex");
            return arrayType;
        }
        case ast::NodeType::ArrayDoubleLiteral:
        {
            auto arrayType = std::make_unique<ast::TypeArray>();
            arrayType->elementType = std::make_unique<ast::TypeIdentifier>("double");
            return arrayType;
        }
        case ast::NodeType::ArrayLiteral:
        {
            auto arrayType = std::make_unique<ast::TypeArray>();
            auto arrayNode = static_cast<ast::ArrayLiteral *>(node);
            if (arrayNode->elements.empty())
            {
                arrayType->elementType = std::make_unique<ast::TypeAll>();
                return arrayType;
            }
            auto elementType = computeType(arrayNode->elements.front().get(), context);
            for (size_t i = 1; i < arrayNode->elements.size(); ++i)
            {
                auto nextElementType = computeType(arrayNode->elements[i].get(), context);
                elementType = mergeTypes(std::move(elementType), std::move(nextElementType)); // merge and reduce the types
            }
            arrayType->elementType = std::move(elementType);
            return arrayType;
        }
        case ast::NodeType::DictLiteral:
        {
            auto dictType = std::make_unique<ast::TypeDictionary>();
            auto dictNode = static_cast<ast::DictLiteral *>(node);
            if (dictNode->elements.empty())
            {
                dictType->keyType = std::make_unique<ast::TypeAll>();
                dictType->valueType = std::make_unique<ast::TypeAll>();
                return dictType;
            }

            auto elementIt = dictNode->elements.begin();
            auto keyType = computeType(elementIt->first.get(), context);
            auto valueType = computeType(elementIt->second.get(), context);

            ++elementIt;
            for (; elementIt != dictNode->elements.end(); ++elementIt)
            {
                auto nextKeyType = computeType(elementIt->first.get(), context);
                keyType = mergeTypes(std::move(keyType), std::move(nextKeyType)); // merge and reduce the types

                auto nextValueType = computeType(elementIt->second.get(), context);
                valueType = mergeTypes(std::move(valueType), std::move(nextValueType)); // merge and reduce the types
            }
            dictType->keyType = std::move(keyType);
            dictType->valueType = std::move(valueType);
            return dictType;
        }
        case ast::NodeType::SetLiteral:
        {
            auto setType = std::make_unique<ast::TypeSet>();
            auto setNode = static_cast<ast::SetLiteral *>(node);
            if (setNode->elements.empty())
            {
                setType->elementType = std::make_unique<ast::TypeAll>();
                return setType;
            }

            auto elementIt = setNode->elements.begin();
            auto elementType = computeType(elementIt->get(), context);
            for (; elementIt != setNode->elements.end(); ++elementIt)
            {
                auto nextElementType = computeType(elementIt->get(), context);
                elementType = mergeTypes(std::move(elementType), std::move(nextElementType)); // merge and reduce the types
            }
            setType->elementType = std::move(elementType);
            return setType;
        }
        case ast::NodeType::InfixExpression:
        {
            return computeTypeInfixExpression(static_cast<ast::InfixExpression *>(node), context);
        }
        case ast::NodeType::FunctionLiteral:
        {
            auto funcLiteral = static_cast<ast::FunctionLiteral *>(node);
            auto funcType = std::make_unique<ast::TypeFunction>();

            // to compute the return type we can use the provided, declared arguments
            auto newScopeContext = std::make_shared<typing::AnalysisContext>();
            newScopeContext->outer = context;
            for (size_t i = 0; i < funcLiteral->arguments.size(); ++i)
            {
                if (i < funcLiteral->argumentTypes.size() && funcLiteral->argumentTypes[i])
                    newScopeContext->context.insert_or_assign(funcLiteral->arguments[i].value, funcLiteral->argumentTypes[i]->clone());
            }

            funcType->returnType = computeReturnType(funcLiteral->body.get(), newScopeContext, true);
            for (const auto &argType : funcLiteral->argumentTypes)
            {
                // cannot yet compute the argument types, we can know the declaration
                // but that is not the point here, we want to compute it and make a
                // separation between computed and declared types
                funcType->argTypes.push_back(std::make_unique<ast::TypeAll>());
            }
            return funcType;
        }
        case ast::NodeType::CallExpression:
        {
            auto callExpr = static_cast<ast::CallExpression *>(node);
            auto &callFuncExpr = callExpr->function;
            if (callFuncExpr && callFuncExpr->type == ast::NodeType::Identifier)
            {
                auto callIdentifier = static_cast<ast::Identifier *>(callFuncExpr.get());

                // builtins do take precedence
                auto foundBuiltin = getBuiltin(callIdentifier->value);
                if (foundBuiltin && foundBuiltin->type == obj::ObjectType::Builtin)
                {
                    auto builtinFunc = static_cast<obj::Builtin *>(foundBuiltin.get());
                    if (builtinFunc->declaredType && builtinFunc->declaredType->type == ast::NodeType::TypeFunction)
                    {
                        auto functionType = static_cast<ast::TypeFunction *>(builtinFunc->declaredType);
                        if (functionType->returnType)
                            return functionType->returnType->clone();
                        return nullptr;
                    }
                    return nullptr;
                }

                auto foundIdentifier = context->findType(callIdentifier->value);
                if (foundIdentifier)
                {
                    switch (foundIdentifier->type)
                    {
                    case ast::NodeType::TypeFunction:
                    {
                        auto funcFound = static_cast<ast::TypeFunction *>(foundIdentifier);
                        if (funcFound->returnType)
                            return funcFound->returnType->clone();
                        return nullptr;
                    }
                    case ast::NodeType::TypeType:
                    {
                        auto typeFound = static_cast<ast::TypeType *>(foundIdentifier);
                        return typeFound->clone();
                    }
                    }
                }
            }
            else if (callFuncExpr && callFuncExpr->type == ast::NodeType::MemberExpression)
            {
                auto memberExpr = static_cast<ast::MemberExpression *>(callFuncExpr.get());
                auto computedMemberExpr = computeType(memberExpr->expr.get(), context);
                if (computedMemberExpr && computedMemberExpr->type == ast::NodeType::TypeType)
                {
                    auto customType = context->findType(computedMemberExpr->text());
                    if (customType && customType->type == ast::NodeType::TypeType)
                    {
                        auto typeType = static_cast<ast::TypeType *>(customType);
                        auto typeLiteral = context->findTypeDefinition(computedMemberExpr->text());
                        // [TODO] find the value in the definitions, should be done more encapsulated
                        for (const auto &definition : typeLiteral->definitions)
                        {
                            if (definition->name.value == memberExpr->value.value)
                            {
                                if (definition->value->type == ast::NodeType::FunctionLiteral)
                                {
                                    auto funcLiteral = static_cast<ast::FunctionLiteral *>(definition->value.get());
                                    return computeReturnType(funcLiteral->body.get(), context, true);
                                }
                                else
                                {
                                    // cannot call a non-function
                                }
                            }
                        }
                    }
                }
                return nullptr;
            }
            return nullptr;
        }
        case ast::NodeType::MemberExpression:
        {
            auto memberExpr = static_cast<ast::MemberExpression *>(node);
            auto computedMemberExpr = computeType(memberExpr->expr.get(), context);
            if (computedMemberExpr && computedMemberExpr->type == ast::NodeType::TypeType)
            {
                auto customType = context->findType(computedMemberExpr->text());
                if (customType && customType->type == ast::NodeType::TypeType)
                {
                    auto typeType = static_cast<ast::TypeType *>(customType);
                    auto typeLiteral = context->findTypeDefinition(computedMemberExpr->text());
                    // [TODO] find the value in the definitions, should be done more encapsulated
                    for (const auto &definition : typeLiteral->definitions)
                    {
                        if (definition->name.value == memberExpr->value.value)
                        {
                            if (definition->exprType)
                                return definition->exprType->clone();
                            return computeType(definition->value.get(), context);
                        }
                    }
                }
            }
            return nullptr;
        }
        case ast::NodeType::IndexExpression:
        {
            auto indexExpr = static_cast<ast::IndexExpression *>(node)->expression.get();
            switch (indexExpr->type)
            {
            case ast::NodeType::Identifier:
            {
                auto indexIdentifier = static_cast<ast::Identifier *>(indexExpr);
                auto identifierFound = context->findType(indexIdentifier->value);
                if (identifierFound)
                {
                    return computeIndexedType(identifierFound, context);
                }
            }
            break;
            default:
            {
                auto computedTypeOfOuter = computeType(indexExpr, context);
                if (computedTypeOfOuter)
                {
                    return computeIndexedType(computedTypeOfOuter.get(), context);
                }
            }
            }
            return nullptr;
        }
        case ast::NodeType::ExpressionStatement:
        {
            auto exprStmt = static_cast<ast::ExpressionStatement *>(node);
            return computeType(exprStmt->expression.get(), context);
        }
        case ast::NodeType::LetStatement:
        case ast::NodeType::ForExpression:
        case ast::NodeType::WhileExpression:
        case ast::NodeType::IfExpression:
            return std::make_unique<ast::TypeIdentifier>("null");
            // [TODO] what about the other expression types?
        }

        return nullptr;
    }

    std::unique_ptr<ast::TypeExpression> computeType(obj::Object *obj)
    {
        if (obj->declaredType)
            return obj->declaredType->clone();

        switch (obj->type)
        {
        case obj::ObjectType::Null:
        case obj::ObjectType::Integer:
        case obj::ObjectType::Double:
        case obj::ObjectType::Complex:
        case obj::ObjectType::Boolean:
        case obj::ObjectType::String:
        case obj::ObjectType::Error:
        case obj::ObjectType::Range:
        case obj::ObjectType::Regex:
        case obj::ObjectType::IOObject:
        {
            std::map<obj::ObjectType, std::string> builtInRevTypeMapping = {
                {obj::ObjectType::Null, "null"},
                {obj::ObjectType::Integer, "int"},
                {obj::ObjectType::Complex, "complex"},
                {obj::ObjectType::Double, "double"},
                {obj::ObjectType::Boolean, "bool"},
                {obj::ObjectType::String, "str"},
                {obj::ObjectType::Error, "error"},
                {obj::ObjectType::IOObject, "io"},
                {obj::ObjectType::Module, "module"},
                {obj::ObjectType::Thread, "thread"},
                {obj::ObjectType::Range, "range"},
                {obj::ObjectType::Regex, "regex"},
            };
            auto typeIdentifier = std::make_unique<ast::TypeIdentifier>();
            typeIdentifier->value = builtInRevTypeMapping.at(obj->type);
            return std::move(typeIdentifier);
        }
        case obj::ObjectType::Array:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> elementTypes;
            auto array = static_cast<obj::Array *>(obj);
            for (const auto &element : array->value)
            {
                auto typeOfElement = std::move(computeType(element.get()));
                elementTypes.insert_or_assign(typeOfElement.get(), std::move(typeOfElement));
            }
            auto typeArray = std::make_unique<ast::TypeArray>();
            if (elementTypes.size() == 1)
            {
                typeArray->elementType = std::move(elementTypes.begin()->second);
            }
            else
            {
                auto typeChoice = std::make_unique<ast::TypeChoice>();
                for (auto &[k, v] : elementTypes)
                    typeChoice->choices.push_back(std::move(v));
                typeArray->elementType = std::move(typeChoice);
            }
            return std::move(typeArray);
        }
        case obj::ObjectType::ArrayDouble:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> elementTypes;
            auto typeArray = std::make_unique<ast::TypeArray>();
            auto doubleTypeIndentifier = std::make_unique<ast::TypeIdentifier>();
            doubleTypeIndentifier->value = "double";
            typeArray->elementType = std::move(doubleTypeIndentifier);
            return std::move(typeArray);
        }
        case obj::ObjectType::ArrayComplex:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> elementTypes;
            auto typeArray = std::make_unique<ast::TypeArray>();
            auto doubleTypeIndentifier = std::make_unique<ast::TypeIdentifier>();
            doubleTypeIndentifier->value = "complex";
            typeArray->elementType = std::move(doubleTypeIndentifier);
            return std::move(typeArray);
        }
        case obj::ObjectType::Dictionary:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> keyElementTypes;
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> valueElementTypes;
            auto dict = static_cast<obj::Dictionary *>(obj);
            for (const auto &[k, v] : dict->value)
            {
                auto keyTypeOfElement = std::move(computeType(k.get()));
                auto valueTypeOfElement = std::move(computeType(v.get()));
                keyElementTypes.insert_or_assign(keyTypeOfElement.get(), std::move(keyTypeOfElement));
                valueElementTypes.insert_or_assign(valueTypeOfElement.get(), std::move(valueTypeOfElement));
            }
            auto typeDict = std::make_unique<ast::TypeDictionary>();
            if (keyElementTypes.size() == 1)
            {
                typeDict->keyType = std::move(keyElementTypes.begin()->second);
            }
            else
            {
                auto typeChoice = std::make_unique<ast::TypeChoice>();
                for (auto &[k, v] : keyElementTypes)
                    typeChoice->choices.push_back(std::move(v));
                typeDict->keyType = std::move(typeChoice);
            }
            if (valueElementTypes.size() == 1)
            {
                typeDict->valueType = std::move(valueElementTypes.begin()->second);
            }
            else
            {
                auto typeChoice = std::make_unique<ast::TypeChoice>();
                for (auto &[k, v] : valueElementTypes)
                    typeChoice->choices.push_back(std::move(v));
                typeDict->valueType = std::move(typeChoice);
            }
            return std::move(typeDict);
        }
        case obj::ObjectType::Set:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> elementTypes;
            auto setObj = static_cast<obj::Set *>(obj);
            for (const auto &element : setObj->value)
            {
                auto typeOfElement = std::move(computeType(element.get()));
                elementTypes.insert_or_assign(typeOfElement.get(), std::move(typeOfElement));
            }
            auto typeSet = std::make_unique<ast::TypeSet>();
            if (elementTypes.size() == 1)
            {
                typeSet->elementType = std::move(elementTypes.begin()->second);
            }
            else
            {
                auto typeChoice = std::make_unique<ast::TypeChoice>();
                for (auto &[k, v] : elementTypes)
                    typeChoice->choices.push_back(std::move(v));
                typeSet->elementType = std::move(typeChoice);
            }
            return std::move(typeSet);
        }
        case obj::ObjectType::Function:
        {
            auto typeFunction = std::make_unique<ast::TypeFunction>();
            auto function = static_cast<obj::Function *>(obj);
            for (const auto &argType : function->argumentTypes)
                typeFunction->argTypes.push_back(argType->clone());

            typeFunction->returnType = function->returnType->clone();
            return std::move(typeFunction);
        }
        case obj::ObjectType::UserType:
        {
            auto typeType = std::make_unique<ast::TypeType>();
            auto userType = static_cast<obj::UserType *>(obj);
            typeType->value = userType->name;
            return std::move(typeType);
        }
        case obj::ObjectType::UserObject:
        {
            auto typeIdentifier = std::make_unique<ast::TypeIdentifier>();
            auto userObj = static_cast<obj::UserObject *>(obj);
            typeIdentifier->value = userObj->userType->name;
            return std::move(typeIdentifier);
        }
        case obj::ObjectType::BoundUserTypeFunction:
        {
            // [TODO] introduce a compound type that can convey the boundedness to a type
            auto typeFunction = std::make_unique<ast::TypeFunction>();
            auto function = static_cast<obj::BoundUserTypeFunction *>(obj);
            for (const auto &argType : function->function->argumentTypes)
                typeFunction->argTypes.push_back(argType->clone());

            typeFunction->returnType = function->function->returnType->clone();

            auto boundType = std::make_unique<ast::BoundType>();
            boundType->boundTo = computeType(function->boundTo.get());
            boundType->boundType = std::move(typeFunction);
            return std::move(boundType);
        }
        case obj::ObjectType::BoundUserTypeProperty:
        {
            // [TODO] introduce a compound type that can convey the boundedness to a type
            auto property = static_cast<obj::BoundUserTypeProperty *>(obj);
            return computeType(property->property->obj.get());
        }
        case obj::ObjectType::BoundBuiltinTypeFunction:
        {
            auto function = static_cast<obj::BoundBuiltinTypeFunction *>(obj);
            auto boundType = std::make_unique<ast::BoundType>();
            boundType->boundTo = computeType(function->boundTo.get());
            boundType->boundType = function->functionType->clone();
            return std::move(boundType);
        }
        case obj::ObjectType::BoundBuiltinTypeProperty:
        {
            // [TODO] introduce a compound type that can convey the boundedness to a type
            auto property = static_cast<obj::BoundBuiltinTypeProperty *>(obj);
            return computeType(property->property->obj.get());
        }
        }
        return std::unique_ptr<ast::TypeExpression>(nullptr);
    }

    /* returns true when type1 is compatible with type2
     *  In general a type1 is compatible with type2 if it is a subset of type2
     *   For instance
     *       isCompatible( "double", "double") -> True
     *       isCompatible( "double", "all") -> True
     *       isCompatible( "all", "double") -> False         all is not a subset of double
     *       isCompatible( "any", "double") -> True          any can be a subset of double
     *       isCompatible( "any", "any") -> True             any is a subset of any
     *       isCompatible( "double", "any") -> True          double is a subset of any
     *       isCompatible( "any", "all") -> True             any is a subset of all
     *       isCompatible( "all", "any") -> False            all is not a subset of any
     *       isCompatible( "double", "<double>") -> True            double is a subset of a choice of double
     *       isCompatible( "double", "<double,int>") -> True            double is a subset of a choice of double, int
     *       isCompatible( "<double,int>", "<double,int>") -> True            double is a subset of a choice of double, int
     */
    bool isCompatibleType(ast::TypeExpression *type1, ast::TypeExpression *type2)
    {
        if (type1 == nullptr && type2 == nullptr)
            return true;

        if (type1->type == ast::NodeType::TypeAny)
            return true;

        if (type2 == nullptr)
            return false;

        switch (type1->type)
        {
        case ast::NodeType::TypeChoice:
        {
            // all the choices must match in the type2 or there is an incompatibility possible
            const auto &type1Choices = static_cast<ast::TypeChoice *>(type1)->choices;
            for (const auto &choice : type1Choices)
            {
                if (!isCompatibleType(choice.get(), type2))
                    return false;
            }
            return true;
        }
        }

        switch (type2->type)
        {
        case ast::NodeType::TypeAll:
            return true;
        case ast::NodeType::TypeAny:
            return type1->type != ast::NodeType::TypeAll;
        case ast::NodeType::TypeNull:
            return type1->type == type2->type;
        case ast::NodeType::TypeIdentifier:
        {
            if (type1->type != type2->type)
                return false;
            auto type1IdentifierValue = static_cast<ast::TypeIdentifier *>(type1)->value;
            auto type2IdentifierValue = static_cast<ast::TypeIdentifier *>(type2)->value;
            return type1IdentifierValue == type2IdentifierValue;
        }
        case ast::NodeType::TypeChoice:
        {
            const auto &type2Choices = static_cast<ast::TypeChoice *>(type2)->choices;
            for (const auto &choice : type2Choices)
            {
                if (isCompatibleType(type1, choice.get()))
                    return true;
            }
            return false;
        }
        case ast::NodeType::TypeArray:
        {
            if (type1->type != type2->type)
                return false;
            auto type1Array = static_cast<ast::TypeArray *>(type1);
            auto type2Array = static_cast<ast::TypeArray *>(type2);
            return isCompatibleType(type1Array->elementType.get(), type2Array->elementType.get());
        }
        case ast::NodeType::TypeDictionary:
        {
            if (type1->type != type2->type)
                return false;
            auto type1Dict = static_cast<ast::TypeDictionary *>(type1);
            auto type2Dict = static_cast<ast::TypeDictionary *>(type2);
            if (!isCompatibleType(type1Dict->keyType.get(), type2Dict->keyType.get()))
                return false;
            if (!isCompatibleType(type1Dict->valueType.get(), type2Dict->valueType.get()))
                return false;
            return true;
        }
        }
        throw std::runtime_error("Failed to compute type compatibility");
    }

    bool isCompatibleType(ast::TypeExpression *type, obj::Object *obj, obj::Object *existingObj)
    {
        if (type == nullptr)
            return true;

        if (obj == nullptr)
            return false; // [TODO] should this be an exception?  This should not be occuring

        switch (type->type)
        {
        case ast::NodeType::TypeAll:
            return true;
        case ast::NodeType::TypeAny:
        {
            if (existingObj == nullptr)
                return true;
            // extract the type from the existingObj and then compare it to the type
            auto existingType = computeType(existingObj);
            return isCompatibleType(existingType.get(), obj, nullptr);
        }
        case ast::NodeType::TypeNull:
            return obj->type == obj::ObjectType::Null;
        case ast::NodeType::TypeIdentifier:
        {
            auto typeIdentifierValue = static_cast<ast::TypeIdentifier *>(type)->value;
            std::map<std::string, obj::ObjectType> builtInTypeMapping = {
                {"null", obj::ObjectType::Null},
                {"int", obj::ObjectType::Integer},
                {"double", obj::ObjectType::Double},
                {"complex", obj::ObjectType::Complex},
                {"bool", obj::ObjectType::Boolean},
                {"str", obj::ObjectType::String},
                {"error", obj::ObjectType::Error},
                {"io", obj::ObjectType::IOObject},
                {"module", obj::ObjectType::Module},
                {"thread", obj::ObjectType::Thread},
                {"regex", obj::ObjectType::Regex},
                {"range", obj::ObjectType::Range},
            };
            auto expectedObjType = builtInTypeMapping.find(typeIdentifierValue);
            if (expectedObjType == builtInTypeMapping.end())
                return false;
            return expectedObjType->second == obj->type;
        }
        case ast::NodeType::TypeChoice:
        {
            auto typeChoice = static_cast<ast::TypeChoice *>(type);
            for (const auto &aType : typeChoice->choices)
            {
                if (isCompatibleType(aType.get(), obj, existingObj))
                    return true;
            }
            return false;
        }
        case ast::NodeType::TypeArray:
        {
            if (obj->type != obj::ObjectType::Array && obj->type != obj::ObjectType::ArrayDouble && obj->type != obj::ObjectType::ArrayComplex)
                return false;

            auto typeArray = static_cast<ast::TypeArray *>(type);
            switch (obj->type)
            {
            case obj::ObjectType::Array:
            {
                auto array = static_cast<obj::Array *>(obj);
                for (const auto &element : array->value)
                {
                    if (!isCompatibleType(typeArray->elementType.get(), element.get(), nullptr))
                        return false;
                }
                break;
            }
            case obj::ObjectType::ArrayDouble:
            {
                if (typeArray->elementType->type != ast::NodeType::TypeIdentifier)
                    return false;
                return (static_cast<ast::TypeIdentifier *>(typeArray->elementType.get())->value == "double");
            }
            case obj::ObjectType::ArrayComplex:
            {
                if (typeArray->elementType->type != ast::NodeType::TypeIdentifier)
                    return false;
                return (static_cast<ast::TypeIdentifier *>(typeArray->elementType.get())->value == "complex");
            }
            }
            return true;
        }
        case ast::NodeType::TypeDictionary:
        {
            if (obj->type != obj::ObjectType::Dictionary)
                return false;

            auto typeDictionary = static_cast<ast::TypeDictionary *>(type);
            auto dict = static_cast<obj::Dictionary *>(obj);
            for (const auto &[k, v] : dict->value)
            {
                if (!isCompatibleType(typeDictionary->keyType.get(), k.get(), nullptr))
                    return false;

                if (!isCompatibleType(typeDictionary->valueType.get(), v.get(), nullptr))
                    return false;
            }
            return true;
        }
        case ast::NodeType::TypeSet:
        {
            if (obj->type != obj::ObjectType::Set)
                return false;

            auto typeSet = static_cast<ast::TypeSet *>(type);
            auto setObj = static_cast<obj::Set *>(obj);
            for (const auto &element : setObj->value)
            {
                if (!isCompatibleType(typeSet->elementType.get(), element.get(), nullptr))
                    return false;
            }
            return true;
        }
        case ast::NodeType::TypeFunction:
        {
            if (obj->type != obj::ObjectType::Function)
                return false;

            auto typeFunction = static_cast<ast::TypeFunction *>(type);
            auto func = static_cast<obj::Function *>(obj);
            if (!isCompatibleType(typeFunction->returnType.get(), func->returnType))
                return false;

            if (typeFunction->argTypes.size() != func->argumentTypes.size())
                return false;

            for (size_t i = 0; i < typeFunction->argTypes.size(); ++i)
            {
                if (!isCompatibleType(typeFunction->argTypes[i].get(), func->argumentTypes[i]))
                    return false;
            }
            return true;
        }
        }
        return false;
    }

    ast::TypeFunction *makeFunctionType(const std::string &argTypeStr, const std::string &returnTypeStr)
    {
        ast::TypeFunction *functionType = new ast::TypeFunction();
        {
            auto lexer = createLexer(returnTypeStr, "");
            auto parser = createParser(std::move(lexer));
            functionType->returnType = parser->parseTypeExpression(Parser::Precedence::LOWEST);
            if (!functionType->returnType)
                throw std::runtime_error("Internal error: invalid return type string: " + returnTypeStr);
        }
        {
            auto lexer = createLexer("(" + argTypeStr + ")", "");
            auto parser = createParser(std::move(lexer));
            functionType->argTypes = parser->parseTypeExpressionList(TokenType::RPAREN);
            for (const auto &typeExpr : functionType->argTypes)
            {
                if (!typeExpr)
                    throw std::runtime_error("Internal error: invalid arguments type string: " + argTypeStr);
            }
        }

        return functionType;
    }
}
