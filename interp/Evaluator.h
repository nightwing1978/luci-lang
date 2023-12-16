/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#ifndef GUARDIAN_OF_INCLUSION_EVALUATOR_H
#define GUARDIAN_OF_INCLUSION_EVALUATOR_H

#include "Object.h"

std::shared_ptr<obj::Object> evalDestructor(obj::Function *function, obj::UserObject *self, const std::shared_ptr<obj::Environment> &environment);

std::shared_ptr<obj::Object> eval(std::unique_ptr<ast::Node> node, const std::shared_ptr<obj::Environment> &environment);
std::shared_ptr<obj::Object> eval(ast::Node *node, const std::shared_ptr<obj::Environment> &environment);

std::shared_ptr<obj::Object> evalExpression(ast::Expression *expression, const std::shared_ptr<obj::Environment> &environment, ast::TypeExpression *typeHint = nullptr);
std::shared_ptr<obj::Object> evalProgram(ast::Program *program, const std::shared_ptr<obj::Environment> &environment);

/* function that can unwrap a return/member expr value, typically needed after
 * a call to eval or evalExpression */
std::shared_ptr<obj::Object> unwrap(const std::shared_ptr<obj::Object> &object);

std::shared_ptr<obj::Object> evalFunctionWithArguments(obj::Function *functionObj, const std::vector<std::shared_ptr<obj::Object>> &evaluatedArgs, const std::shared_ptr<obj::Environment> &environment);

/* initialize the internal structures from the outside environment
 * so that the interpreter can return arg when requested
 */
void initializeArg(int argc, char **argv);

/* perform initialization tasks, such as populating the
 * internal types and built-in functions
 */
void initialize();

/* perform finalization tasks, such as clearing the
 * internal types and built-in functions
 */
void finalize();

/* shared NullObject that can be pointed to instead of being re-allocated all the time*/
extern std::shared_ptr<obj::Object> NullObject;

namespace builtin
{
    std::shared_ptr<obj::Object> makeBuiltInFunctionObj(obj::TBuiltinFunction fn, const std::string &argTypeStr, const std::string &returnTypeStr);
}

#define RETURN_TYPE_ERROR_ON_MISMATCH(expr, expectedType, msg) \
    if (expr->type == obj::ObjectType::Error)                  \
        return expr;                                           \
    if (expr->type != obj::ObjectType::expectedType)           \
        return obj::makeTypeError(msg);

#endif