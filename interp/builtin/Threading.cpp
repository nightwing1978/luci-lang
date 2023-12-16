/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Threading.h"
#include "../Evaluator.h"
#include "../Typing.h"

namespace builtin
{
    std::shared_ptr<obj::Object> thread(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 2)
            return std::make_shared<obj::Error>("thread: expected 0 or 1 argument of type (func, all)", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, Function, "thread: expected argument 1 to be a function");

        auto threadObj = std::make_shared<obj::Thread>();
        threadObj->function = std::dynamic_pointer_cast<obj::Function>(evaluatedExpr1);

        if (arguments->size() == 2)
        {
            auto evaluatedExpr2 = evalExpression(arguments->back().get(), environment);
            threadObj->argument = evaluatedExpr2;
        }

        return threadObj;
    }

    std::shared_ptr<obj::Object> sleep(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("sleep: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, Double, "sleep: expected argument 1 to be a double");

        int64_t nanoseconds = static_cast<int64_t>(1e9 * static_cast<obj::Double *>(evaluatedExpr1.get())->value);
        std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
        return NullObject;
    }

    std::shared_ptr<obj::Module> createThreadingModule()
    {
        auto threadingModule = std::make_shared<obj::Module>();
        threadingModule->environment->add("thread", builtin::makeBuiltInFunctionObj(&builtin::thread, "", "thread"), false, nullptr);
        threadingModule->environment->add("sleep", builtin::makeBuiltInFunctionObj(&builtin::sleep, "double", "null"), false, nullptr);
        threadingModule->state = obj::ModuleState::Loaded;
        return threadingModule;
    }
} // namespace builtin
