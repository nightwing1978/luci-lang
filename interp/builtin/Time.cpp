/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Time.h"
#include "../Evaluator.h"
#include "../Typing.h"

namespace builtin
{
    std::shared_ptr<obj::Object> time_(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 0)
            return std::make_shared<obj::Error>("time: expected 0 arguments", obj::ErrorType::TypeError);

        const auto timeP = std::chrono::steady_clock::now();
        return std::make_shared<obj::Double>(static_cast<double>(timeP.time_since_epoch().count()) * 1e-9);
    }

    std::shared_ptr<obj::Module> createTimeModule()
    {
        auto timeModule = std::make_shared<obj::Module>();
        timeModule->environment->add("time", builtin::makeBuiltInFunctionObj(&builtin::time_, "", "double"), false, nullptr);
        timeModule->state = obj::ModuleState::Loaded;
        return timeModule;
    }
} // namespace builtin
