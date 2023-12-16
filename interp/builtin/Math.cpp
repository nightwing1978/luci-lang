/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Math.h"
#include "../Evaluator.h"
#include "../Typing.h"

namespace
{
    typedef double (*TBuiltinDoubleFunction)(double arg);

    template <TBuiltinDoubleFunction double_fn>
    std::shared_ptr<obj::Object> r_double_double_function(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        auto errorObj = dynamic_cast<obj::Error *>(evaluatedExpr.get());
        if (errorObj)
            return evaluatedExpr;

        auto doubleObj = dynamic_cast<obj::Double *>(evaluatedExpr.get());
        if (doubleObj)
            return std::make_shared<obj::Double>(double_fn(doubleObj->value));

        return std::make_shared<obj::Error>(obj::Error("Invalid type for function, expected double, got: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError));
    }
}

namespace builtin
{
    std::shared_ptr<obj::Object> pow_function(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return std::make_shared<obj::Error>("expected 2 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->at(0).get(), environment);
        auto errorObj = dynamic_cast<obj::Error *>(evaluatedExpr.get());
        if (errorObj)
            return evaluatedExpr;

        auto doubleObj = dynamic_cast<obj::Double *>(evaluatedExpr.get());
        if (!doubleObj)
            return std::make_shared<obj::Error>(obj::Error("Invalid type for function, expected double, got: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError));

        auto evaluatedSecondExpr = evalExpression(arguments->at(1).get(), environment);
        auto errorObjSecond = dynamic_cast<obj::Error *>(evaluatedSecondExpr.get());
        if (errorObjSecond)
            return evaluatedSecondExpr;

        auto doubleSecondObj = dynamic_cast<obj::Double *>(evaluatedSecondExpr.get());
        if (!doubleSecondObj)
            return std::make_shared<obj::Error>(obj::Error("Invalid type for function, expected double, got: " + obj::toString(evaluatedSecondExpr->type), obj::ErrorType::TypeError));

        return std::make_shared<obj::Double>(pow(doubleObj->value, doubleSecondObj->value));
    }

    template <TBuiltinDoubleFunction double_fn>
    std::shared_ptr<obj::Object> makeBuiltInDoubleFunctionObj()
    {
        auto func = std::make_shared<obj::Builtin>();
        func->function = &r_double_double_function<double_fn>;
        func->declaredType = typing::makeFunctionType("double", "double");
        return func;
    }

    std::shared_ptr<obj::Module> createMathModule()
    {
        auto mathModule = std::make_shared<obj::Module>();
        mathModule->environment->add("abs", builtin::makeBuiltInDoubleFunctionObj<fabs>(), false, nullptr);
        mathModule->environment->add("acos", builtin::makeBuiltInDoubleFunctionObj<acos>(), false, nullptr);
        mathModule->environment->add("asin", builtin::makeBuiltInDoubleFunctionObj<asin>(), false, nullptr);
        mathModule->environment->add("atan", builtin::makeBuiltInDoubleFunctionObj<atan>(), false, nullptr);
        mathModule->environment->add("cbrt", builtin::makeBuiltInDoubleFunctionObj<cbrt>(), false, nullptr);
        mathModule->environment->add("cos", builtin::makeBuiltInDoubleFunctionObj<cos>(), false, nullptr);
        mathModule->environment->add("erf", builtin::makeBuiltInDoubleFunctionObj<erf>(), false, nullptr);
        mathModule->environment->add("erfc", builtin::makeBuiltInDoubleFunctionObj<erfc>(), false, nullptr);
        mathModule->environment->add("exp", builtin::makeBuiltInDoubleFunctionObj<exp>(), false, nullptr);
        mathModule->environment->add("lgamma", builtin::makeBuiltInDoubleFunctionObj<lgamma>(), false, nullptr);
        mathModule->environment->add("log", builtin::makeBuiltInDoubleFunctionObj<log>(), false, nullptr);
        mathModule->environment->add("log10", builtin::makeBuiltInDoubleFunctionObj<log10>(), false, nullptr);
        mathModule->environment->add("round", builtin::makeBuiltInDoubleFunctionObj<round>(), false, nullptr);
        mathModule->environment->add("sin", builtin::makeBuiltInDoubleFunctionObj<sin>(), false, nullptr);
        mathModule->environment->add("sqrt", builtin::makeBuiltInDoubleFunctionObj<sqrt>(), false, nullptr);
        mathModule->environment->add("tan", builtin::makeBuiltInDoubleFunctionObj<tan>(), false, nullptr);
        mathModule->environment->add("tgamma", builtin::makeBuiltInDoubleFunctionObj<tgamma>(), false, nullptr);
        mathModule->environment->add("trunc", builtin::makeBuiltInDoubleFunctionObj<trunc>(), false, nullptr);
        mathModule->environment->add("pow", builtin::makeBuiltInFunctionObj(&builtin::pow_function, "double, double", "double"), false, nullptr);
        mathModule->state = obj::ModuleState::Loaded;
        return mathModule;
    }
} // namespace builtin
