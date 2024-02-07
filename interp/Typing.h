/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#ifndef GUARDIAN_OF_INCLUSION_TYPING_H
#define GUARDIAN_OF_INCLUSION_TYPING_H

#include "Ast.h"
#include "Object.h"

namespace typing
{
    struct CompareTypes
    {
        bool operator()(ast::TypeExpression *a, ast::TypeExpression *b) const;
    };

    struct AnalysisContext
    {
        std::map<std::string, std::unique_ptr<ast::TypeExpression>> context;
        std::map<std::string, ast::TypeLiteral *> typeContext;
        std::shared_ptr<AnalysisContext> outer;

        ast::TypeExpression *findType(const std::string &name) const;
        ast::TypeLiteral *findTypeDefinition(const std::string &name) const;
    };

    /* compute the type of an expression in the AST, without tracing
     * Returns the most specific type possible.
     * For instance let a = [3, 2.0] will give [<int, double>]
     */
    std::unique_ptr<ast::TypeExpression> computeType(ast::Node *node, std::shared_ptr<AnalysisContext> context);

    /* compute the return type for a block statement, like occurs
     * in function literals
     */
    std::unique_ptr<ast::TypeExpression> computeReturnType(ast::BlockStatement *node, std::shared_ptr<AnalysisContext> context, bool lastStatementIsImplicitReturn);

    /* compute the type when indexed into an expression */
    std::unique_ptr<ast::TypeExpression> computeIndexedType(ast::TypeExpression *type, std::shared_ptr<AnalysisContext> context);

    /* compute the type of an object */
    std::unique_ptr<ast::TypeExpression> computeType(obj::Object *obj);

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
     */
    bool isCompatibleType(ast::TypeExpression *type1, ast::TypeExpression *type2);

    /* computes type compatibility
     *  during assignment to an existing variable, the existingObj is provided for additional context
     */
    bool isCompatibleType(ast::TypeExpression *type, obj::Object *obj, obj::Object *existingObj);

    /* create a function type object based on an arg str and return str
     *   makeFunctionType("int, int","null") gives a function type of "fn(int, int) -> null"
     */
    ast::TypeFunction *makeFunctionType(const std::string &argTypeStr, const std::string &returnTypeStr);
}

#endif