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