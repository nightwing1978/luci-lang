/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Regex.h"
#include "../Evaluator.h"
#include "../Typing.h"

namespace builtin
{
    std::shared_ptr<obj::Object> regex(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 2)
            return std::make_shared<obj::Error>("regex: expected 0 or 1 arguments of type (str, int)", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, String, "regex: expected argument 1 to be a str");

        std::regex::flag_type flags = std::regex::ECMAScript;
        if (arguments->size() == 2)
        {
            auto evaluatedExpr2 = evalExpression(arguments->back().get(), environment);
            RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr2, Integer, "regex: expected argument 2 to be an int");

            flags = static_cast<std::regex::flag_type>(static_cast<obj::Integer *>(evaluatedExpr2.get())->value);
        }

        std::string pattern = static_cast<obj::String *>(evaluatedExpr1.get())->value;
        auto regexObj = std::make_shared<obj::Regex>(std::regex(pattern, flags));
        return regexObj;
    }

    std::shared_ptr<obj::Object> match(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return std::make_shared<obj::Error>("match: expected 2 arguments of type (regex, str)", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, Regex, "match: expected argument 1 to be a regex");

        auto evaluatedExpr2 = evalExpression(arguments->back().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr2, String, "match: expected argument 2 to be a str");

        std::string search = static_cast<obj::String *>(evaluatedExpr2.get())->value;
        std::smatch patternMatch;
        bool found = std::regex_match(search, patternMatch, *static_cast<obj::Regex *>(evaluatedExpr1.get())->regex);
        if (!found)
            return NullObject;

        std::vector<std::shared_ptr<obj::Object>> arr;
        for (size_t i = 0; i < patternMatch.size(); ++i)
        {
            arr.push_back(std::make_shared<obj::String>(patternMatch[i].str()));
        }
        return std::make_shared<obj::Array>(arr);
    }

    std::shared_ptr<obj::Object> search(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return std::make_shared<obj::Error>("search: expected 2 arguments of type (regex, str)", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, Regex, "search: expected argument 1 to be a regex");

        auto evaluatedExpr2 = evalExpression(arguments->back().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr2, String, "search: expected argument 2 to be a str");

        std::string search = static_cast<obj::String *>(evaluatedExpr2.get())->value;
        std::smatch patternMatch;
        bool found = std::regex_search(search, patternMatch, *static_cast<obj::Regex *>(evaluatedExpr1.get())->regex);
        if (!found)
            return NullObject;

        std::vector<std::shared_ptr<obj::Object>> arr;
        for (size_t i = 0; i < patternMatch.size(); ++i)
        {
            arr.push_back(std::make_shared<obj::String>(patternMatch[i].str()));
        }
        return std::make_shared<obj::Array>(arr);
    }

    std::shared_ptr<obj::Object> replace(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 3)
            return std::make_shared<obj::Error>("replace: expected 3 arguments of type (regex, str, str)", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, Regex, "replace: expected argument 1 to be a regex");

        auto evaluatedExpr2 = evalExpression(arguments->at(1).get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr2, String, "replace: expected argument 2 to be a str");

        auto evaluatedExpr3 = evalExpression(arguments->at(2).get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr3, String, "replace: expected argument 3 to be a str");

        std::string search = static_cast<obj::String *>(evaluatedExpr2.get())->value;
        std::string replacement = static_cast<obj::String *>(evaluatedExpr3.get())->value;
        std::string result = std::regex_replace(search, *static_cast<obj::Regex *>(evaluatedExpr1.get())->regex, replacement);
        return std::make_shared<obj::String>(result);
    }

    std::shared_ptr<obj::Module> createRegexModule()
    {
        auto regexModule = std::make_shared<obj::Module>();
        regexModule->state = obj::ModuleState::Loaded;
        regexModule->environment->add("regex", builtin::makeBuiltInFunctionObj(&builtin::regex, "", "regex"), false, nullptr);
        regexModule->environment->add("match", builtin::makeBuiltInFunctionObj(&builtin::match, "regex,str", "<null, [str]>"), false, nullptr);
        regexModule->environment->add("search", builtin::makeBuiltInFunctionObj(&builtin::search, "regex,str", "<null, [str]>"), false, nullptr);
        regexModule->environment->add("replace", builtin::makeBuiltInFunctionObj(&builtin::replace, "regex,str,str", "str"), false, nullptr);

        std::map<std::string, std::regex::flag_type> flagTypes = {
            {"icase", std::regex_constants::icase},
            {"nosubs", std::regex_constants::nosubs},
            {"optimize", std::regex_constants::optimize},
            {"collate", std::regex_constants::collate},
            {"ECMAscript", std::regex_constants::ECMAScript},
            {"basic", std::regex_constants::basic},
            {"extended", std::regex_constants::extended},
            {"awk", std::regex_constants::awk},
            {"grep", std::regex_constants::grep},
            {"egrep", std::regex_constants::egrep},
        };

        for (const auto [name, flagType] : flagTypes)
            regexModule->environment->add(name, std::make_shared<obj::Integer>(static_cast<int64_t>(flagType)), true, nullptr);

        return regexModule;
    }
} // namespace builtin
