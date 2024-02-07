/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "String.h"
#include "../Util.h"
#include "../Typing.h"

#include <algorithm>

namespace
{
    std::shared_ptr<obj::Object>
    validateArguments(
        const std::string &errorPrefix,
        const std::shared_ptr<obj::Object> &self,
        const std::vector<std::shared_ptr<obj::Object>> &arguments,
        const obj::ObjectType expectedType,
        std::set<size_t> nrExpectedArguments)
    {
        if (self.get()->type != expectedType)
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + toString(expectedType) + ", got " + toString(self.get()->type), obj::ErrorType::TypeError);
        if (nrExpectedArguments.find(arguments.size()) == nrExpectedArguments.end())
        {
            if (nrExpectedArguments.size() == 1)
            {
                return std::make_shared<obj::Error>(errorPrefix + ": expected " + std::to_string(*nrExpectedArguments.begin()) + " arguments, got " + std::to_string(arguments.size()), obj::ErrorType::TypeError);
            }
            else
            {
                std::vector<std::string> nrExpected;
                for (const auto &element : nrExpectedArguments)
                    nrExpected.push_back(std::to_string(element));
                return std::make_shared<obj::Error>(errorPrefix + ": expected " + util::join(nrExpected, ",") + " arguments, got " + std::to_string(arguments.size()), obj::ErrorType::TypeError);
            }
        }
        return nullptr;
    }

    std::shared_ptr<obj::Object>
    validateArguments(
        const std::string &errorPrefix,
        const std::shared_ptr<obj::Object> &self,
        const std::vector<std::shared_ptr<obj::Object>> &arguments,
        const obj::ObjectType expectedType,
        size_t nrExpectedArguments)
    {
        if (self.get()->type != expectedType)
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + toString(expectedType) + ", got " + toString(self.get()->type), obj::ErrorType::TypeError);
        if (nrExpectedArguments != arguments.size())
        {
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + std::to_string(nrExpectedArguments) + " arguments, got " + std::to_string(arguments.size()), obj::ErrorType::TypeError);
        }
        return nullptr;
    }

}

namespace builtin
{
    std::shared_ptr<obj::Object> string_size(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("size", self, arguments, obj::ObjectType::String, 0);
        if (errorObj)
            return errorObj;
        return std::make_shared<obj::Integer>(static_cast<obj::String *>(self.get())->value.size());
    }

    std::shared_ptr<obj::Object> string_clear(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        if (self->frozen > 0)
            return std::make_shared<obj::Error>("string clear expects a non-frozen object", obj::ErrorType::TypeError);

        auto errorObj = validateArguments("clear", self, arguments, obj::ObjectType::String, 0);
        if (errorObj)
            return errorObj;

        static_cast<obj::String *>(self.get())->value.clear();
        return self;
    }

    std::shared_ptr<obj::Object> string_empty(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("is_empty", self, arguments, obj::ObjectType::String, 0);
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Boolean>(static_cast<obj::String *>(self.get())->value.empty());
    }

    std::shared_ptr<obj::Object> string_starts_with(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("starts_with", self, arguments, obj::ObjectType::String, 1);
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("starts_with: expected argument 1 of type str", obj::ErrorType::TypeError);

        const std::string &selfStr = static_cast<obj::String *>(self.get())->value;
        const std::string &whatStr = static_cast<obj::String *>(arguments[0].get())->value;

        bool startsWith = selfStr.compare(0, whatStr.length(), whatStr) == 0;
        return std::make_shared<obj::Boolean>(startsWith);
    }

    std::shared_ptr<obj::Object> string_ends_with(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("ends_with", self, arguments, obj::ObjectType::String, 1);
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("ends_with: expected argument 1 of type str", obj::ErrorType::TypeError);

        const std::string &selfStr = static_cast<obj::String *>(self.get())->value;
        const std::string &whatStr = static_cast<obj::String *>(arguments[0].get())->value;
        if (whatStr.length() > selfStr.length())
            return std::make_shared<obj::Boolean>(false);

        bool endsWith = selfStr.compare(selfStr.length() - whatStr.length(), whatStr.length(), whatStr) == 0;
        return std::make_shared<obj::Boolean>(endsWith);
    }

    std::shared_ptr<obj::Object> string_find(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("find", self, arguments, obj::ObjectType::String, 1);
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("find: expected argument 1 of type str", obj::ErrorType::TypeError);

        const std::string &selfStr = static_cast<obj::String *>(self.get())->value;
        const std::string &whatStr = static_cast<obj::String *>(arguments[0].get())->value;
        int index = selfStr.find_first_of(whatStr);
        return std::make_shared<obj::Integer>(index);
    }

    std::shared_ptr<obj::Object> string_replace(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("replace", self, arguments, obj::ObjectType::String, 2);
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("replace: expected argument 1 of type str", obj::ErrorType::TypeError);
        if (arguments[1]->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("replace: expected argument 2 of type str", obj::ErrorType::TypeError);

        const std::string &selfStr = static_cast<obj::String *>(self.get())->value;
        const std::string &whatStr = static_cast<obj::String *>(arguments[0].get())->value;
        const std::string &replacementStr = static_cast<obj::String *>(arguments[1].get())->value;

        std::string replaced = selfStr;
        size_t pos = 0;
        while ((pos = replaced.find(whatStr, pos)) != std::string::npos)
        {
            replaced.replace(pos, whatStr.length(), replacementStr);
            pos += replacementStr.length();
        }
        return std::make_shared<obj::String>(replaced);
    }

    std::shared_ptr<obj::Object> string_split(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("split", self, arguments, obj::ObjectType::String, {0, 1});
        if (errorObj)
            return errorObj;

        const std::string &selfStr = static_cast<obj::String *>(self.get())->value;
        std::string splitDelimiters;
        if (arguments.size() == 1)
        {
            if (arguments[0]->type != obj::ObjectType::String)
                return std::make_shared<obj::Error>("split: expected argument 1 of type str", obj::ErrorType::TypeError);

            splitDelimiters = static_cast<obj::String *>(arguments[0].get())->value;
        }

        auto splitFunction = [splitDelimiters](int character) -> bool
        {
            if (!splitDelimiters.empty())
                return (splitDelimiters.find(character) != std::string::npos);
            return isspace(character);
        };

        std::vector<std::string> splitted;

        auto itStr = selfStr.begin();
        auto endOfStr = selfStr.end();

        while (itStr != endOfStr)
        {
            itStr = std::find_if_not(itStr, endOfStr, splitFunction);
            if (itStr == endOfStr)
                break;

            auto itEndDelim = std::find_if(itStr, endOfStr, splitFunction);
            splitted.push_back(std::string(itStr, itEndDelim));
            itStr = itEndDelim;
        }

        std::vector<std::shared_ptr<obj::Object>> splittedObj;
        splittedObj.reserve(splitted.size() + 1);
        for (const auto &element : splitted)
        {
            splittedObj.push_back(std::make_shared<obj::String>(element));
        }
        return std::make_shared<obj::Array>(splittedObj);
    }

    std::shared_ptr<obj::Object> string_join(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("join", self, arguments, obj::ObjectType::String, 1);
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::Array)
            return std::make_shared<obj::Error>("join: expected argument 1 of type array", obj::ErrorType::TypeError);

        std::vector<std::string> whatStrs;
        obj::Array *arr = static_cast<obj::Array *>(arguments[0].get());
        for (const auto &element : arr->value)
        {
            if (element->type != obj::ObjectType::String)
                return std::make_shared<obj::Error>("join: expected argument array of str", obj::ErrorType::TypeError);
            whatStrs.push_back(static_cast<obj::String *>(element.get())->value);
        }

        const std::string &selfStr = static_cast<obj::String *>(self.get())->value;
        std::string joined = util::join(whatStrs, selfStr);
        return std::make_shared<obj::String>(joined);
    }

    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeString()
    {
        typedef obj::TBuiltinTypeFunctionDefinition TBuiltInFD;

        auto stringBuiltinType = std::make_shared<obj::BuiltinType>();
        stringBuiltinType->builtinObjectType = obj::ObjectType::String;

        stringBuiltinType->functions = {
            {"clear", TBuiltInFD({&builtin::string_clear, typing::makeFunctionType("", "str")})},     // "str.fn() -> str"
            {"is_empty", TBuiltInFD({&builtin::string_empty, typing::makeFunctionType("", "bool")})}, // "str.fn() -> bool"
            {"size", TBuiltInFD({&builtin::string_size, typing::makeFunctionType("", "int")})},       // "str.fn() -> int"

            {"starts_with", TBuiltInFD({&builtin::string_starts_with, typing::makeFunctionType("str", "bool")})}, // "str.fn() -> bool"
            {"ends_with", TBuiltInFD({&builtin::string_ends_with, typing::makeFunctionType("str", "bool")})},     // "str.fn() -> bool"

            {"find", TBuiltInFD({&builtin::string_find, typing::makeFunctionType("str", "int")})},            // "str.fn(str) -> int"
            {"replace", TBuiltInFD({&builtin::string_replace, typing::makeFunctionType("str, str", "str")})}, // "str.fn(str,str) -> str"

            {"split", TBuiltInFD({&builtin::string_split, typing::makeFunctionType("str", "[str]")})}, // "str.fn() -> [str]"
            {"join", TBuiltInFD({&builtin::string_join, typing::makeFunctionType("[str]", "str")})},   // "str.fn([str]) -> str"
        };
        return stringBuiltinType;
    }
}