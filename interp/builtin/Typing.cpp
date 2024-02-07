/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Typing.h"
#include "../Evaluator.h"
#include "../Typing.h"
#include "../Lexer.h"
#include "../Parser.h"

namespace builtin
{
    std::shared_ptr<obj::Object> isCompatibleTypStr(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return std::make_shared<obj::Error>("is_compatible_type_str: expected 2 argument of type (str, str)", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, String, "is_compatible_type_str: expected argument 1 to be a str");

        auto evaluatedExpr2 = evalExpression(arguments->back().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr2, String, "is_compatible_type_str: expected argument 2 to be a str");

        auto str1 = static_cast<obj::String *>(evaluatedExpr1.get())->value;
        auto str2 = static_cast<obj::String *>(evaluatedExpr2.get())->value;

        auto lexer1 = createLexer(str1, "");
        auto parser1 = createParser(std::move(lexer1));
        auto typeExpr1 = parser1->parseTypeExpression(Parser::Precedence::LOWEST);

        if (!typeExpr1)
            return std::make_shared<obj::Error>("Cannot parse type str for argument 1", obj::ErrorType::ValueError);

        auto lexer2 = createLexer(str2, "");
        auto parser2 = createParser(std::move(lexer2));
        auto typeExpr2 = parser2->parseTypeExpression(Parser::Precedence::LOWEST);

        if (!typeExpr2)
            return std::make_shared<obj::Error>("Cannot parse type str for argument 2", obj::ErrorType::ValueError);

        bool compatible = typing::isCompatibleType(typeExpr1.get(), typeExpr2.get());
        return std::make_shared<obj::Boolean>(compatible);
    }

    std::shared_ptr<obj::Module> createTypingModule()
    {
        auto typingModule = std::make_shared<obj::Module>();
        typingModule->environment->add("is_compatible_type_str", builtin::makeBuiltInFunctionObj(&builtin::isCompatibleTypStr, "str,str", "bool"), false, nullptr);
        typingModule->state = obj::ModuleState::Loaded;
        return typingModule;
    }
} // namespace builtin
