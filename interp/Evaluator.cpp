/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Evaluator.h"
#include "Version.h"
#include "Typing.h"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <string>
#include <unordered_set>
#include <filesystem>

#include "Lexer.h"
#include "Parser.h"

#include "Util.h"

#include "builtin/Array.h"
#include "builtin/Dictionary.h"
#include "builtin/Error.h"
#include "builtin/ErrorType.h"
#include "builtin/IO.h"
#include "builtin/Json.h"
#include "builtin/Math.h"
#include "builtin/OS.h"
#include "builtin/Regex.h"
#include "builtin/Set.h"
#include "builtin/String.h"
#include "builtin/Freeze.h"
#include "builtin/Time.h"
#include "builtin/Thread.h"
#include "builtin/Threading.h"
#include "builtin/Typing.h"
#include "format/Format.h"

std::shared_ptr<obj::Object> evalStatement(ast::Statement *statement, const std::shared_ptr<obj::Environment> &environment);
std::shared_ptr<obj::Object> evalFunctionWithArguments(obj::Function *functionObj, const std::vector<std::shared_ptr<obj::Object>> &evaluatedArgs, const std::shared_ptr<obj::Environment> &environment);

obj::Object *evalInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right);

std::shared_ptr<obj::Object> NullObject(new obj::Null());

namespace
{
    std::shared_ptr<obj::Environment> makeNewEnvironment(const std::shared_ptr<obj::Environment> &parentEnvironment)
    {
        auto newEnvironment = std::make_shared<obj::Environment>();
        newEnvironment->outer = parentEnvironment;
        return newEnvironment;
    }

    std::vector<std::string> argsFromEnvironment;

    /* return a normalized index so it can be used in a non-zero length array/deque */
    size_t normalizedArrayIndex(int64_t userProvidedIndex, size_t arrayLength)
    {
        int64_t finalIndex = userProvidedIndex;
        if (userProvidedIndex < 0)
        {
            if (arrayLength > 0)
                return static_cast<size_t>((arrayLength - (abs(userProvidedIndex) % arrayLength)) % arrayLength);
            else
                return static_cast<size_t>(userProvidedIndex);
        }
        return static_cast<size_t>(userProvidedIndex);
    }

    /* using squared powering to reduce linear complexity to O(log^2(p)) */
    int64_t pow_int(int64_t x, int64_t p)
    {
        if (p < 0)
            return 0;
        if (p == 0)
            return 1;
        if (p == 1)
            return x;

        int64_t tmp = pow_int(x, p / 2);
        if (p % 2 == 0)
            return tmp * tmp;
        else
            return x * tmp * tmp;
    }

    bool isValueAssigned(const std::shared_ptr<obj::Object> &rhs)
    {
        switch (rhs->type)
        {
        case obj::ObjectType::Boolean:
        case obj::ObjectType::Integer:
        case obj::ObjectType::String:
        case obj::ObjectType::Char:
        case obj::ObjectType::Double:
        case obj::ObjectType::Complex:
            return true;
        default:
            return false;
        }
        return false;
    }

    size_t arrayLikeLength(const obj::Object *obj)
    {
        switch (obj->type)
        {
        case obj::ObjectType::Array:
            return static_cast<const obj::Array *>(obj)->value.size();
        case obj::ObjectType::ArrayDouble:
            return static_cast<const obj::ArrayDouble *>(obj)->value.size();
        case obj::ObjectType::ArrayComplex:
            return static_cast<const obj::ArrayComplex *>(obj)->value.size();
        }
        throw std::runtime_error("Trying to get length of non-array like type " + toString(obj->type));
    }

    std::shared_ptr<obj::Object> arrayLikeItem(const obj::Object *obj, size_t index)
    {
        switch (obj->type)
        {
        case obj::ObjectType::Array:
            return static_cast<const obj::Array *>(obj)->value[index];
        case obj::ObjectType::ArrayDouble:
            return std::make_shared<obj::Double>(static_cast<const obj::ArrayDouble *>(obj)->value[index]);
        case obj::ObjectType::ArrayComplex:
            return std::make_shared<obj::Complex>(static_cast<const obj::ArrayComplex *>(obj)->value[index]);
        }
        throw std::runtime_error("Trying to get element of non-array like type");
    }

}

void initializeArg(int offset, int argc, char **argv)
{
    for (int i = offset; i < argc; ++i)
        argsFromEnvironment.push_back(std::string(argv[i]));
}

namespace builtin
{
    std::shared_ptr<obj::Object> exit(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (arguments->size() > 1)
            return std::make_shared<obj::Error>("exit: expected zero or 1 arguments", obj::ErrorType::TypeError);

        int retValue = 0;
        if (!arguments->empty())
        {
            auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
            auto intObj = dynamic_cast<obj::Integer *>(evaluatedExpr.get());
            if (!intObj)
                return std::make_shared<obj::Error>("exit: argument needs to be of type int", obj::ErrorType::TypeError);
            retValue = static_cast<int>(intObj->value);
        }
        return std::make_shared<obj::Exit>(retValue);
    }

    std::shared_ptr<obj::Object> version(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 0)
            return std::make_shared<obj::Error>("version: expected no arguments", obj::ErrorType::TypeError);

        std::vector<std::shared_ptr<obj::Object>> values;
        values.push_back(std::make_shared<obj::Integer>(majorVersion));
        values.push_back(std::make_shared<obj::Integer>(minorVersion));
        values.push_back(std::make_shared<obj::Integer>(patchVersion));
        return std::make_shared<obj::Array>(obj::Array(values));
    }

    std::shared_ptr<obj::Object> arg(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 0)
            return std::make_shared<obj::Error>("arg: expected no arguments", obj::ErrorType::TypeError);

        std::vector<std::shared_ptr<obj::Object>> values;
        for (const auto &argument : argsFromEnvironment)
            values.push_back(std::make_shared<obj::String>(argument));
        return std::make_shared<obj::Array>(obj::Array(values));
    }

    std::shared_ptr<obj::Object> address(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("address: expected 1 argument");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        auto addr = reinterpret_cast<uint64_t>(evaluatedExpr.get());
        return std::make_shared<obj::Integer>(obj::Integer(addr));
    }

    std::shared_ptr<obj::Object> lookup_hash(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("lookup_hash: expected 1 argument");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        auto hash = obj::Hash().operator()(evaluatedExpr);
        return std::make_shared<obj::Integer>(obj::Integer(hash));
    }

    std::shared_ptr<obj::Object> lookup_hashable(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("lookup_hashable: expected 1 argument");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        return std::make_shared<obj::Boolean>(obj::Boolean(evaluatedExpr->hashAble()));
    }

    std::shared_ptr<obj::Object> lookup_equal(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return obj::makeTypeError("lookup_equal: expected 2 arguments");

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        auto evaluatedExpr2 = evalExpression(arguments->front().get(), environment);
        bool eq = obj::Equal().operator()(evaluatedExpr1, evaluatedExpr2);
        return std::make_shared<obj::Boolean>(eq);
    }

    std::shared_ptr<obj::Object> type_str(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("type_str: expected 1 argument");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        auto typeExpr = typing::computeType(evaluatedExpr.get());
        if (typeExpr == nullptr)
            return obj::makeTypeError("type_str: cannot compute type");

        return std::make_shared<obj::String>(typeExpr->text());
    }

    std::shared_ptr<obj::Object> internal_type_str(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("type_str: expected 1 argument");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        return std::make_shared<obj::String>(obj::toString(evaluatedExpr->type));
    }

    std::shared_ptr<obj::Object> print_impl(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment, std::ostream &outStream)
    {
        if (!arguments)
            return NullObject;

        size_t argIndex = 0;
        for (const auto &arg : *arguments)
        {
            auto evaluatedExpr = evalExpression(arg.get(), environment);
            if (evaluatedExpr->type == obj::ObjectType::Error)
                return evaluatedExpr;
            // for most expression their inspected value is fine, except for strings where the extra quotes make it
            // not useful, so for that case we trim the inspected value;
            std::string inspected = evaluatedExpr->inspect();
            auto stringExpr = dynamic_cast<obj::String *>(evaluatedExpr.get());
            if (stringExpr)
                inspected = inspected.substr(1, inspected.size() - 2);

            outStream << inspected;
            ++argIndex;
        }
        outStream << std::endl;
        return NullObject;
    }

    std::shared_ptr<obj::Object> print(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return print_impl(arguments, environment, std::cout);
    }

    std::shared_ptr<obj::Object> eprint(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        return print_impl(arguments, environment, std::cerr);
    }

    std::shared_ptr<obj::Object>
    format(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        /*
         * format( " {0:5.2f} {1:.1f} ", a, b);
         * future (but more difficult in a compilation scenario, due to variable scoping rules): format( " {a:05.2f} ", a);
         * format type:
         *      fill-and-align (optional) sign (optional) #(optional) 0(optional) width (optional) precision (optional) L(optional) type (optional)
         */

        if (!arguments)
            return NullObject;

        if (arguments->size() < 1)
            return obj::makeTypeError("format: expected at least 1 of type str");

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, String, "format: expected argument 1 to be a string");

        auto format = static_cast<obj::String *>(evaluatedExpr1.get())->value;
        std::vector<std::shared_ptr<obj::Object>> values;

        for (size_t i = 1; i < arguments->size(); ++i)
        {
            auto value = evalExpression((*arguments)[i].get(), environment);
            if (value->type == obj::ObjectType::Error)
                return value;

            values.push_back(value);
        }

        size_t formatIndex = 0;
        size_t valueIndex = 0;
        std::stringstream result;

        while (formatIndex < format.size())
        {
            if (format[formatIndex] == '{')
            {
                // Find the end of the placeholder
                size_t endBraceIndex = format.find('}', formatIndex);
                if (endBraceIndex == std::string::npos)
                {
                    return std::make_shared<obj::Error>("Missing closing brace", obj::ErrorType::ValueError);
                }

                auto placeHolderFormat = format.substr(formatIndex + 1, endBraceIndex - formatIndex - 1);
                std::string formatStr = "";

                // check if the placeholder format refers to an index, a variable or is left default and empty
                // a special case is also the empty placeholder format

                auto referencedValue = valueIndex;
                if (!placeHolderFormat.empty())
                {
                    auto isInteger = [](const std::string &s) -> bool
                    {
                        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
                    };

                    // the start until the ':' is either the index or a variable name
                    size_t doubleColonIndex = placeHolderFormat.find(':');
                    if (doubleColonIndex != std::string::npos)
                    {
                        auto referenceStr = placeHolderFormat.substr(0, doubleColonIndex);

                        if (!referenceStr.empty())
                        {
                            if (isInteger(referenceStr))
                                referencedValue = std::atoi(referenceStr.c_str());
                            else
                                return std::make_shared<obj::Error>("Referenced value is not an integer", obj::ErrorType::ValueError);
                        }
                        // what comes after the ':' is the format string
                        formatStr = placeHolderFormat.substr(doubleColonIndex + 1);
                    }
                    else
                    {
                        if (isInteger(placeHolderFormat))
                            referencedValue = std::atoi(placeHolderFormat.c_str());
                        else
                            return std::make_shared<obj::Error>("Referenced value is not an integer", obj::ErrorType::ValueError);
                    }
                }

                if (referencedValue >= values.size())
                    return std::make_shared<obj::Error>("Referenced value out of range", obj::ErrorType::IndexError);

                // parse the formatStr into the Formatting structure to pass later into a proper formatting function

                auto formatting = builtin::parseFormatting(formatStr);
                if (!formatting.error.empty())
                    return std::make_shared<obj::Error>("Format string malformed: " + formatting.error, obj::ErrorType::ValueError);
                result << builtin::format_impl(values[referencedValue].get(), formatting);

                // move to the character after the closing brace
                formatIndex = endBraceIndex + 1;
                ++valueIndex;
            }
            else
            {
                // Copy non-placeholder characters to the result
                result << format[formatIndex++];
            }
        }
        return std::make_shared<obj::String>(result.str());
    }

    std::shared_ptr<obj::Object> input_line(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 0)
            return obj::makeTypeError("input_line: expected 1 argument");

        std::string input;
        std::cin >> input;
        return std::make_shared<obj::String>(input);
    }

    std::shared_ptr<obj::Object> doc(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("doc: expected 1 argument");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Function)
        {
            return std::make_shared<obj::String>(static_cast<obj::Function *>(evaluatedExpr.get())->doc);
        }
        else if (evaluatedExpr->type == obj::ObjectType::UserType)
        {
            return std::make_shared<obj::String>(static_cast<obj::UserType *>(evaluatedExpr.get())->doc);
        }
        else if (evaluatedExpr->type == obj::ObjectType::BoundUserTypeFunction)
        {
            return std::make_shared<obj::String>(static_cast<obj::BoundUserTypeFunction *>(evaluatedExpr.get())->function->doc);
        }

        return NullObject;
    }

    std::shared_ptr<obj::Object> open(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() < 1 || arguments->size() > 2)
            return obj::makeTypeError("open: expected 1 or 2 argument of type (str,str)");

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, String, "open: expected argument 1 to be a string");

        std::string mode = "r";
        if (arguments->size() == 2)
        {
            auto evaluatedExpr2 = evalExpression((*arguments)[1].get(), environment);
            RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr2, String, "open: expected argument 2 to be a string");
            mode = static_cast<obj::String *>(evaluatedExpr2.get())->value;
        }

        auto path = static_cast<obj::String *>(evaluatedExpr1.get())->value;
        std::map<std::string, std::ios_base::openmode> openModeMapping = {
            {"r", std::ios_base::in},
            {"rb", std::ios_base::in | std::ios_base::binary},
            {"w", std::ios_base::out},
            {"wb", std::ios_base::out | std::ios_base::binary},
            {"a", std::ios_base::app},
        };

        if (openModeMapping.find(mode) == openModeMapping.end())
        {
            std::vector<std::string> openModeChoices;
            for (const auto &[k, v] : openModeMapping)
                openModeChoices.push_back(k);
            return obj::makeTypeError("open: openmode has to be one of " + util::join(openModeChoices, ",") + ", got " + mode);
        }

        std::ios_base::openmode openMode = openModeMapping.at(mode);
        auto ioObject = std::make_shared<obj::IOObject>();
        ioObject->open(path, openMode);

        return ioObject;
    }

    std::shared_ptr<obj::Object> run_impl(const std::string &text, const std::string &fileName, const std::shared_ptr<obj::Environment> &environment)
    {
        auto lexer = createLexer(text, fileName);
        auto parser = createParser(std::move(lexer));
        auto program = parser->parseProgram();

        if (!parser->errorMsgs.empty())
        {
            std::stringstream ss;
            for (const auto &msg : parser->errorMsgs)
                ss << msg << std::endl;
            return std::make_shared<obj::Error>("run: parsing errors encountered: " + ss.str(), obj::ErrorType::SyntaxError);
        }

        return evalProgram(program.get(), environment);
    }

    std::shared_ptr<obj::Object> run(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("run: expected 1 argument of type str", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, String, "run: expected argument 1 to be a string");

        std::string fileToRun = static_cast<obj::String *>(evaluatedExpr1.get())->value;
        std::string text;
        std::string line;
        std::ifstream inputf;
        inputf.open(fileToRun);
        if (inputf.is_open())
        {
            while (std::getline(inputf, line))
                text += line + "\n";
        }
        else
        {
            return std::make_shared<obj::Error>("run: " + fileToRun + " cannot be read", obj::ErrorType::OSError);
        }

        return run_impl(text, fileToRun, environment);
    }

    std::shared_ptr<obj::Object> import(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("import: expected 1", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr1->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("run: expected argument 1 to be a string", obj::ErrorType::TypeError);

        std::string fileToRun = static_cast<obj::String *>(evaluatedExpr1.get())->value;
        std::string text;
        std::string line;
        std::ifstream inputf;
        inputf.open(fileToRun);
        if (inputf.is_open())
        {
            while (std::getline(inputf, line))
                text += line + "\n";
        }
        else
        {
            return std::make_shared<obj::Error>("import: " + fileToRun + " cannot be read", obj::ErrorType::OSError);
        }

        auto newEnvironment = makeNewEnvironment(nullptr);
        auto moduleObj = std::make_shared<obj::Module>();
        moduleObj->environment = newEnvironment;
        auto runResult = run_impl(text, fileToRun, newEnvironment);
        if (runResult->type == obj::ObjectType::Error)
            return runResult;
        return moduleObj;
    }

    std::unordered_set<std::string> run_onceRegistry;
    std::shared_ptr<obj::Object> run_once(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("run: expected 1 or 2 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr1->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("run: expected argument 1 to be a string", obj::ErrorType::TypeError);

        std::string fileToRun = static_cast<obj::String *>(evaluatedExpr1.get())->value;
        std::filesystem::path fileToRunPath = std::filesystem::canonical(std::filesystem::path(fileToRun));

        if (run_onceRegistry.find(fileToRunPath.string()) != run_onceRegistry.end())
            return NullObject;
        run_onceRegistry.insert(fileToRunPath.string());

        std::string text;
        std::string line;
        std::ifstream inputf;
        inputf.open(fileToRun);
        if (inputf.is_open())
        {
            while (std::getline(inputf, line))
                text += line + "\n";
        }
        else
        {
            return std::make_shared<obj::Error>("run: " + fileToRun + " cannot be read", obj::ErrorType::OSError);
        }

        return run_impl(text, fileToRun, environment);
    }

    namespace
    {
        void collectContextNames(const obj::Environment &environment, std::vector<std::string> &names)
        {
            if (environment.outer)
                collectContextNames(*environment.outer, names);

            for (const auto &[key, value] : environment.store)
                names.push_back(key);
        }
    }

    std::shared_ptr<obj::Object> scope_names(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (arguments->size() != 0)
            return std::make_shared<obj::Error>("scope_names: expected no arguments", obj::ErrorType::TypeError);

        std::vector<std::string> names;
        if (environment)
            collectContextNames(*environment, names);

        std::vector<std::shared_ptr<obj::Object>> values;
        for (const auto &name : names)
            values.push_back(std::make_shared<obj::String>(name));

        return std::make_shared<obj::Array>(values);
    }

    std::shared_ptr<obj::Object> clone(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("clone: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        return evaluatedExpr->clone();
    }

    std::shared_ptr<obj::Object> error(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("error: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("error: expected 1 argument to be a string", obj::ErrorType::TypeError);

        auto stringValue = static_cast<obj::String *>(evaluatedExpr.get());
        return std::make_shared<obj::Error>(stringValue->value, obj::ErrorType::UndefinedError);
    }

    std::shared_ptr<obj::Object> array(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 1)
            return std::make_shared<obj::Error>("array: expected no or one argument", obj::ErrorType::TypeError);

        std::vector<std::shared_ptr<obj::Object>> values;

        if (arguments->size() == 1)
        {
            auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
            switch (evaluatedExpr->type)
            {
            case obj::ObjectType::Range:
            {
                auto rangeVal = static_cast<obj::Range *>(evaluatedExpr.get());
                for (const auto &val : rangeVal->values())
                {
                    values.push_back(std::make_shared<obj::Integer>(val));
                }
            }
            break;
            default:
                return std::make_shared<obj::Error>("array: cannot convert first argument", obj::ErrorType::TypeError);
            };
        }

        return std::make_shared<obj::Array>(obj::Array(values));
    }

    std::shared_ptr<obj::Object> array_double(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 1)
            return std::make_shared<obj::Error>("array_double: expected at most 1 argument", obj::ErrorType::TypeError);

        std::vector<double> values;
        if (arguments->size() == 1)
        {
            auto typeHintArray = std::make_unique<ast::TypeArray>();
            typeHintArray->elementType = std::make_unique<ast::TypeIdentifier>("double");
            auto evalExpr = evalExpression(arguments->front().get(), environment, typeHintArray.get());
            if (evalExpr->type == obj::ObjectType::ArrayDouble)
                values = static_cast<obj::ArrayDouble *>(evalExpr.get())->value;
            else
                return std::make_shared<obj::Error>("array_double: cannot convert argument", obj::ErrorType::TypeError);
        }
        return std::make_shared<obj::ArrayDouble>(values);
    }

    std::shared_ptr<obj::Object> array_complex(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 1)
            return std::make_shared<obj::Error>("array_complex: expected at most 1 argument", obj::ErrorType::TypeError);

        std::vector<std::complex<double>> values;
        if (arguments->size() == 1)
        {
            auto typeHintArray = std::make_unique<ast::TypeArray>();
            typeHintArray->elementType = std::make_unique<ast::TypeIdentifier>("complex");
            auto evalExpr = evalExpression(arguments->front().get(), environment, typeHintArray.get());
            if (evalExpr->type == obj::ObjectType::ArrayComplex)
                values = static_cast<obj::ArrayComplex *>(evalExpr.get())->value;
            else
                return std::make_shared<obj::Error>("array_complex: cannot convert argument", obj::ErrorType::TypeError);
        }
        return std::make_shared<obj::ArrayComplex>(values);
    }

    std::shared_ptr<obj::Object> complex(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 2)
            return std::make_shared<obj::Error>("complex: expected less than 3 arguments", obj::ErrorType::TypeError);

        if (arguments->size() == 0)
        {
            std::complex<double> value;
            return std::make_shared<obj::Complex>(obj::Complex(value));
        }
        else if (arguments->size() == 1)
        {
            auto evalExpr = evalExpression(arguments->front().get(), environment);
            if (evalExpr->type == obj::ObjectType::Double)
                return std::make_shared<obj::Complex>(std::complex<double>({static_cast<obj::Double *>(evalExpr.get())->value}));
            return std::make_shared<obj::Error>("complex: first argument needs to be a double", obj::ErrorType::TypeError);
        }
        else if (arguments->size() == 2)
        {
            auto evalExpr1 = evalExpression(arguments->front().get(), environment);
            if (evalExpr1->type != obj::ObjectType::Double)
                return std::make_shared<obj::Error>("complex: first argument needs to be a double", obj::ErrorType::TypeError);
            auto evalExpr2 = evalExpression(arguments->back().get(), environment);
            if (evalExpr2->type != obj::ObjectType::Double)
                return std::make_shared<obj::Error>("complex: second argument needs to be a double", obj::ErrorType::TypeError);

            return std::make_shared<obj::Complex>(obj::Complex({static_cast<obj::Double *>(evalExpr1.get())->value, static_cast<obj::Double *>(evalExpr2.get())->value}));
        }
        return std::make_shared<obj::Error>("complex: unexpected", obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> dict(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 0)
            return std::make_shared<obj::Error>("dict: expected no arguments", obj::ErrorType::TypeError);

        std::unordered_map<std::shared_ptr<obj::Object>, std::shared_ptr<obj::Object>, obj::Hash, obj::Equal> value;
        return std::make_shared<obj::Dictionary>(obj::Dictionary(value));
    }

    std::shared_ptr<obj::Object> set(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 0)
            return std::make_shared<obj::Error>("set: expected no arguments", obj::ErrorType::TypeError);

        std::unordered_set<std::shared_ptr<obj::Object>, obj::Hash, obj::Equal> value;
        return std::make_shared<obj::Set>(obj::Set(value));
    }

    std::shared_ptr<obj::Object> range(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1 && arguments->size() != 2 && arguments->size() != 3)
            return std::make_shared<obj::Error>("range: expected two or three arguments", obj::ErrorType::TypeError);

        int64_t arg1Value = 0;
        int64_t arg2Value = 0;
        int64_t arg3Value = 1;

        auto arg1 = evalExpression((*arguments)[0].get(), environment);
        if (arg1->type != obj::ObjectType::Integer)
            return std::make_shared<obj::Error>("range: first argument needs to be Integer, got " + toString(arg1->type), obj::ErrorType::TypeError);
        arg2Value = static_cast<obj::Integer *>(arg1.get())->value;

        if (arguments->size() > 1)
        {
            auto arg2 = evalExpression((*arguments)[1].get(), environment);
            if (arg2->type != obj::ObjectType::Integer)
                return std::make_shared<obj::Error>("range: second argument needs to be Integer, got " + toString(arg2->type), obj::ErrorType::TypeError);
            arg1Value = arg2Value;
            arg2Value = static_cast<obj::Integer *>(arg2.get())->value;
        }

        std::shared_ptr<obj::Object> arg3;
        if (arguments->size() == 3)
        {
            arg3 = evalExpression((*arguments)[2].get(), environment);
            if (arg3->type != obj::ObjectType::Integer)
                return std::make_shared<obj::Error>("range: third argument needs to be Integer, got " + toString(arg3->type), obj::ErrorType::TypeError);
            arg3Value = static_cast<obj::Integer *>(arg3.get())->value;
        }

        return std::make_shared<obj::Range>(arg1Value, arg2Value, arg3Value);
    }

    std::shared_ptr<obj::Object> len(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("len: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Error:
            return evaluatedExpr;
        case obj::ObjectType::String:
            return std::make_shared<obj::Integer>(static_cast<obj::String *>(evaluatedExpr.get())->value.size());
        case obj::ObjectType::Array:
            return std::make_shared<obj::Integer>(static_cast<obj::Array *>(evaluatedExpr.get())->value.size());
        case obj::ObjectType::ArrayDouble:
            return std::make_shared<obj::Integer>(static_cast<obj::ArrayDouble *>(evaluatedExpr.get())->value.size());
        case obj::ObjectType::ArrayComplex:
            return std::make_shared<obj::Integer>(static_cast<obj::ArrayComplex *>(evaluatedExpr.get())->value.size());
        case obj::ObjectType::Dictionary:
            return std::make_shared<obj::Integer>(static_cast<obj::Dictionary *>(evaluatedExpr.get())->value.size());
        case obj::ObjectType::Set:
            return std::make_shared<obj::Integer>(static_cast<obj::Set *>(evaluatedExpr.get())->value.size());
        case obj::ObjectType::Range:
            return std::make_shared<obj::Integer>(static_cast<obj::Range *>(evaluatedExpr.get())->length());
        };
        return std::make_shared<obj::Error>("Invalid type for len: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> to_bool(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("to_bool: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;

        auto stringObj = dynamic_cast<obj::String *>(evaluatedExpr.get());
        if (stringObj)
        {
            if (stringObj->value == "false")
                return std::make_shared<obj::Boolean>(false);
            if (stringObj->value == "true")
                return std::make_shared<obj::Boolean>(true);

            try
            {
                return std::make_shared<obj::Boolean>(std::stoll(stringObj->value) != 0);
            }
            catch (std::invalid_argument &e)
            {
                return std::make_shared<obj::Error>(std::string("Invalid cast to_bool, invalid argument: ") + e.what(), obj::ErrorType::TypeError);
            }
            catch (std::out_of_range &e)
            {
                return std::make_shared<obj::Error>(std::string("Invalid cast to_bool, out of range: ") + e.what(), obj::ErrorType::ValueError);
            }
        }
        return std::make_shared<obj::Error>("Invalid type for to_bool: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> to_int(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("to_int: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;

        auto stringObj = dynamic_cast<obj::String *>(evaluatedExpr.get());
        if (stringObj)
        {
            try
            {
                return std::make_shared<obj::Integer>(std::stoll(stringObj->value));
            }
            catch (std::invalid_argument &e)
            {
                return std::make_shared<obj::Error>(std::string("Invalid cast to_int, invalid argument: ") + e.what(), obj::ErrorType::TypeError);
            }
            catch (std::out_of_range &e)
            {
                return std::make_shared<obj::Error>(std::string("Invalid cast to_int, out of range: ") + e.what(), obj::ErrorType::ValueError);
            }
        }
        return std::make_shared<obj::Error>("Invalid type for to_int: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> to_double(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("to_double: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;

        switch (evaluatedExpr.get()->type)
        {
        case obj::ObjectType::String:
        {
            auto stringObj = static_cast<obj::String *>(evaluatedExpr.get());
            if (stringObj)
            {
                try
                {
                    return std::make_shared<obj::Double>(std::stold(stringObj->value));
                }
                catch (std::invalid_argument &e)
                {
                    return std::make_shared<obj::Error>(std::string("Invalid cast to_double, invalid argument: ") + e.what(), obj::ErrorType::TypeError);
                }
                catch (std::out_of_range &e)
                {
                    return std::make_shared<obj::Error>(std::string("Invalid cast to_double, out of range: ") + e.what(), obj::ErrorType::ValueError);
                }
            }
        }
        case obj::ObjectType::Integer:
        {
            return std::make_shared<obj::Double>(static_cast<double>(static_cast<obj::Integer *>(evaluatedExpr.get())->value));
        }
        };
        return std::make_shared<obj::Error>("Invalid type for to_double: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> updateArray(std::shared_ptr<obj::Object> obj, const std::vector<ast::Expression *> &arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        auto arrayObj = dynamic_cast<obj::Array *>(obj.get());
        if (!arrayObj)
            return std::make_shared<obj::Error>("Invalid argument 1 for array update: " + obj::toString(obj->type), obj::ErrorType::TypeError);

        auto indexExpr = std::move(evalExpression(arguments.at(1), environment));
        if (indexExpr->type == obj::ObjectType::Error)
            return indexExpr;

        if (indexExpr->type != obj::ObjectType::Integer)
            return std::make_shared<obj::Error>("Invalid argument 1 for update: " + obj::toString(indexExpr->type), obj::ErrorType::TypeError);

        auto intObj = static_cast<obj::Integer *>(indexExpr.get());
        size_t arraySize = static_cast<int>(arrayObj->value.size());
        size_t finalIndex = normalizedArrayIndex(intObj->value, arraySize);
        if (finalIndex >= arraySize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intObj->value) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError);

        auto validObj = std::move(evalExpression(arguments.at(2), environment));
        if (validObj->type == obj::ObjectType::Error)
            return validObj;

        if (isValueAssigned(validObj))
            arrayObj->value[finalIndex] = validObj->clone();
        else
            arrayObj->value[finalIndex] = validObj;
        return obj;
    }

    std::shared_ptr<obj::Object> updateArrayDouble(std::shared_ptr<obj::Object> obj, const std::vector<ast::Expression *> &arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        auto arrayObj = dynamic_cast<obj::ArrayDouble *>(obj.get());
        if (!arrayObj)
            return std::make_shared<obj::Error>("Invalid argument 1 for array update: " + obj::toString(obj->type), obj::ErrorType::TypeError);

        auto indexExpr = std::move(evalExpression(arguments.at(1), environment));
        if (indexExpr->type == obj::ObjectType::Error)
            return indexExpr;

        if (indexExpr->type != obj::ObjectType::Integer)
            return std::make_shared<obj::Error>("Invalid argument 1 for update: " + obj::toString(indexExpr->type), obj::ErrorType::TypeError);

        auto intObj = static_cast<obj::Integer *>(indexExpr.get());
        size_t arraySize = static_cast<int>(arrayObj->value.size());
        size_t finalIndex = normalizedArrayIndex(intObj->value, arraySize);
        if (finalIndex >= arraySize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intObj->value) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError);

        auto validObj = std::move(evalExpression(arguments.at(2), environment));
        if (validObj->type == obj::ObjectType::Error)
            return validObj;

        if (validObj->type != obj::ObjectType::Double)
        {
            return std::make_shared<obj::Error>("Invalid argument 1 for update [double]: " + obj::toString(validObj->type), obj::ErrorType::ValueError);
        }

        arrayObj->value[finalIndex] = static_cast<obj::Double *>(validObj.get())->value;
        return obj;
    }

    std::shared_ptr<obj::Object> updateArrayComplex(std::shared_ptr<obj::Object> obj, const std::vector<ast::Expression *> &arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        auto arrayObj = dynamic_cast<obj::ArrayComplex *>(obj.get());
        if (!arrayObj)
            return std::make_shared<obj::Error>("Invalid argument 1 for array update: " + obj::toString(obj->type), obj::ErrorType::TypeError);

        auto indexExpr = std::move(evalExpression(arguments.at(1), environment));
        if (indexExpr->type == obj::ObjectType::Error)
            return indexExpr;

        if (indexExpr->type != obj::ObjectType::Integer)
            return std::make_shared<obj::Error>("Invalid argument 1 for update: " + obj::toString(indexExpr->type), obj::ErrorType::TypeError);

        auto intObj = static_cast<obj::Integer *>(indexExpr.get());
        size_t arraySize = static_cast<int>(arrayObj->value.size());
        size_t finalIndex = normalizedArrayIndex(intObj->value, arraySize);
        if (finalIndex >= arraySize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intObj->value) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError);

        auto validObj = std::move(evalExpression(arguments.at(2), environment));
        if (validObj->type == obj::ObjectType::Error)
            return validObj;

        // check if we can keep the ArrayComplex or need to demote to Array
        if (validObj->type != obj::ObjectType::Complex)
        {
            return std::make_shared<obj::Error>("Invalid argument 1 for update [complex]: " + obj::toString(validObj->type), obj::ErrorType::TypeError);
        }

        arrayObj->value[finalIndex] = static_cast<obj::Complex *>(validObj.get())->value;
        return obj;
    }

    std::shared_ptr<obj::Object> updateString(std::shared_ptr<obj::Object> obj, const std::vector<ast::Expression *> &arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        auto stringObj = dynamic_cast<obj::String *>(obj.get());
        if (!stringObj)
            return std::make_shared<obj::Error>("Invalid argument 1 for string update: " + obj::toString(obj->type), obj::ErrorType::TypeError);

        auto indexExpr = std::move(evalExpression(arguments.at(1), environment));
        if (indexExpr->type == obj::ObjectType::Error)
            return indexExpr;

        if (indexExpr->type != obj::ObjectType::Integer)
            return std::make_shared<obj::Error>("Invalid argument 1 for update: " + obj::toString(indexExpr->type), obj::ErrorType::TypeError);

        auto intObj = static_cast<obj::Integer *>(indexExpr.get());
        size_t stringSize = static_cast<int>(stringObj->value.size());
        size_t finalIndex = normalizedArrayIndex(intObj->value, stringSize);
        if (finalIndex >= stringSize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intObj->value) + ", string size=" + std::to_string(stringSize), obj::ErrorType::IndexError);

        auto validObj = std::move(evalExpression(arguments.at(2), environment));
        if (validObj->type == obj::ObjectType::Error)
            return validObj;

        auto stringRhs = dynamic_cast<obj::String *>(validObj.get());
        if (!stringRhs)
            return std::make_shared<obj::Error>("Invalid right hand side for string update: " + obj::toString(stringRhs->type), obj::ErrorType::TypeError);

        if (stringRhs->value.empty())
            return obj;

        if (stringRhs->value.size() == 1)
            stringObj->value[finalIndex] = stringRhs->value[0];
        else
            stringObj->value.replace(finalIndex, 1, stringRhs->value);
        return obj;
    }

    std::shared_ptr<obj::Object> updateDictionary(std::shared_ptr<obj::Object> obj, const std::vector<ast::Expression *> &arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        auto dictObj = dynamic_cast<obj::Dictionary *>(obj.get());
        if (!dictObj)
            return std::make_shared<obj::Error>("Invalid argument 1 for dictionary update: " + obj::toString(obj->type), obj::ErrorType::TypeError);

        auto indexExpr = evalExpression(arguments.at(1), environment);
        if (indexExpr->type == obj::ObjectType::Error)
            return indexExpr;

        auto validObj = evalExpression(arguments.at(2), environment);
        if (validObj->type == obj::ObjectType::Error)
            return validObj;

        if (isValueAssigned(validObj))
            dictObj->value.insert_or_assign(indexExpr, validObj->clone());
        else
            dictObj->value.insert_or_assign(indexExpr, validObj);
        return obj;
    }

    std::shared_ptr<obj::Object> updateImpl(const std::vector<ast::Expression *> &arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (arguments.size() != 3)
            return std::make_shared<obj::Error>("update: expected 3 arguments", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments.front(), environment);
        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Error:
            return evaluatedExpr;
        case obj::ObjectType::Array:
            return updateArray(evaluatedExpr, arguments, environment);
        case obj::ObjectType::ArrayDouble:
            return updateArrayDouble(evaluatedExpr, arguments, environment);
        case obj::ObjectType::ArrayComplex:
            return updateArrayComplex(evaluatedExpr, arguments, environment);
        case obj::ObjectType::Dictionary:
            return updateDictionary(evaluatedExpr, arguments, environment);
        case obj::ObjectType::String:
            return updateString(evaluatedExpr, arguments, environment);
        default:
            return std::make_shared<obj::Error>("Invalid type for update: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
        }
    }

    std::shared_ptr<obj::Object> update(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 3)
            return std::make_shared<obj::Error>("update: expected 3 arguments", obj::ErrorType::TypeError);

        std::vector<ast::Expression *> args;
        for (const auto &arg : *arguments)
            args.push_back(arg.get());

        return updateImpl(args, environment);
    }

    std::shared_ptr<obj::Object> append(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return std::make_shared<obj::Error>("append: expected 2 arguments", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        auto errorObj = dynamic_cast<obj::Error *>(evaluatedExpr.get());
        if (errorObj)
            return evaluatedExpr;

        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Array:
        case obj::ObjectType::ArrayDouble:
        case obj::ObjectType::ArrayComplex:
        {
            auto evaluatedExprSecond = evalExpression(arguments->back().get(), environment);
            if (evaluatedExprSecond->type == obj::ObjectType::Error)
                return evaluatedExprSecond;
            return array_push_back(evaluatedExpr, {evaluatedExprSecond});
        }
        default:
            return std::make_shared<obj::Error>("Invalid argument for first argument for append: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
        }
    }

    std::shared_ptr<obj::Object> slice(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 3)
            return std::make_shared<obj::Error>("slice: expected 3 arguments", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);

        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Error:
            return evaluatedExpr;
        case obj::ObjectType::Array:
        case obj::ObjectType::ArrayDouble:
        case obj::ObjectType::ArrayComplex:
            break;
        default:
            return std::make_shared<obj::Error>("Invalid argument for first argument for slice: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
        };
        auto evaluatedExprSecond = evalExpression(arguments->at(1).get(), environment);
        auto errorObjSecond = dynamic_cast<obj::Error *>(evaluatedExprSecond.get());
        if (errorObjSecond)
            return evaluatedExprSecond;
        auto startIndex = dynamic_cast<obj::Integer *>(evaluatedExprSecond.get());
        if (!startIndex)
            return std::make_shared<obj::Error>("Invalid argument for second argument for slice: " + obj::toString(evaluatedExpr->type) + ", expected integer", obj::ErrorType::TypeError);

        auto evaluatedExprThird = evalExpression(arguments->at(2).get(), environment);
        auto errorObjThird = dynamic_cast<obj::Error *>(evaluatedExprThird.get());
        if (errorObjThird)
            return evaluatedExprThird;
        auto stopIndex = dynamic_cast<obj::Integer *>(evaluatedExprThird.get());
        if (!stopIndex)
            return std::make_shared<obj::Error>("Invalid argument for third argument for slice: " + obj::toString(evaluatedExpr->type) + ", expected integer", obj::ErrorType::TypeError);

        const size_t arrayLength = arrayLikeLength(evaluatedExpr.get());
        const size_t startValue = normalizedArrayIndex(startIndex->value, arrayLength);
        const size_t stopValue = std::max(startValue, normalizedArrayIndex(stopIndex->value, arrayLength));

        if (startValue >= arrayLength)
            return std::make_shared<obj::Error>("Slicing error, start index=" + std::to_string(startValue) + ", array size=" + std::to_string(arrayLength), obj::ErrorType::IndexError, arguments->at(1)->token);
        if (stopValue > arrayLength)
            return std::make_shared<obj::Error>("Slicing error, stop index=" + std::to_string(stopValue) + ", array size=" + std::to_string(arrayLength), obj::ErrorType::IndexError, arguments->at(2)->token);

        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Array:
        {
            auto arrayObj = static_cast<obj::Array *>(evaluatedExpr.get());
            std::vector<std::shared_ptr<obj::Object>> values;
            values.reserve(stopValue - startValue + 1);
            for (size_t i = startValue; i < stopValue; ++i)
                values.push_back(arrayObj->value.at(i));
            return std::make_shared<obj::Array>(values);
        }
        case obj::ObjectType::ArrayDouble:
        {
            auto arrayObj = static_cast<obj::ArrayDouble *>(evaluatedExpr.get());
            std::vector<double> values;
            values.reserve(stopValue - startValue + 1);
            for (size_t i = startValue; i < stopValue; ++i)
                values.push_back(arrayObj->value.at(i));
            return std::make_shared<obj::ArrayDouble>(values);
        }
        case obj::ObjectType::ArrayComplex:
        {
            auto arrayObj = static_cast<obj::ArrayComplex *>(evaluatedExpr.get());
            std::vector<std::complex<double>> values;
            values.reserve(stopValue - startValue + 1);
            for (size_t i = startValue; i < stopValue; ++i)
                values.push_back(arrayObj->value.at(i));
            return std::make_shared<obj::ArrayComplex>(values);
        }
        };
        return std::make_shared<obj::Error>("Slicing general error", obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> rotate(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        /* rotate an array in-place with given number of places, returns the array */
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return std::make_shared<obj::Error>("rotate: expected 2 arguments", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;

        auto evaluatedExprSecond = evalExpression(arguments->at(1).get(), environment);
        if (evaluatedExprSecond->type == obj::ObjectType::Error)
            return evaluatedExprSecond;

        return array_rotate(evaluatedExpr, {evaluatedExprSecond});
    }

    std::shared_ptr<obj::Object> rotated(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        /* copies and rotates an array with given number of places */
        if (!arguments)
            return NullObject;

        if (arguments->size() != 2)
            return std::make_shared<obj::Error>("rotate: expected 2 arguments", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->type == obj::ObjectType::Error)
            return evaluatedExpr;

        auto evaluatedExprSecond = evalExpression(arguments->at(1).get(), environment);
        if (evaluatedExprSecond->type == obj::ObjectType::Error)
            return evaluatedExprSecond;

        return array_rotated(evaluatedExpr, {evaluatedExprSecond});
    }

    std::shared_ptr<obj::Object> reverse(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        /* reverse an array/string in-place with given number of places, returns the array/string */
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("reverse: expected 1 arguments", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Error:
            return evaluatedExpr;
        case obj::ObjectType::Array:
        case obj::ObjectType::ArrayDouble:
        case obj::ObjectType::ArrayComplex:
            return array_reverse(evaluatedExpr, {});
        case obj::ObjectType::String:
        {
            auto stringObj = dynamic_cast<obj::String *>(evaluatedExpr.get());
            std::reverse(stringObj->value.begin(), stringObj->value.end());
            return evaluatedExpr;
        }
        default:
            return std::make_shared<obj::Error>("Invalid argument for first argument for reversed: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
        }
    }

    bool isSmallerThan(const std::shared_ptr<obj::Object> &a, const std::shared_ptr<obj::Object> &b)
    {
        obj::Object *compareResult = evalInfixOperator(TokenType::LT, a.get(), b.get());
        if (compareResult && compareResult->type == obj::ObjectType::Boolean)
        {
            obj::Boolean *compareResultBoolean = static_cast<obj::Boolean *>(compareResult);
            return compareResultBoolean->value;
        }
        throw std::runtime_error("Failed to compare objects");
    }

    std::shared_ptr<obj::Object> sort(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 2)
            return std::make_shared<obj::Error>("sort: expected 1 or 2 arguments", obj::ErrorType::TypeError);

        obj::Function *customComparator = nullptr;
        std::shared_ptr<obj::Object> customComparatorObj;
        if (arguments->size() == 2)
        {
            customComparatorObj = evalExpression(arguments->back().get(), environment);
            if (customComparatorObj->type != obj::ObjectType::Function)
                return std::make_shared<obj::Error>("sort: expected argument 2 to be a function", obj::ErrorType::TypeError);
            customComparator = static_cast<obj::Function *>(customComparatorObj.get());
        }

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Error:
            return evaluatedExpr;
        case obj::ObjectType::Array:
        {
            auto arrayObj = dynamic_cast<obj::Array *>(evaluatedExpr.get());
            try
            {
                // line below will deallocate a value when an exception is generated in isSmallerThan
                // to the best understanding this is a bug in VC, either compiler or STL
                // std::sort(arrayObj->value.begin(), arrayObj->value.end(), isSmallerThan);
                std::vector<int> ordering(arrayObj->value.size());
                for (size_t i = 0; i < ordering.size(); ++i)
                    ordering[i] = i;

                if (customComparator)
                {
                    std::sort(ordering.begin(), ordering.end(), [environment, customComparator, arrayObj](const int &a, const int &b) -> bool
                              {
                                auto retValue = evalFunctionWithArguments(customComparator, {arrayObj->value[a], arrayObj->value[b]}, environment);
                                if (retValue->type == obj::ObjectType::Boolean)
                                    return static_cast<obj::Boolean*>(retValue.get())->value;
                                throw std::runtime_error("Invalid return type from comparator"); });
                }
                else
                {
                    std::sort(ordering.begin(), ordering.end(), [arrayObj](const int &a, const int &b) -> bool
                              { return isSmallerThan(arrayObj->value[a], arrayObj->value[b]); });
                }

                std::vector<std::shared_ptr<obj::Object>> temp;
                temp.resize(ordering.size());
                for (size_t i = 0; i < ordering.size(); ++i)
                    temp[i] = std::move(arrayObj->value[ordering[i]]);

                arrayObj->value = std::move(temp);
            }
            catch (const std::exception & /*e*/)
            {
                return std::make_shared<obj::Boolean>(false);
            }
            return std::make_shared<obj::Boolean>(true);
        }
        case obj::ObjectType::ArrayDouble:
        {
            auto arrayObj = dynamic_cast<obj::ArrayDouble *>(evaluatedExpr.get());
            if (customComparator)
            {
                try
                {
                    std::sort(arrayObj->value.begin(), arrayObj->value.end(), [environment, customComparator, arrayObj](const double &a, const double &b) -> bool
                              {
                                auto retValue = evalFunctionWithArguments(customComparator, {std::make_shared<obj::Double>(a), std::make_shared<obj::Double>(b)}, environment);
                                if (retValue->type == obj::ObjectType::Boolean)
                                    return static_cast<obj::Boolean*>(retValue.get())->value;
                                throw std::runtime_error("Invalid return type from comparator"); });
                }
                catch (std::exception & /*e*/)
                {
                    return std::make_shared<obj::Boolean>(false);
                }
            }
            else
            {
                std::sort(arrayObj->value.begin(), arrayObj->value.end());
            }
            return std::make_shared<obj::Boolean>(true);
        }
        case obj::ObjectType::ArrayComplex:
        {
            auto arrayObj = dynamic_cast<obj::ArrayComplex *>(evaluatedExpr.get());
            if (!customComparator)
                return std::make_shared<obj::Boolean>(false);

            try
            {
                std::sort(arrayObj->value.begin(), arrayObj->value.end(), [environment, customComparator, arrayObj](const std::complex<double> &a, const std::complex<double> &b) -> bool
                          {
                            auto retValue = evalFunctionWithArguments(customComparator, {std::make_shared<obj::Complex>(a), std::make_shared<obj::Complex>(b)}, environment);
                            if (retValue->type == obj::ObjectType::Boolean)
                                return static_cast<obj::Boolean*>(retValue.get())->value;
                            throw std::runtime_error("Invalid return type from comparator"); });
            }
            catch (std::exception & /*e*/)
            {
                return std::make_shared<obj::Boolean>(false);
            }
            return std::make_shared<obj::Boolean>(true);
        }
        default:
            return std::make_shared<obj::Error>("Invalid argument for first argument for sort: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
        }
    }

    std::shared_ptr<obj::Object> sorted(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 2)
            return std::make_shared<obj::Error>("sorted: expected 1 or 2 arguments", obj::ErrorType::TypeError);

        obj::Function *customComparator = nullptr;
        std::shared_ptr<obj::Object> customComparatorObj;
        if (arguments->size() == 2)
        {
            customComparatorObj = evalExpression(arguments->back().get(), environment);
            if (customComparatorObj->type != obj::ObjectType::Function)
                return std::make_shared<obj::Error>("sort: expected argument 2 to be a function", obj::ErrorType::TypeError);
            customComparator = static_cast<obj::Function *>(customComparatorObj.get());
        }

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Error:
            return evaluatedExpr;
        case obj::ObjectType::Array:
        {
            auto arrayObj = dynamic_cast<obj::Array *>(evaluatedExpr.get());
            std::vector<std::shared_ptr<obj::Object>> values(arrayObj->value);

            try
            {
                // line below will deallocate a value when an exception is generated in isSmallerThan
                // to the best understanding this is a bug in VC, either compiler or STL
                // std::sort(arrayObj->value.begin(), arrayObj->value.end(), isSmallerThan);

                std::vector<int> ordering(values.size());
                for (size_t i = 0; i < ordering.size(); ++i)
                    ordering[i] = i;

                if (customComparator)
                {
                    std::sort(ordering.begin(), ordering.end(), [environment, customComparator, arrayObj](const int &a, const int &b) -> bool
                              {
                                auto retValue = evalFunctionWithArguments(customComparator, {arrayObj->value[a], arrayObj->value[b]}, environment);
                                if (retValue->type == obj::ObjectType::Boolean)
                                    return static_cast<obj::Boolean*>(retValue.get())->value;
                                throw std::runtime_error("Invalid return type from comparator"); });
                }
                else
                {
                    std::sort(ordering.begin(), ordering.end(), [values](const int &a, const int &b) -> bool
                              { return isSmallerThan(values[a], values[b]); });
                }

                std::vector<std::shared_ptr<obj::Object>> temp;
                temp.resize(ordering.size());
                for (size_t i = 0; i < ordering.size(); ++i)
                    temp[i] = std::move(arrayObj->value[ordering[i]]);

                values = std::move(temp);
            }
            catch (const std::exception & /*e*/)
            {
                return std::make_shared<obj::Array>(values);
            }
            return std::make_shared<obj::Array>(values);
        }
        case obj::ObjectType::ArrayDouble:
        {
            auto arrayObj = dynamic_cast<obj::ArrayDouble *>(evaluatedExpr.get());
            std::vector<double> values(arrayObj->value);
            std::sort(values.begin(), values.end());
            return std::make_shared<obj::ArrayDouble>(values);
        }
        default:
            return std::make_shared<obj::Error>("Invalid argument for first argument for sort: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
        }
    }

    std::shared_ptr<obj::Object> is_sorted(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() > 2)
            return std::make_shared<obj::Error>("sorted: expected 1 or 2 arguments", obj::ErrorType::TypeError);

        obj::Function *customComparator = nullptr;
        std::shared_ptr<obj::Object> customComparatorObj;
        if (arguments->size() == 2)
        {
            customComparatorObj = evalExpression(arguments->back().get(), environment);
            if (customComparatorObj->type != obj::ObjectType::Function)
                return std::make_shared<obj::Error>("sort: expected argument 2 to be a function", obj::ErrorType::TypeError);
            customComparator = static_cast<obj::Function *>(customComparatorObj.get());
        }

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Error:
            return evaluatedExpr;
        case obj::ObjectType::Array:
        {
            auto arrayObj = dynamic_cast<obj::Array *>(evaluatedExpr.get());

            try
            {
                if (customComparator)
                {
                    const bool isSorted = std::is_sorted(arrayObj->value.begin(), arrayObj->value.end(), [environment, customComparator, arrayObj](const std::shared_ptr<obj::Object> &a, const std::shared_ptr<obj::Object> &b) -> bool
                                                         {
                                auto retValue = evalFunctionWithArguments(customComparator, {a,b}, environment);
                                if (retValue->type == obj::ObjectType::Boolean)
                                    return static_cast<obj::Boolean*>(retValue.get())->value;
                                throw std::runtime_error("Invalid return type from comparator"); });
                    return std::make_shared<obj::Boolean>(isSorted);
                }
                else
                {
                    const bool isSorted = std::is_sorted(arrayObj->value.begin(), arrayObj->value.end(), isSmallerThan);
                    return std::make_shared<obj::Boolean>(isSorted);
                }
            }
            catch (std::exception & /*e*/)
            {
                return std::make_shared<obj::Boolean>(false);
            }
            return std::make_shared<obj::Boolean>(false);
        }
        case obj::ObjectType::ArrayDouble:
        {
            auto arrayObj = dynamic_cast<obj::ArrayDouble *>(evaluatedExpr.get());
            if (customComparator)
            {
                try
                {
                    const bool isSorted = std::is_sorted(arrayObj->value.begin(), arrayObj->value.end(), [environment, customComparator, arrayObj](const double &a, const double &b) -> bool
                                                         {
                                auto retValue = evalFunctionWithArguments(customComparator, {std::make_shared<obj::Double>(a), std::make_shared<obj::Double>(b)}, environment);
                                if (retValue->type == obj::ObjectType::Boolean)
                                    return static_cast<obj::Boolean*>(retValue.get())->value;
                                throw std::runtime_error("Invalid return type from comparator"); });
                    return std::make_shared<obj::Boolean>(isSorted);
                }
                catch (std::exception & /*e*/)
                {
                    return std::make_shared<obj::Boolean>(false);
                }
            }
            else
            {
                const bool isSorted = std::is_sorted(arrayObj->value.begin(), arrayObj->value.end());
                return std::make_shared<obj::Boolean>(isSorted);
            }
        }
        case obj::ObjectType::ArrayComplex:
        {
            auto arrayObj = dynamic_cast<obj::ArrayComplex *>(evaluatedExpr.get());
            if (!customComparator)
                return std::make_shared<obj::Boolean>(false);

            try
            {
                const bool isSorted = std::is_sorted(arrayObj->value.begin(), arrayObj->value.end(), [environment, customComparator, arrayObj](const std::complex<double> &a, const std::complex<double> &b) -> bool
                                                     {
                            auto retValue = evalFunctionWithArguments(customComparator, {std::make_shared<obj::Complex>(a), std::make_shared<obj::Complex>(b)}, environment);
                            if (retValue->type == obj::ObjectType::Boolean)
                                return static_cast<obj::Boolean*>(retValue.get())->value;
                            throw std::runtime_error("Invalid return type from comparator"); });
                return std::make_shared<obj::Boolean>(isSorted);
            }
            catch (std::exception & /*e*/)
            {
                return std::make_shared<obj::Boolean>(false);
            }
            return std::make_shared<obj::Boolean>(true);
        }
        default:
            return std::make_shared<obj::Error>("Invalid argument for first argument for sort: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
        }
    }

    std::shared_ptr<obj::Object> reversed(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        /* reverse an array/string with given number of places, returns new array/string */
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("reverse: expected 1 arguments", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        switch (evaluatedExpr->type)
        {
        case obj::ObjectType::Error:
            return evaluatedExpr;
        case obj::ObjectType::Array:
        case obj::ObjectType::ArrayDouble:
        case obj::ObjectType::ArrayComplex:
            return array_reversed(evaluatedExpr, {});
        case obj::ObjectType::String:
        {
            auto stringObj = dynamic_cast<obj::String *>(evaluatedExpr.get());
            std::string values(stringObj->value);
            std::reverse(values.begin(), values.end());
            return std::make_shared<obj::String>(obj::String(values));
        }
        default:
            return std::make_shared<obj::Error>("Invalid argument for first argument for reversed: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
        }
    }

    std::shared_ptr<obj::Object> values(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("values: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        auto errorObj = dynamic_cast<obj::Error *>(evaluatedExpr.get());
        if (errorObj)
            return evaluatedExpr;

        auto dictObj = dynamic_cast<obj::Dictionary *>(evaluatedExpr.get());
        if (dictObj)
        {
            std::vector<std::shared_ptr<obj::Object>> values;
            for (auto it : dictObj->value)
                values.push_back(it.second);
            return std::make_shared<obj::Array>(obj::Array(values));
        }

        return std::make_shared<obj::Error>("Invalid type for values: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> keys(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("values: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        auto errorObj = dynamic_cast<obj::Error *>(evaluatedExpr.get());
        if (errorObj)
            return evaluatedExpr;

        auto dictObj = dynamic_cast<obj::Dictionary *>(evaluatedExpr.get());
        if (dictObj)
        {
            std::vector<std::shared_ptr<obj::Object>> values;
            for (auto it : dictObj->value)
                values.push_back(it.first);
            return std::make_shared<obj::Array>(obj::Array(values));
        }

        return std::make_shared<obj::Error>("Invalid type for keys: " + obj::toString(evaluatedExpr->type), obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> iter_impl(const std::shared_ptr<obj::Object> &obj)
    {
        switch (obj->type)
        {
        case obj::ObjectType::Array:
            return std::make_shared<obj::ArrayIterator<obj::Array>>(std::dynamic_pointer_cast<obj::Array>(obj), 0);
        case obj::ObjectType::ArrayDouble:
            return std::make_shared<obj::ArrayIterator<obj::ArrayDouble>>(std::dynamic_pointer_cast<obj::ArrayDouble>(obj), 0);
        case obj::ObjectType::ArrayComplex:
            return std::make_shared<obj::ArrayIterator<obj::ArrayComplex>>(std::dynamic_pointer_cast<obj::ArrayComplex>(obj), 0);
        case obj::ObjectType::Dictionary:
            return std::make_shared<obj::DictionaryIterator>(std::dynamic_pointer_cast<obj::Dictionary>(obj), static_cast<obj::Dictionary *>(obj.get())->value.begin());
        case obj::ObjectType::Set:
            return std::make_shared<obj::SetIterator>(std::dynamic_pointer_cast<obj::Set>(obj), static_cast<obj::Set *>(obj.get())->value.begin());
        case obj::ObjectType::String:
            return std::make_shared<obj::StringIterator>(std::dynamic_pointer_cast<obj::String>(obj), 0);
        case obj::ObjectType::Range:
            return std::make_shared<obj::RangeIterator>(std::dynamic_pointer_cast<obj::Range>(obj), static_cast<obj::Range *>(obj.get())->lower);
        }

        return NullObject;
    }

    std::shared_ptr<obj::Object> makeBuiltInFunctionObj(obj::TBuiltinFunction fn, const std::string &argTypeStr, const std::string &returnTypeStr)
    {
        auto func = std::make_shared<obj::Builtin>();
        func->function = fn;
        func->declaredType = typing::makeFunctionType(argTypeStr, returnTypeStr);
        return func;
    }

    typedef std::shared_ptr<obj::Object> (*TBuiltinTypeFunction)(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Object> &self);
}

namespace
{
    std::unordered_map<std::string, std::shared_ptr<obj::Module>> builtinModules;

    void fillBuiltinModules()
    {
        // ERROR-TYPE module
        auto errorTypeBuiltinModule = builtin::makeModuleErrorType();
        builtinModules.insert_or_assign("error_type", std::move(errorTypeBuiltinModule));

        // MATH module
        auto mathBuiltinModule = builtin::createMathModule();
        builtinModules.insert_or_assign("math", std::move(mathBuiltinModule));

        // JSON module
        auto jsonBuiltinModule = builtin::createJsonModule();
        builtinModules.insert_or_assign("json", std::move(jsonBuiltinModule));

        // OS module
        auto osBuiltinModule = builtin::makeModuleOS();
        builtinModules.insert_or_assign("os", std::move(osBuiltinModule));

        // OS module
        auto regexBuiltinModule = builtin::createRegexModule();
        builtinModules.insert_or_assign("regex", std::move(regexBuiltinModule));

        auto timeBuiltinModule = builtin::createTimeModule();
        builtinModules.insert_or_assign("time", std::move(timeBuiltinModule));

        auto threadingBuiltinModule = builtin::createThreadingModule();
        builtinModules.insert_or_assign("threading", std::move(threadingBuiltinModule));

        auto typingBuiltinModule = builtin::createTypingModule();
        builtinModules.insert_or_assign("typing", std::move(typingBuiltinModule));
    }

    std::unordered_map<obj::ObjectType, std::shared_ptr<obj::BuiltinType>> builtinTypes;

    void fillBuiltinTypes()
    {
        // ERROR
        auto errorBuiltinType = builtin::makeBuiltinTypeError();
        builtinTypes.insert_or_assign(errorBuiltinType->builtinObjectType, std::move(errorBuiltinType));

        // ARRAY
        auto arrayBuiltinTypes = builtin::makeBuiltinTypeArrays();
        for (const auto &arrayBuiltinType : arrayBuiltinTypes)
        {
            builtinTypes.insert_or_assign(arrayBuiltinType->builtinObjectType, std::move(arrayBuiltinType));
        }

        // DICTIONARY
        auto dictionaryBuiltinType = builtin::makeBuiltinTypeDictionary();
        builtinTypes.insert_or_assign(dictionaryBuiltinType->builtinObjectType, std::move(dictionaryBuiltinType));

        // IO
        auto ioBuiltinType = builtin::makeBuiltinTypeIo();
        builtinTypes.insert_or_assign(ioBuiltinType->builtinObjectType, std::move(ioBuiltinType));

        //  SET
        auto setBuiltinType = builtin::makeBuiltinTypeSet();
        builtinTypes.insert_or_assign(setBuiltinType->builtinObjectType, std::move(setBuiltinType));

        // STRING
        auto stringBuiltinType = builtin::makeBuiltinTypeString();
        builtinTypes.insert_or_assign(stringBuiltinType->builtinObjectType, std::move(stringBuiltinType));

        // THREAD
        auto threadBuiltinType = builtin::makeBuiltinTypeThread();
        builtinTypes.insert_or_assign(threadBuiltinType->builtinObjectType, std::move(threadBuiltinType));
    }

    std::unordered_map<std::string, std::shared_ptr<obj::Object>> builtins;

    void fillBuiltins()
    {
        builtins = std::unordered_map<std::string, std::shared_ptr<obj::Object>>{
            // all objects - debugging of addresses
            {"address", builtin::makeBuiltInFunctionObj(&builtin::address, "all", "int")},
            // all objects - debugging of types
            {"internal_type_str", builtin::makeBuiltInFunctionObj(&builtin::internal_type_str, "all", "str")},

            // dictionary and set support
            {"lookup_hashable", builtin::makeBuiltInFunctionObj(&builtin::lookup_hashable, "all", "bool")},
            {"lookup_hash", builtin::makeBuiltInFunctionObj(&builtin::lookup_hash, "all", "int")},
            {"lookup_equal", builtin::makeBuiltInFunctionObj(&builtin::lookup_equal, "all, all", "bool")},

            // iteration and for-loop support
            //{"iter", builtin::makeBuiltInFunctionObj(&builtin::iter, "all", "iter")},
            //{"iterable", builtin::makeBuiltInFunctionObj(&builtin::iterable, "all", "bool")},

            // querying for frozen objects
            {"frozen", builtin::makeBuiltInFunctionObj(&builtin::frozen, "all", "bool")},
            {"freeze", builtin::makeBuiltInFunctionObj(&builtin::freeze, "all", "all")},
            {"defrost", builtin::makeBuiltInFunctionObj(&builtin::defrost, "all", "all")},
            {"freezer", builtin::makeBuiltInFunctionObj(&builtin::freezer, "all", "freezer")},

            // type system
            {"type_str", builtin::makeBuiltInFunctionObj(&builtin::type_str, "all", "str")},
            //{"comparable", builtin::makeBuiltInFunctionObj(&builtin::comparable, "[all]", "bool")},
            //{"orderable", builtin::makeBuiltInFunctionObj(&builtin::orderable, "[all]", "bool")},

            // errors
            {"error", builtin::makeBuiltInFunctionObj(&builtin::error, "str, int", "error")},

            // duplicating objects
            {"clone", builtin::makeBuiltInFunctionObj(&builtin::clone, "all", "all")},

            // documentation
            {"doc", builtin::makeBuiltInFunctionObj(&builtin::doc, "all", "str")}, /*< extract documentation from functions and properties */

            // communication with outside world
            {"print", builtin::makeBuiltInFunctionObj(&builtin::print, "all", "null")},       /*< print to console on stdout*/
            {"eprint", builtin::makeBuiltInFunctionObj(&builtin::eprint, "all", "null")},     /*< print to console on stderr*/
            {"input_line", builtin::makeBuiltInFunctionObj(&builtin::input_line, "", "str")}, /*< grab input from stdin */
            {"version", builtin::makeBuiltInFunctionObj(&builtin::version, "", "[int]")},     /*< request version as array */
            {"arg", builtin::makeBuiltInFunctionObj(&builtin::arg, "", "[str]")},             /*< command line args as array */
            {"format", builtin::makeBuiltInFunctionObj(&builtin::format, "str, all", "str")}, /*< format a string */

            // interpreter control
            {"run", builtin::makeBuiltInFunctionObj(&builtin::run, "str", "null")},
            {"run_once", builtin::makeBuiltInFunctionObj(&builtin::run_once, "str", "null")},
            {"exit", builtin::makeBuiltInFunctionObj(&builtin::exit, "int", "null")}, /*< exiting the interpreter */

            // module control
            {"import", builtin::makeBuiltInFunctionObj(&builtin::import, "str", "module")},

            // environment control/query
            {"scope_names", builtin::makeBuiltInFunctionObj(&builtin::scope_names, "", "[str]")},

            // creation of non-trivial empty builtins and to account for empty sets
            // which have no literal
            {"array", builtin::makeBuiltInFunctionObj(&builtin::array, "", "[all]")},
            {"array_double", builtin::makeBuiltInFunctionObj(&builtin::array_double, "", "[double]")},
            {"array_complex", builtin::makeBuiltInFunctionObj(&builtin::array_complex, "", "[complex]")},
            {"complex", builtin::makeBuiltInFunctionObj(&builtin::complex, "", "complex")},
            {"dict", builtin::makeBuiltInFunctionObj(&builtin::dict, "", "{all:all}")},
            {"set", builtin::makeBuiltInFunctionObj(&builtin::set, "", "{all}")},

            // arrays
            {"append", builtin::makeBuiltInFunctionObj(&builtin::append, "[all], all", "[all]")},
            {"slice", builtin::makeBuiltInFunctionObj(&builtin::slice, "[all], int, int", "[all]")},
            {"update", builtin::makeBuiltInFunctionObj(&builtin::update, "[all],int, all", "[all]")},
            {"rotate", builtin::makeBuiltInFunctionObj(&builtin::rotate, "[all],int", "[all]")},
            {"reverse", builtin::makeBuiltInFunctionObj(&builtin::reverse, "[all]", "[all]")},
            {"sort", builtin::makeBuiltInFunctionObj(&builtin::sort, "[all]", "<bool>")},
            {"reversed", builtin::makeBuiltInFunctionObj(&builtin::reversed, "[all]", "[all]")},
            {"rotated", builtin::makeBuiltInFunctionObj(&builtin::rotated, "[all], int", "[all]")},
            {"sorted", builtin::makeBuiltInFunctionObj(&builtin::sorted, "[all]", "[all]")},
            {"is_sorted", builtin::makeBuiltInFunctionObj(&builtin::is_sorted, "[all]", "bool")},

            // ranges
            {"range", builtin::makeBuiltInFunctionObj(&builtin::range, "int,int", "range")},

            // arrays/dictionary/string
            {"len", builtin::makeBuiltInFunctionObj(&builtin::len, "<[all],{all:all},str>", "int")},

            // dictionary
            {"values", builtin::makeBuiltInFunctionObj(&builtin::values, "{all:all}", "[all]")},
            {"keys", builtin::makeBuiltInFunctionObj(&builtin::keys, "{all:all}", "[all]")},

            // string functions
            {"to_bool", builtin::makeBuiltInFunctionObj(&builtin::to_bool, "str", "bool")},
            {"to_int", builtin::makeBuiltInFunctionObj(&builtin::to_int, "str", "int")},
            {"to_double", builtin::makeBuiltInFunctionObj(&builtin::to_double, "str", "double")},

            // io functions
            {"open", builtin::makeBuiltInFunctionObj(&builtin::open, "str", "io")},
        };
    }

    void clearBuiltinModules()
    {
        builtinModules.clear();
    }

    void clearBuiltinTypes()
    {
        builtinTypes.clear();
    }

    void clearBuiltins()
    {
        builtins.clear();
    }
}

void initialize()
{
    fillBuiltins();
    fillBuiltinTypes();
    fillBuiltinModules();
}

void finalize()
{
    clearBuiltinModules();
    clearBuiltinTypes();
    clearBuiltins();

    NullObject.reset();
}

std::shared_ptr<obj::Object> getBuiltin(const std::string &name)
{
    auto foundBuiltin = builtins.find(name);
    if (foundBuiltin != builtins.end())
        return foundBuiltin->second;
    return nullptr;
}

/* check if there are objects that are going out of scope in this environment, if so
 *  call and reset their destructors so that only thing that is left is the destruction
 *  of the C++ object, but nothing is left in user space to act on
 */
std::shared_ptr<obj::Object> evalUserObjectDestructors(const std::shared_ptr<obj::Environment> &environment)
{
    if (!environment)
        throw std::runtime_error("Unexpected NULL environment");

    for (const auto &[varName, tokenSharedObj] : environment->store)
    {
        if (tokenSharedObj.obj.use_count() == 1 && tokenSharedObj.obj->type == obj::ObjectType::UserObject)
        {
            auto userObj = static_cast<obj::UserObject *>(tokenSharedObj.obj.get());
            auto retValue = userObj->evalAndResetDestructor(environment);

            if (retValue->type == obj::ObjectType::Error || retValue->type == obj::ObjectType::Exit)
                return retValue;
        }
    }

    return NullObject;
}

std::shared_ptr<obj::Object> addTokenInCaseOfError(std::shared_ptr<obj::Object> object, const Token &token)
{
    if (object->type == obj::ObjectType::Error)
    {
        auto errorObj = static_cast<obj::Error *>(object.get());
        if (errorObj->token.type == TokenType::NOT_SET)
            errorObj->token = token;
    }
    return object;
}

std::shared_ptr<obj::Object> evalArrayIndexExpression(obj::Array *arrayExpr, std::shared_ptr<obj::Object> evaluatedIndex, ast::IndexExpression *indexExpr)
{
    if (!arrayExpr)
        return std::make_shared<obj::Error>("NULL array", obj::ErrorType::TypeError, indexExpr->token);

    if (arrayExpr->value.empty())
        return std::make_shared<obj::Error>("Attempting index in empty array", obj::ErrorType::IndexError, indexExpr->token);

    size_t arraySize = static_cast<int>(arrayExpr->value.size());
    switch (evaluatedIndex->type)
    {
    case obj::ObjectType::Integer:
    {
        auto intLiteral = static_cast<obj::Integer *>(evaluatedIndex.get());
        size_t finalIndex = normalizedArrayIndex(intLiteral->value, arraySize);
        if (finalIndex >= arraySize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intLiteral->value) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError, indexExpr->token);
        return arrayExpr->value[finalIndex];
    }
    case obj::ObjectType::Range:
    {
        auto rangeLiteral = static_cast<obj::Range *>(evaluatedIndex.get());
        std::vector<std::shared_ptr<obj::Object>> ret;

        for (const auto &index : rangeLiteral->values())
        {
            size_t finalIndex = normalizedArrayIndex(index, arraySize);
            if (finalIndex >= arraySize)
                return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(index) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError, indexExpr->token);
            ret.push_back(arrayExpr->value[finalIndex]);
        }
        return std::make_shared<obj::Array>(ret);
    }
    default:
        return std::make_shared<obj::Error>("Indexing in array must be done with Integer or Range but found " + toString(evaluatedIndex->type), obj::ErrorType::TypeError, indexExpr->token);
    };
}

std::shared_ptr<obj::Object> evalArrayDoubleIndexExpression(obj::ArrayDouble *arrayExpr, std::shared_ptr<obj::Object> evaluatedIndex, ast::IndexExpression *indexExpr)
{
    if (!arrayExpr)
        return std::make_shared<obj::Error>("NULL array", obj::ErrorType::TypeError, indexExpr->token);

    if (arrayExpr->value.empty())
        return std::make_shared<obj::Error>("Attempting index in empty array", obj::ErrorType::IndexError, indexExpr->token);

    size_t arraySize = static_cast<int>(arrayExpr->value.size());
    switch (evaluatedIndex->type)
    {
    case obj::ObjectType::Integer:
    {
        auto intLiteral = static_cast<obj::Integer *>(evaluatedIndex.get());
        size_t finalIndex = normalizedArrayIndex(intLiteral->value, arraySize);
        if (finalIndex >= arraySize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intLiteral->value) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError, indexExpr->token);
        return std::make_shared<obj::Double>(arrayExpr->value[finalIndex]);
    }
    case obj::ObjectType::Range:
    {
        auto rangeLiteral = static_cast<obj::Range *>(evaluatedIndex.get());
        std::vector<double> ret;

        for (const auto &index : rangeLiteral->values())
        {
            size_t finalIndex = normalizedArrayIndex(index, arraySize);
            if (finalIndex >= arraySize)
                return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(index) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError, indexExpr->token);
            ret.push_back(arrayExpr->value[finalIndex]);
        }
        return std::make_shared<obj::ArrayDouble>(ret);
    }
    default:
        return std::make_shared<obj::Error>("Indexing in array must be done with Integer or Range but found " + toString(evaluatedIndex->type), obj::ErrorType::TypeError, indexExpr->token);
    };
}

std::shared_ptr<obj::Object> evalArrayComplexIndexExpression(obj::ArrayComplex *arrayExpr, std::shared_ptr<obj::Object> evaluatedIndex, ast::IndexExpression *indexExpr)
{
    if (!arrayExpr)
        return std::make_shared<obj::Error>("NULL array", obj::ErrorType::TypeError, indexExpr->token);

    if (arrayExpr->value.empty())
        return std::make_shared<obj::Error>("Attempting index in empty array", obj::ErrorType::IndexError, indexExpr->token);

    size_t arraySize = static_cast<int>(arrayExpr->value.size());
    switch (evaluatedIndex->type)
    {
    case obj::ObjectType::Integer:
    {
        auto intLiteral = static_cast<obj::Integer *>(evaluatedIndex.get());
        size_t finalIndex = normalizedArrayIndex(intLiteral->value, arraySize);
        if (finalIndex >= arraySize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intLiteral->value) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError, indexExpr->token);
        return std::make_shared<obj::Complex>(arrayExpr->value[finalIndex]);
    }
    case obj::ObjectType::Range:
    {
        auto rangeLiteral = static_cast<obj::Range *>(evaluatedIndex.get());
        std::vector<std::complex<double>> ret;

        for (const auto &index : rangeLiteral->values())
        {
            size_t finalIndex = normalizedArrayIndex(index, arraySize);
            if (finalIndex >= arraySize)
                return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(index) + " transformed to " + std::to_string(finalIndex) + ", array size=" + std::to_string(arraySize), obj::ErrorType::IndexError, indexExpr->token);
            ret.push_back(arrayExpr->value[finalIndex]);
        }
        return std::make_shared<obj::ArrayComplex>(ret);
    }
    default:
        return std::make_shared<obj::Error>("Indexing in array must be done with Integer or Range but found " + toString(evaluatedIndex->type), obj::ErrorType::TypeError, indexExpr->token);
    };
}

std::shared_ptr<obj::Object> evalStringIndexExpression(obj::String *stringLiteral, std::shared_ptr<obj::Object> evaluatedIndex, ast::IndexExpression *indexExpr)
{
    if (stringLiteral->value.empty())
        return std::make_shared<obj::Error>("Attempting index in empty string", obj::ErrorType::TypeError, indexExpr->token);

    size_t stringSize = static_cast<int>(stringLiteral->value.size());
    switch (evaluatedIndex->type)
    {
    case obj::ObjectType::Integer:
    {
        auto intLiteral = static_cast<obj::Integer *>(evaluatedIndex.get());
        size_t finalIndex = normalizedArrayIndex(intLiteral->value, stringSize);
        if (finalIndex >= stringSize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intLiteral->value) + " transformed to " + std::to_string(finalIndex) + ", string size=" + std::to_string(stringSize), obj::ErrorType::IndexError, indexExpr->token);
        return std::make_shared<obj::String>(std::string({stringLiteral->value[finalIndex]}));
    }
    case obj::ObjectType::Range:
    {
        auto rangeLiteral = static_cast<obj::Range *>(evaluatedIndex.get());
        std::string ret;

        for (const auto &index : rangeLiteral->values())
        {
            size_t finalIndex = normalizedArrayIndex(index, stringSize);
            if (finalIndex >= stringSize)
                return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(index) + " transformed to " + std::to_string(finalIndex) + ", string size=" + std::to_string(stringSize), obj::ErrorType::IndexError, indexExpr->token);
            ret += stringLiteral->value[finalIndex];
        }
        return std::make_shared<obj::String>(ret);
    }
    default:
        return std::make_shared<obj::Error>("Indexing in string must be done with Integer or Range but found " + toString(evaluatedIndex->type), obj::ErrorType::TypeError, indexExpr->token);
    };
}

std::shared_ptr<obj::Object> evalRangeIndexExpression(obj::Range *rangeLiteral, std::shared_ptr<obj::Object> evaluatedIndex, ast::IndexExpression *indexExpr)
{
    if (rangeLiteral->length() == 0)
        return std::make_shared<obj::Error>("Attempting index in empty range", obj::ErrorType::TypeError, indexExpr->token);

    size_t rangeSize = static_cast<int>(rangeLiteral->length());
    switch (evaluatedIndex->type)
    {
    case obj::ObjectType::Integer:
    {
        auto intLiteral = static_cast<obj::Integer *>(evaluatedIndex.get());
        size_t finalIndex = normalizedArrayIndex(intLiteral->value, rangeSize);
        if (finalIndex >= rangeSize)
            return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(intLiteral->value) + " transformed to " + std::to_string(finalIndex) + ", range size=" + std::to_string(rangeSize), obj::ErrorType::IndexError, indexExpr->token);
        return std::make_shared<obj::Integer>(rangeLiteral->values()[finalIndex]);
    }
    case obj::ObjectType::Range:
    {
        auto rangeIndexer = static_cast<obj::Range *>(evaluatedIndex.get());
        std::vector<std::shared_ptr<obj::Object>> ret;

        for (const auto &index : rangeIndexer->values())
        {
            size_t finalIndex = normalizedArrayIndex(index, rangeSize);
            if (finalIndex >= rangeSize)
                return std::make_shared<obj::Error>("Indexing error, index=" + std::to_string(index) + " transformed to " + std::to_string(finalIndex) + ", range size=" + std::to_string(rangeSize), obj::ErrorType::IndexError, indexExpr->token);
            ret.push_back(std::make_shared<obj::Integer>(rangeLiteral->values()[finalIndex]));
        }
        return std::make_shared<obj::Array>(ret);
    }
    default:
        return std::make_shared<obj::Error>("Indexing in range must be done with Integer or Range but found " + toString(evaluatedIndex->type), obj::ErrorType::IndexError, indexExpr->token);
    };
}

std::shared_ptr<obj::Object> evalDictionaryIndexExpression(obj::Dictionary *dictExpr, std::shared_ptr<obj::Object> evaluatedIndex, ast::IndexExpression *indexExpr)
{
    if (dictExpr->value.empty())
        return std::make_shared<obj::Error>("Attempting index in empty dictionary", obj::ErrorType::KeyError, indexExpr->token);

    auto foundIt = dictExpr->value.find(evaluatedIndex);
    if (foundIt == dictExpr->value.end())
        return std::make_shared<obj::Error>("Key " + evaluatedIndex->inspect() + " not found", obj::ErrorType::KeyError, indexExpr->token);
    return foundIt->second;
}

std::shared_ptr<obj::Object> evalIndexExpression(ast::IndexExpression *indexExpr, const std::shared_ptr<obj::Environment> &environment)
{
    std::shared_ptr<obj::Object> evaluatedIndex = std::move(evalExpression(indexExpr->index.get(), environment));
    if (evaluatedIndex->type == obj::ObjectType::Error)
        return evaluatedIndex;

    std::shared_ptr<obj::Object> evaluatedExpr = std::move(evalExpression(indexExpr->expression.get(), environment));

    switch (evaluatedExpr->type)
    {
    case obj::ObjectType::Error:
        return evaluatedExpr;
    case obj::ObjectType::Array:
        return evalArrayIndexExpression(static_cast<obj::Array *>(evaluatedExpr.get()), evaluatedIndex, indexExpr);
    case obj::ObjectType::ArrayDouble:
        return evalArrayDoubleIndexExpression(static_cast<obj::ArrayDouble *>(evaluatedExpr.get()), evaluatedIndex, indexExpr);
    case obj::ObjectType::ArrayComplex:
        return evalArrayComplexIndexExpression(static_cast<obj::ArrayComplex *>(evaluatedExpr.get()), evaluatedIndex, indexExpr);
    case obj::ObjectType::Dictionary:
        return evalDictionaryIndexExpression(static_cast<obj::Dictionary *>(evaluatedExpr.get()), evaluatedIndex, indexExpr);
    case obj::ObjectType::String:
        return evalStringIndexExpression(static_cast<obj::String *>(evaluatedExpr.get()), evaluatedIndex, indexExpr);
    case obj::ObjectType::Range:
        return evalRangeIndexExpression(static_cast<obj::Range *>(evaluatedExpr.get()), evaluatedIndex, indexExpr);
    default:
        return std::make_shared<obj::Error>("Was expecting array, dictionary or string but found " + toString(evaluatedExpr->type), obj::ErrorType::TypeError, indexExpr->token);
    };
}

std::shared_ptr<obj::Object> evalMemberExpression(ast::MemberExpression *memberExpression, const std::shared_ptr<obj::Environment> &environment)
{
    auto expr = evalExpression(memberExpression->expr.get(), environment);

    if (expr->type == obj::ObjectType::BoundUserTypeProperty)
    {
        obj::BoundUserTypeProperty *property = static_cast<obj::BoundUserTypeProperty *>(expr.get());
        expr = property->property->obj;
    }
    else if (expr->type == obj::ObjectType::BoundBuiltinTypeProperty)
    {
        obj::BoundBuiltinTypeProperty *property = static_cast<obj::BoundBuiltinTypeProperty *>(expr.get());
        expr = property->property->obj;
    }

    auto exprType = expr->type;
    auto builtinTypeIt = builtinTypes.find(exprType);
    if (builtinTypeIt != builtinTypes.end())
    {
        /* Functions have precedence over properties
         * when looking for a name.
         */
        auto memberFunctionIt = builtinTypeIt->second->functions.find(memberExpression->value.value);
        if (memberFunctionIt != builtinTypeIt->second->functions.end())
        {
            return std::make_shared<obj::BoundBuiltinTypeFunction>(expr, memberFunctionIt->second.function, memberFunctionIt->second.functionType);
        }
        auto propertyIt = builtinTypeIt->second->properties.find(memberExpression->value.value);
        if (propertyIt != builtinTypeIt->second->properties.end())
        {
            return std::make_shared<obj::BoundBuiltinTypeProperty>(expr, &propertyIt->second);
        }
    }
    else if (exprType == obj::ObjectType::UserObject)
    {
        auto userObject = static_cast<obj::UserObject *>(expr.get());
        auto typeObjectPtr = userObject->userType.get();
        auto memberFunctionIt = typeObjectPtr->functions.find(memberExpression->value.value);
        if (memberFunctionIt != typeObjectPtr->functions.end())
        {
            return std::make_shared<obj::BoundUserTypeFunction>(expr, memberFunctionIt->second);
        }
        auto propertyIt = userObject->properties.find(memberExpression->value.value);
        if (propertyIt != userObject->properties.end())
        {
            return std::make_shared<obj::BoundUserTypeProperty>(expr, &propertyIt->second);
        }
        return std::make_shared<obj::Error>("Cannot resolve object member " + memberExpression->value.value, obj::ErrorType::TypeError, memberExpression->token);
    }
    else if (exprType == obj::ObjectType::UserType)
    {
        auto userType = static_cast<obj::UserType *>(expr.get());
        auto typeObjectPtr = userType;
        auto memberFunctionIt = typeObjectPtr->functions.find(memberExpression->value.value);
        if (memberFunctionIt != typeObjectPtr->functions.end())
        {
            return std::make_shared<obj::BoundUserTypeFunction>(expr, memberFunctionIt->second);
        }
        auto propertyIt = typeObjectPtr->properties.find(memberExpression->value.value);
        if (propertyIt != typeObjectPtr->properties.end())
        {
            return std::make_shared<obj::BoundUserTypeProperty>(expr, &propertyIt->second);
        }
        return std::make_shared<obj::Error>("Cannot resolve type member " + memberExpression->value.value, obj::ErrorType::TypeError, memberExpression->token);
    }
    else if (exprType == obj::ObjectType::Module)
    {
        auto moduleObj = static_cast<obj::Module *>(expr.get());
        return moduleObj->environment->get(memberExpression->value.value);
    }
    return std::make_shared<obj::Error>("Cannot evaluate member expression of type " + obj::toString(exprType), obj::ErrorType::TypeError, memberExpression->token);
}

std::shared_ptr<obj::Object> evalModuleMemberExpression(ast::ModuleMemberExpression *moduleMemberExpression, const std::shared_ptr<obj::Environment> &environment)
{
    auto expr = evalExpression(moduleMemberExpression->expr.get(), environment);
    if (expr->type == obj::ObjectType::Error)
        return expr;

    auto exprType = expr->type;
    if (exprType == obj::ObjectType::Module)
    {
        auto moduleObj = static_cast<obj::Module *>(expr.get());
        return moduleObj->environment->get(moduleMemberExpression->value.value);
    }
    return std::make_shared<obj::Error>("Cannot evaluate module member expression of type " + obj::toString(exprType), obj::ErrorType::TypeError, moduleMemberExpression->token);
}

std::shared_ptr<obj::Object> evalBangOperator(obj::Object *object)
{
    if (!object)
        return std::make_shared<obj::Error>("Invalid type for ! NULL", obj::ErrorType::TypeError);

    switch (object->type)
    {
    case obj::ObjectType::Boolean:
        return std::make_shared<obj::Boolean>(!static_cast<obj::Boolean *>(object)->value);
    case obj::ObjectType::Null:
        return std::make_shared<obj::Boolean>(true);
    };

    return std::make_shared<obj::Boolean>(false);
}

std::shared_ptr<obj::Object> evalMinusPrefixOperator(obj::Object *object)
{
    if (!object)
        return std::make_shared<obj::Error>("Invalid type for - NULL", obj::ErrorType::TypeError);

    switch (object->type)
    {
    case obj::ObjectType::Integer:
        return std::make_shared<obj::Integer>(-static_cast<obj::Integer *>(object)->value);
    case obj::ObjectType::Double:
        return std::make_shared<obj::Double>(-static_cast<obj::Double *>(object)->value);
    };

    return std::make_shared<obj::Error>("Invalid type for - " + obj::toString(object->type), obj::ErrorType::TypeError);
}

std::shared_ptr<obj::Object> evalNullPrefixOperator(TokenType operator_t, const std::shared_ptr<obj::Object> &object)
{
    if (operator_t == TokenType::BANG)
        return std::make_shared<obj::Boolean>(true);

    return std::make_shared<obj::Error>("Invalid prefix operator " + toString(operator_t) + " for " + obj::toString(object->type), obj::ErrorType::TypeError);
}

std::shared_ptr<obj::Object> evalIntegerPrefixOperator(TokenType operator_t, const std::shared_ptr<obj::Object> &object)
{
    obj::Integer *intObj = static_cast<obj::Integer *>(object.get());
    switch (operator_t)
    {
    case TokenType::BANG:
        return std::make_shared<obj::Boolean>(obj::Boolean(intObj->value != 0));
    case TokenType::MINUS:
        return std::make_shared<obj::Integer>(obj::Integer(-intObj->value));
    }

    return std::make_shared<obj::Error>("Invalid prefix operator " + toString(operator_t) + " for " + obj::toString(object->type), obj::ErrorType::TypeError);
}

std::shared_ptr<obj::Object> evalDoublePrefixOperator(TokenType operator_t, const std::shared_ptr<obj::Object> &object)
{
    obj::Double *doubleObj = static_cast<obj::Double *>(object.get());
    if (operator_t == TokenType::MINUS)
        return std::make_shared<obj::Double>(-doubleObj->value);

    return std::make_shared<obj::Error>("Invalid prefix operator " + toString(operator_t) + " for " + obj::toString(object->type), obj::ErrorType::TypeError);
}

std::shared_ptr<obj::Object> evalBooleanPrefixOperator(TokenType operator_t, const std::shared_ptr<obj::Object> &object)
{
    obj::Boolean *booleanObj = static_cast<obj::Boolean *>(object.get());
    if (operator_t == TokenType::BANG)
        return std::make_shared<obj::Boolean>(!booleanObj->value);

    return std::make_shared<obj::Error>("Invalid prefix operator " + toString(operator_t) + " for " + obj::toString(object->type), obj::ErrorType::TypeError);
}

std::shared_ptr<obj::Object> evalPrefixExpression(TokenType operator_t, const std::shared_ptr<obj::Object> &object)
{
    switch (object->type)
    {
    case obj::ObjectType::Integer:
        return evalIntegerPrefixOperator(operator_t, object);
    case obj::ObjectType::Double:
        return evalDoublePrefixOperator(operator_t, object);
    case obj::ObjectType::Boolean:
        return evalBooleanPrefixOperator(operator_t, object);
    case obj::ObjectType::Null:
        return evalNullPrefixOperator(operator_t, object);
    case obj::ObjectType::Error:
        return object;
    };

    return std::make_shared<obj::Error>("unknown prefix operator " + toString(operator_t) + " for " + obj::toString(object->type), obj::ErrorType::TypeError);
}

obj::Object *evalIntegerInfixOperator(TokenType operator_t, obj::Integer *left, obj::Integer *right)
{
    switch (operator_t)
    {
    case TokenType::PLUS:
        return new obj::Integer(left->value + right->value);
    case TokenType::MINUS:
        return new obj::Integer(left->value - right->value);
    case TokenType::ASTERISK:
        return new obj::Integer(left->value * right->value);
    case TokenType::SLASH:
        if (right->value == 0)
            return new obj::Error("Division by 0", obj::ErrorType::ValueError);
        return new obj::Integer(left->value / right->value);
    case TokenType::PERCENT:
        return new obj::Integer(left->value % right->value);
    case TokenType::DOUBLEASTERISK:
        return new obj::Integer(pow_int(left->value, right->value));
    case TokenType::GT:
        return new obj::Boolean(left->value > right->value);
    case TokenType::GTEQ:
        return new obj::Boolean(left->value >= right->value);
    case TokenType::LT:
        return new obj::Boolean(left->value < right->value);
    case TokenType::LTEQ:
        return new obj::Boolean(left->value <= right->value);
    case TokenType::N_EQ:
        return new obj::Boolean(left->value != right->value);
    case TokenType::EQ:
        return new obj::Boolean(left->value == right->value);
    }

    return new obj::Error("unknown operator " + toString(operator_t) + " for Integer", obj::ErrorType::TypeError);
}

obj::Object *evalDoubleInfixOperator(TokenType operator_t, obj::Double *left, obj::Double *right)
{
    switch (operator_t)
    {
    case TokenType::PLUS:
        return new obj::Double(left->value + right->value);
    case TokenType::MINUS:
        return new obj::Double(left->value - right->value);
    case TokenType::ASTERISK:
        return new obj::Double(left->value * right->value);
    case TokenType::SLASH:
        return new obj::Double(left->value / right->value);
    case TokenType::GT:
        return new obj::Boolean(left->value > right->value);
    case TokenType::GTEQ:
        return new obj::Boolean(left->value >= right->value);
    case TokenType::LT:
        return new obj::Boolean(left->value < right->value);
    case TokenType::LTEQ:
        return new obj::Boolean(left->value <= right->value);
    case TokenType::N_EQ:
        return new obj::Boolean(left->value != right->value);
    case TokenType::EQ:
        return new obj::Boolean(left->value == right->value);
    }

    return new obj::Error("unknown operator " + toString(operator_t) + " for Double", obj::ErrorType::TypeError);
}

obj::Object *evalComplexInfixOperator(TokenType operator_t, obj::Complex *left, obj::Complex *right)
{
    switch (operator_t)
    {
    case TokenType::PLUS:
        return new obj::Complex(left->value + right->value);
    case TokenType::MINUS:
        return new obj::Complex(left->value - right->value);
    case TokenType::ASTERISK:
        return new obj::Complex(left->value * right->value);
    case TokenType::SLASH:
        return new obj::Complex(left->value / right->value);
    case TokenType::N_EQ:
        return new obj::Boolean(left->value != right->value);
    case TokenType::EQ:
        return new obj::Boolean(left->value == right->value);
    }

    return new obj::Error("unknown operator " + toString(operator_t) + " for Double", obj::ErrorType::TypeError);
}

obj::Object *evalStringInfixOperator(TokenType operator_t, obj::String *left, obj::String *right)
{
    switch (operator_t)
    {
    case TokenType::N_EQ:
        return new obj::Boolean(left->value != right->value);
    case TokenType::EQ:
        return new obj::Boolean(left->value == right->value);
    case TokenType::LT:
        return new obj::Boolean(left->value < right->value);
    case TokenType::GT:
        return new obj::Boolean(left->value > right->value);
    case TokenType::LTEQ:
        return new obj::Boolean(left->value <= right->value);
    case TokenType::GTEQ:
        return new obj::Boolean(left->value >= right->value);
    case TokenType::PLUS:
        return new obj::String(left->value + right->value);
    }

    return new obj::Error("unknown operator " + toString(operator_t) + " for String", obj::ErrorType::TypeError);
}

obj::Object *evalBoolInfixOperator(TokenType operator_t, obj::Boolean *left, obj::Boolean *right)
{
    switch (operator_t)
    {
    case TokenType::EQ:
        return new obj::Boolean(left->value == right->value);
    case TokenType::N_EQ:
        return new obj::Boolean(left->value != right->value);
    case TokenType::DOUBLEPIPE:
        return new obj::Boolean(left->value || right->value);
    case TokenType::DOUBLEAMPERSAND:
        return new obj::Boolean(left->value && right->value);
    }

    return new obj::Error("unknown operator " + toString(operator_t) + " for Boolean", obj::ErrorType::TypeError);
}

obj::Object *evalNullInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    bool isLeftNull = dynamic_cast<obj::Null *>(left);
    bool isRightNull = dynamic_cast<obj::Null *>(right);

    switch (operator_t)
    {
    case TokenType::EQ:
        return new obj::Boolean(isLeftNull && (isLeftNull == isRightNull));
    case TokenType::N_EQ:
        return new obj::Boolean((isLeftNull || isRightNull) && (isLeftNull != isRightNull));
    }

    return new obj::Error("Cannot use operator " + toString(operator_t) + " on NULL types", obj::ErrorType::TypeError);
}

bool dictEq(const obj::Dictionary *left, const obj::Dictionary *right)
{
    const size_t leftSize = left->value.size();
    const size_t rightSize = right->value.size();

    if (leftSize != rightSize)
        return false;

    for (const auto leftKv : left->value)
    {
        const auto leftKey = leftKv.first;
        const auto rightMapIt = right->value.find(leftKey);
        if (rightMapIt == right->value.end())
            return false;

        auto boolObj = std::unique_ptr<obj::Object>(evalInfixOperator(TokenType::EQ, leftKv.second.get(), rightMapIt->second.get()));
        if (boolObj->type == obj::ObjectType::Boolean)
        {
            if (!static_cast<obj::Boolean *>(boolObj.get())->value)
                return false;
        }
        else
            return false;
    }
    return true;
}

bool setEq(const obj::Set *left, const obj::Set *right)
{
    const size_t leftSize = left->value.size();
    const size_t rightSize = right->value.size();

    if (leftSize != rightSize)
        return false;

    for (const auto &leftElement : left->value)
    {
        auto rightMapIt = right->value.find(leftElement);
        if (rightMapIt == right->value.end())
            return false;
    }
    return true;
}

bool arrayEq(const obj::Array *left, const obj::Array *right)
{
    const size_t leftSize = left->value.size();
    const size_t rightSize = right->value.size();

    if (leftSize != rightSize)
        return false;

    for (size_t i = 0; i < leftSize; ++i)
    {
        if (left->value[i]->type != right->value[i]->type)
            return false;

        auto boolObj = std::unique_ptr<obj::Object>(evalInfixOperator(TokenType::EQ, left->value[i].get(), right->value[i].get()));
        if (boolObj->type == obj::ObjectType::Boolean)
        {
            if (!static_cast<obj::Boolean *>(boolObj.get())->value)
                return false;
        }
        else
            return false;
    }
    return true;
}

bool arrayEq(const obj::ArrayDouble *left, const obj::ArrayDouble *right)
{
    const size_t leftSize = left->value.size();
    const size_t rightSize = right->value.size();

    if (leftSize != rightSize)
        return false;

    for (size_t i = 0; i < leftSize; ++i)
    {
        if (left->value[i] != right->value[i])
            return false;
    }
    return true;
}

bool arrayEq(const obj::ArrayComplex *left, const obj::ArrayComplex *right)
{
    const size_t leftSize = left->value.size();
    const size_t rightSize = right->value.size();

    if (leftSize != rightSize)
        return false;

    for (size_t i = 0; i < leftSize; ++i)
    {
        if (left->value[i] != right->value[i])
            return false;
    }
    return true;
}

bool arrayLikeEq(const obj::Object *left, const obj::Object *right)
{
    if (right->type == obj::ObjectType::Null)
        return false;

    const size_t leftSize = arrayLikeLength(left);
    const size_t rightSize = arrayLikeLength(right);

    if (leftSize != rightSize)
        return false;

    for (size_t i = 0; i < leftSize; ++i)
    {
        auto leftVal = arrayLikeItem(left, i);
        auto rightVal = arrayLikeItem(right, i);
        if (leftVal->type != rightVal->type)
            return false;

        auto boolObj = std::unique_ptr<obj::Object>(evalInfixOperator(TokenType::EQ, leftVal.get(), rightVal.get()));
        if (boolObj->type == obj::ObjectType::Boolean)
        {
            if (!static_cast<obj::Boolean *>(boolObj.get())->value)
                return false;
        }
        else
            return false;
    }
    return true;
}

obj::Object *evalAnyArrayInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    if (operator_t == TokenType::EQ)
        return new obj::Boolean(arrayLikeEq(left, right));
    if (operator_t == TokenType::N_EQ)
        return new obj::Boolean(!arrayLikeEq(left, right));

    return new obj::Error("Cannot use operator " + toString(operator_t) + " on Array types", obj::ErrorType::TypeError);
}

obj::Object *evalArrayInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    auto leftArr = dynamic_cast<obj::Array *>(left);
    auto rightArr = dynamic_cast<obj::Array *>(right);

    if (operator_t == TokenType::EQ)
        return new obj::Boolean(arrayEq(leftArr, rightArr));
    if (operator_t == TokenType::N_EQ)
        return new obj::Boolean(!arrayEq(leftArr, rightArr));

    return new obj::Error("Cannot use operator " + toString(operator_t) + " on Array types", obj::ErrorType::TypeError);
}

obj::Object *evalArrayDoubleInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    auto leftArr = dynamic_cast<obj::ArrayDouble *>(left);
    auto rightArr = dynamic_cast<obj::ArrayDouble *>(right);

    if (operator_t == TokenType::EQ)
        return new obj::Boolean(arrayEq(leftArr, rightArr));
    if (operator_t == TokenType::N_EQ)
        return new obj::Boolean(!arrayEq(leftArr, rightArr));

    return new obj::Error("Cannot use operator " + toString(operator_t) + " on Array types", obj::ErrorType::TypeError);
}

obj::Object *evalArrayComplexInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    auto leftArr = dynamic_cast<obj::ArrayComplex *>(left);
    auto rightArr = dynamic_cast<obj::ArrayComplex *>(right);

    if (operator_t == TokenType::EQ)
        return new obj::Boolean(arrayEq(leftArr, rightArr));
    if (operator_t == TokenType::N_EQ)
        return new obj::Boolean(!arrayEq(leftArr, rightArr));

    return new obj::Error("Cannot use operator " + toString(operator_t) + " on Array types", obj::ErrorType::TypeError);
}

obj::Object *evalDictionaryInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    auto leftDict = dynamic_cast<obj::Dictionary *>(left);
    auto rightDict = dynamic_cast<obj::Dictionary *>(right);

    switch (operator_t)
    {
    case TokenType::EQ:
        return new obj::Boolean(dictEq(leftDict, rightDict));
    case TokenType::N_EQ:
        return new obj::Boolean(!dictEq(leftDict, rightDict));
    }

    return new obj::Error("Cannot use operator " + toString(operator_t) + " on Dictionary types", obj::ErrorType::TypeError);
}

obj::Object *evalSetInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    auto leftSet = dynamic_cast<obj::Set *>(left);
    auto rightSet = dynamic_cast<obj::Set *>(right);

    switch (operator_t)
    {
    case TokenType::EQ:
        return new obj::Boolean(setEq(leftSet, rightSet));
    case TokenType::N_EQ:
        return new obj::Boolean(!setEq(leftSet, rightSet));
    }

    return new obj::Error("Cannot use operator " + toString(operator_t) + " on Set types", obj::ErrorType::TypeError);
}

obj::Object *evalRangeInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    auto leftRange = dynamic_cast<obj::Range *>(left);
    auto rightRange = dynamic_cast<obj::Range *>(right);

    switch (operator_t)
    {
    case TokenType::EQ:
        return new obj::Boolean(leftRange->eq(rightRange));
    case TokenType::N_EQ:
        return new obj::Boolean(!leftRange->eq(rightRange));
    }

    return new obj::Error("Cannot use operator " + toString(operator_t) + " on Set types", obj::ErrorType::TypeError);
}

std::shared_ptr<obj::Object> evalAssignmentOperator(ast::Identifier *identifier, const std::shared_ptr<obj::Object> &right, const std::shared_ptr<obj::Environment> &environment)
{
    auto identifierObj = environment->get(identifier->value);
    if (identifierObj->type == obj::ObjectType::Error)
        return identifierObj;

    auto identifierType = environment->getType(identifier->value);

    if (!typing::isCompatibleType(identifierType, right.get(), identifierObj.get()))
    {
        return std::make_shared<obj::Error>("Incompatible type " + identifierType->text(), obj::ErrorType::TypeError);
    }

    if (isValueAssigned(right))
        return environment->set(identifier->value, right->clone());
    return environment->set(identifier->value, right);
}

bool evalOpInteger(obj::Integer *integer, TokenType operator_t, const std::shared_ptr<obj::Object> &right)
{
    if (right->type != obj::ObjectType::Integer)
        return false;

    auto rightValue = static_cast<obj::Integer *>(right.get());
    switch (operator_t)
    {
    case TokenType::PLUSASSIGN:
    {
        integer->value += rightValue->value;
        return true;
    }
    case TokenType::MINUSASSIGN:
    {
        integer->value -= rightValue->value;
        return true;
    }
    case TokenType::SLASHASSIGN:
    {
        integer->value /= rightValue->value;
        return true;
    }
    case TokenType::ASTERISKASSIGN:
    {
        integer->value *= rightValue->value;
        return true;
    }
    }
    return false;
}

bool evalOpDouble(obj::Double *doubleObj, TokenType operator_t, const std::shared_ptr<obj::Object> &right)
{
    if (right->type != obj::ObjectType::Double)
        return false;

    auto rightValue = static_cast<obj::Double *>(right.get());
    switch (operator_t)
    {
    case TokenType::PLUSASSIGN:
    {
        doubleObj->value += rightValue->value;
        return true;
    }
    case TokenType::MINUSASSIGN:
    {
        doubleObj->value -= rightValue->value;
        return true;
    }
    case TokenType::SLASHASSIGN:
    {
        doubleObj->value /= rightValue->value;
        return true;
    }
    case TokenType::ASTERISKASSIGN:
    {
        doubleObj->value *= rightValue->value;
        return true;
    }
    }
    return false;
}

bool evalOpAssignmentOperatorObject(obj::Object *object, TokenType operator_t, const std::shared_ptr<obj::Object> &right)
{
    bool succeeded = false;
    switch (object->type)
    {
    case obj::ObjectType::Integer:
        return evalOpInteger(static_cast<obj::Integer *>(object), operator_t, right);
    case obj::ObjectType::Double:
        return evalOpDouble(static_cast<obj::Double *>(object), operator_t, right);
    default:
        return false;
    }
    return false;
}

std::shared_ptr<obj::Object> evalOpAssignmentOperator(ast::Identifier *identifier, TokenType operator_t, const std::shared_ptr<obj::Object> &right, const std::shared_ptr<obj::Environment> &environment)
{
    auto identifierObj = environment->get(identifier->value);
    if (identifierObj->type == obj::ObjectType::Error)
        return identifierObj;

    bool succeeded = evalOpAssignmentOperatorObject(identifierObj.get(), operator_t, right);
    if (!succeeded)
        return std::make_shared<obj::Error>("Cannot use operator " + toString(operator_t) + " on type " + obj::toString(identifierObj->type), obj::ErrorType::TypeError);

    return identifierObj;
}

std::shared_ptr<obj::Object> evalIndexAssignmentExpression(ast::IndexExpression *indexExpr, ast::Expression *rightExpr, const std::shared_ptr<obj::Environment> &environment)
{
    std::vector<ast::Expression *> arguments;
    arguments.reserve(3);
    arguments.push_back(indexExpr->expression.get());
    arguments.push_back(indexExpr->index.get());
    arguments.push_back(rightExpr);
    return builtin::updateImpl(arguments, environment);
}

std::shared_ptr<obj::Object> evalMemberAssignmentExpression(ast::MemberExpression *memberExpr, ast::Expression *rightExpr, const std::shared_ptr<obj::Environment> &environment)
{
    std::shared_ptr<obj::Object> objPropToAssignInto = std::move(evalExpression(memberExpr, environment));
    if (objPropToAssignInto->type == obj::ObjectType::Error)
        return objPropToAssignInto;

    std::shared_ptr<obj::Object> rhv = std::move(evalExpression(rightExpr, environment));
    if (rhv->type == obj::ObjectType::Error)
        return rhv;

    obj::TPropertyObj *property = nullptr;
    if (objPropToAssignInto->type == obj::ObjectType::BoundBuiltinTypeProperty)
        property = static_cast<obj::BoundBuiltinTypeProperty *>(objPropToAssignInto.get())->property;
    else if (objPropToAssignInto->type == obj::ObjectType::BoundUserTypeProperty)
        property = static_cast<obj::BoundUserTypeProperty *>(objPropToAssignInto.get())->property;

    if (property)
    {
        if (property->constant)
            return std::make_shared<obj::Error>("Cannot update const member " + memberExpr->value.text(), obj::ErrorType::TypeError, memberExpr->token);

        if (!typing::isCompatibleType(property->type, rhv.get(), property->obj.get()))
            return std::make_shared<obj::Error>("Incompatible type " + property->type->text() + " for " + rhv->inspect(), obj::ErrorType::TypeError);

        if (isValueAssigned(rhv))
            property->obj = rhv->clone();
        else
            property->obj = rhv;
        return objPropToAssignInto;
    }
    return std::make_shared<obj::Error>("Cannot update member", obj::ErrorType::TypeError, memberExpr->token);
}

std::shared_ptr<obj::Object> evalIndexOpAssignmentExpression(ast::IndexExpression *indexExpr, TokenType operator_t, ast::Expression *rightExpr, const std::shared_ptr<obj::Environment> &environment)
{
    /* Before passing in the rightExpr make sure we are passing in an obj with the proper value.
     */
    std::shared_ptr<obj::Object> objToAssignInto = std::move(evalIndexExpression(indexExpr, environment));
    std::shared_ptr<obj::Object> rhv = std::move(evalExpression(rightExpr, environment));
    bool succeeded = evalOpAssignmentOperatorObject(objToAssignInto.get(), operator_t, rhv);
    if (!succeeded)
        return std::make_shared<obj::Error>("Cannot use operator " + toString(operator_t) + " on type" + obj::toString(objToAssignInto->type), obj::ErrorType::TypeError);
    return objToAssignInto;
}

obj::Object *evalInfixOperator(TokenType operator_t, obj::Object *left, obj::Object *right)
{
    if (!left)
        return new obj::Error(toString(operator_t) + " has no left-hand object", obj::ErrorType::TypeError);
    if (!right)
        return new obj::Error(toString(operator_t) + " has no right-hand object", obj::ErrorType::TypeError);

    switch (left->type)
    {
    case obj::ObjectType::Integer:
    {
        if (right->type == obj::ObjectType::Integer)
            return evalIntegerInfixOperator(operator_t, static_cast<obj::Integer *>(left), static_cast<obj::Integer *>(right));
        break;
    }
    case obj::ObjectType::Boolean:
    {
        if (right->type == obj::ObjectType::Boolean)
            return evalBoolInfixOperator(operator_t, static_cast<obj::Boolean *>(left), static_cast<obj::Boolean *>(right));
        break;
    }
    case obj::ObjectType::Double:
    {
        if (right->type == obj::ObjectType::Double)
            return evalDoubleInfixOperator(operator_t, static_cast<obj::Double *>(left), static_cast<obj::Double *>(right));
        break;
    }
    case obj::ObjectType::Complex:
    {
        if (right->type == obj::ObjectType::Complex)
            return evalComplexInfixOperator(operator_t, static_cast<obj::Complex *>(left), static_cast<obj::Complex *>(right));
        break;
    }
    case obj::ObjectType::String:
    {
        if (right->type == obj::ObjectType::String)
            return evalStringInfixOperator(operator_t, static_cast<obj::String *>(left), static_cast<obj::String *>(right));
        break;
    }
    case obj::ObjectType::Null:
    {
        if (right->type == obj::ObjectType::Null)
            return evalNullInfixOperator(operator_t, left, right);
        break;
    }
    case obj::ObjectType::Array:
    {
        if (right->type == obj::ObjectType::Array)
            return evalArrayInfixOperator(operator_t, left, right);
        else
            return evalAnyArrayInfixOperator(operator_t, left, right);
        break;
    }
    case obj::ObjectType::ArrayDouble:
    {
        if (right->type == obj::ObjectType::ArrayDouble)
            return evalArrayDoubleInfixOperator(operator_t, left, right);
        else
            return evalAnyArrayInfixOperator(operator_t, left, right);
        break;
    }
    case obj::ObjectType::ArrayComplex:
    {
        if (right->type == obj::ObjectType::ArrayComplex)
            return evalArrayDoubleInfixOperator(operator_t, left, right);
        else
            return evalAnyArrayInfixOperator(operator_t, left, right);
        break;
    }
    case obj::ObjectType::Dictionary:
    {
        if (right->type == obj::ObjectType::Dictionary)
            return evalDictionaryInfixOperator(operator_t, left, right);
        else
            return evalAnyArrayInfixOperator(operator_t, left, right);
        break;
    }
    case obj::ObjectType::Set:
    {
        if (right->type == obj::ObjectType::Set)
            return evalSetInfixOperator(operator_t, left, right);
        break;
    }
    case obj::ObjectType::Range:
    {
        if (right->type == obj::ObjectType::Range)
            return evalRangeInfixOperator(operator_t, left, right);
        break;
    }
    };

    return new obj::Error("Type mismatch for operator " + toString(operator_t) + " for types " + obj::toString(left->type) + " and " + obj::toString(right->type), obj::ErrorType::TypeError);
}

bool isTruthy(const std::shared_ptr<obj::Object> &value)
{
    switch (value->type)
    {
    case obj::ObjectType::Null:
        return false;
    case obj::ObjectType::Boolean:
        return static_cast<obj::Boolean *>(value.get())->value;
    case obj::ObjectType::Integer:
        return static_cast<obj::Integer *>(value.get())->value != 0;
    };
    return true;
}

std::shared_ptr<obj::Object> evalIfExpression(const ast::IfExpression *ifExpr, const std::shared_ptr<obj::Environment> &environment)
{
    auto condition = evalExpression(ifExpr->condition.get(), environment);
    if (condition->type == obj::ObjectType::Error)
        return condition;

    ast::Statement *chosenStatement = nullptr;
    if (isTruthy(condition))
    {
        chosenStatement = ifExpr->consequence.get();
    }
    else if (ifExpr->alternative.get())
    {
        chosenStatement = ifExpr->alternative.get();
    }

    if (chosenStatement)
    {
        auto newScopedEnvironment = makeNewEnvironment(environment);
        auto retValue = addTokenInCaseOfError(evalStatement(chosenStatement, newScopedEnvironment), ifExpr->token);
        auto desRetValue = evalUserObjectDestructors(newScopedEnvironment);
        if (desRetValue->type == obj::ObjectType::Error || desRetValue->type == obj::ObjectType::Exit)
            return desRetValue;
        return retValue;
    }

    return NullObject;
}

std::shared_ptr<obj::Object> evalWhileExpression(const ast::WhileExpression *whileExpr, const std::shared_ptr<obj::Environment> &environment)
{
    if (!whileExpr)
        throw std::runtime_error("WhileExpr* is NULL");

    auto condition = evalExpression(whileExpr->condition.get(), environment);
    auto errorValue = dynamic_cast<obj::Error *>(condition.get());
    if (errorValue)
        return addTokenInCaseOfError(condition, whileExpr->condition->token);

    while (isTruthy(condition))
    {
        auto newEnvironment = makeNewEnvironment(environment);
        auto retValue = evalStatement(whileExpr->statement.get(), newEnvironment);
        auto desRetValue = evalUserObjectDestructors(newEnvironment);
        if (desRetValue->type == obj::ObjectType::Error || desRetValue->type == obj::ObjectType::Exit)
            return desRetValue;

        if (retValue->type == obj::ObjectType::Error)
            return addTokenInCaseOfError(retValue, whileExpr->statement->token);

        if (retValue->type == obj::ObjectType::BreakValue)
            return NullObject;

        if (retValue->type == obj::ObjectType::ReturnValue)
            return retValue;

        if (retValue->type == obj::ObjectType::Exit)
            return retValue;

        condition = std::move(evalExpression(whileExpr->condition.get(), environment));
        if (condition->type == obj::ObjectType::Error)
            return addTokenInCaseOfError(condition, whileExpr->condition->token);
    }
    return NullObject;
}

std::shared_ptr<obj::Object> evalForExpression(const ast::ForExpression *forExpr, const std::shared_ptr<obj::Environment> &environment)
{
    if (!forExpr)
        throw std::runtime_error("ForExpr* is NULL");

    auto iteratable = evalExpression(forExpr->iterable.get(), environment);
    auto iterator = builtin::iter_impl(iteratable);
    std::shared_ptr<obj::Iterator> iter = std::dynamic_pointer_cast<obj::Iterator>(iterator);

    if (!iter)
        return std::make_shared<obj::Error>("Cannot iterate over " + forExpr->iterable->text(), obj::ErrorType::TypeError);

    while (iter->isValid())
    {
        auto newEnvironment = makeNewEnvironment(environment);
        std::shared_ptr<obj::Object> iteratorValue = iter->next();
        if (iteratorValue->type == obj::ObjectType::Error)
            return addTokenInCaseOfError(iteratorValue, forExpr->statement->token);

        if (!typing::isCompatibleType(forExpr->iterType.get(), iteratorValue.get(), nullptr))
        {
            std::string expectedTypeStr = forExpr->iterType->text();
            std::string gottenTypeStr = typing::computeType(iteratorValue.get())->text();
            return std::make_shared<obj::Error>("Incompatible type for loop variable " + forExpr->name.value + ", expected " + expectedTypeStr + " but got " + gottenTypeStr, obj::ErrorType::TypeError, forExpr->token);
        }

        newEnvironment->add(forExpr->name.value, iteratorValue, forExpr->constant, forExpr->iterType.get());
        auto retValue = evalStatement(forExpr->statement.get(), newEnvironment);
        auto desRetValue = evalUserObjectDestructors(newEnvironment);
        if (desRetValue->type == obj::ObjectType::Error || desRetValue->type == obj::ObjectType::Exit)
            return desRetValue;

        if (retValue->type == obj::ObjectType::Error)
            return addTokenInCaseOfError(retValue, forExpr->statement->token);

        if (retValue->type == obj::ObjectType::BreakValue)
            return NullObject;

        if (retValue->type == obj::ObjectType::ReturnValue)
            return retValue;

        if (retValue->type == obj::ObjectType::Exit)
            return retValue;
    }
    return NullObject;
}

std::shared_ptr<obj::Object>
evalFunction(ast::Expression *functionExpression, const std::shared_ptr<obj::Environment> &environment)
{
    // built-ins take precedence
    std::string functionName;
    if (functionExpression->type == ast::NodeType::Identifier)
        functionName = static_cast<ast::Identifier *>(functionExpression)->value;
    else if (functionExpression->type == ast::NodeType::MemberExpression)
    {
        auto evalExpr = evalMemberExpression(static_cast<ast::MemberExpression *>(functionExpression), environment);
        auto exprType = evalExpr->type;
        return evalExpr;
    }
    else if (functionExpression->type == ast::NodeType::ModuleMemberExpression)
    {
        auto evalExpr = evalModuleMemberExpression(static_cast<ast::ModuleMemberExpression *>(functionExpression), environment);
        auto exprType = evalExpr->type;
        return evalExpr;
    }
    else
        functionName = functionExpression->text();

    auto builtInFn = builtins.find(functionName);
    if (builtInFn != builtins.end())
        return builtInFn->second;

    return environment->get(functionName);
}

std::shared_ptr<obj::Object> evalBuiltin(obj::Builtin *builtin, std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
{
    return builtin->function(arguments, environment);
}

std::shared_ptr<obj::Object> unwrap(const std::shared_ptr<obj::Object> &object)
{
    if (object->type == obj::ObjectType::ReturnValue)
        return unwrap(static_cast<obj::ReturnValue *>(object.get())->value);
    else if (object->type == obj::ObjectType::BoundBuiltinTypeProperty)
        return unwrap(static_cast<obj::BoundBuiltinTypeProperty *>(object.get())->property->obj);
    else if (object->type == obj::ObjectType::BoundUserTypeProperty)
        return unwrap(static_cast<obj::BoundUserTypeProperty *>(object.get())->property->obj);
    return object;
}

std::shared_ptr<obj::Object> unwrapReturnValue(const std::shared_ptr<obj::Object> &object)
{
    if (object->type == obj::ObjectType::ReturnValue)
        return static_cast<obj::ReturnValue *>(object.get())->value;
    return object;
}

std::shared_ptr<obj::Object> unwrapMemberValue(const std::shared_ptr<obj::Object> &object)
{
    if (object->type == obj::ObjectType::BoundBuiltinTypeProperty)
        return static_cast<obj::BoundBuiltinTypeProperty *>(object.get())->property->obj;

    if (object->type == obj::ObjectType::BoundUserTypeProperty)
        return static_cast<obj::BoundUserTypeProperty *>(object.get())->property->obj;

    return object;
}

std::shared_ptr<obj::Object> evalDestructor(obj::Function *functionObj, obj::UserObject *self, const std::shared_ptr<obj::Environment> &environment)
{
    auto functionEnvironment = makeNewEnvironment(environment);

    std::shared_ptr<obj::UserObject> ghostObject = std::make_shared<obj::UserObject>();
    ghostObject->declaredType = self->declaredType;
    ghostObject->type = self->type;
    ghostObject->properties = self->properties;
    functionEnvironment->add("this", ghostObject, false, nullptr);

    auto returnValue = unwrapMemberValue(unwrapReturnValue(evalStatement(functionObj->body, std::move(functionEnvironment))));
    if (!typing::isCompatibleType(functionObj->returnType, returnValue.get(), nullptr))
    {
        std::string expectedTypeStr = functionObj->returnType->text();
        std::string gottenTypeStr = typing::computeType(returnValue.get())->text();
        return std::make_shared<obj::Error>("Incompatible return type in destructor, expected " + expectedTypeStr + " but got " + gottenTypeStr, obj::ErrorType::TypeError);
    }

    ghostObject.reset();
    self->properties.clear();

    return returnValue;
}

std::shared_ptr<obj::Object> evalBoundUserTypeFunction(obj::BoundUserTypeFunction *boundUserTypeFunc, ast::CallExpression *callExpr, const std::shared_ptr<obj::Environment> &environment)
{
    const auto &userObj(boundUserTypeFunc->boundTo);
    const auto &functionObj(boundUserTypeFunc->function);

    auto functionEnvironment = makeNewEnvironment(environment);
    if (userObj->type == obj::ObjectType::UserObject)
        functionEnvironment->add("this", userObj, false, nullptr);
    else if (userObj->type == obj::ObjectType::UserType)
        functionEnvironment->add("this_type", userObj, false, nullptr);

    std::vector<std::shared_ptr<obj::Object>> evaluatedArgs;
    size_t argumentIndex = 0;
    if (callExpr)
    {
        for (const auto &expr : callExpr->arguments)
        {
            evaluatedArgs.push_back(evalExpression(expr.get(), environment));
            if (evaluatedArgs.back()->type == obj::ObjectType::Error)
                return evaluatedArgs.back();

            if (!typing::isCompatibleType(functionObj->argumentTypes[argumentIndex], evaluatedArgs.back().get(), nullptr))
            {
                std::string expectedTypeStr = functionObj->argumentTypes[argumentIndex]->text();
                std::string gottenTypeStr = typing::computeType(evaluatedArgs.back().get())->text();
                return std::make_shared<obj::Error>("Incompatible type for argument " + std::to_string(argumentIndex + 1) + ", expected " + expectedTypeStr + " but got " + gottenTypeStr, obj::ErrorType::TypeError, callExpr->token);
            }

            if (argumentIndex >= functionObj->arguments.size())
                return std::make_shared<obj::Error>("Too many arguments provided for function", obj::ErrorType::TypeError, callExpr->token);

            functionEnvironment->add(functionObj->arguments[argumentIndex].value, evaluatedArgs.back(), false, functionObj->argumentTypes[argumentIndex]);
            ++argumentIndex;
        }
    }

    auto returnValue = unwrapMemberValue(unwrapReturnValue(evalStatement(functionObj->body, std::move(functionEnvironment))));
    if (returnValue->type == obj::ObjectType::Error)
        return returnValue;

    if (!typing::isCompatibleType(functionObj->returnType, returnValue.get(), nullptr))
    {
        std::string expectedTypeStr = functionObj->returnType->text();
        std::string gottenTypeStr = typing::computeType(returnValue.get())->text();
        return std::make_shared<obj::Error>("Incompatible return type, expected " + expectedTypeStr + " but got " + gottenTypeStr, obj::ErrorType::TypeError, callExpr->token);
    }

    auto desRetValue = evalUserObjectDestructors(functionEnvironment);
    if (desRetValue->type == obj::ObjectType::Error || desRetValue->type == obj::ObjectType::Exit)
        return desRetValue;

    return returnValue;
}

std::shared_ptr<obj::Object> evalFunctionWithArguments(obj::Function *functionObj, const std::vector<std::shared_ptr<obj::Object>> &evaluatedArgs, const std::shared_ptr<obj::Environment> &environment)
{
    auto functionEnvironment = makeNewEnvironment(functionObj->environment);
    size_t argumentIndex = 0;
    for (const auto &evaluatedArg : evaluatedArgs)
    {
        if (evaluatedArg->type == obj::ObjectType::Error)
            return evaluatedArg;

        if (argumentIndex >= functionObj->arguments.size())
            return std::make_shared<obj::Error>("Too many arguments provided for function", obj::ErrorType::TypeError);

        if (argumentIndex >= functionObj->argumentTypes.size())
            return std::make_shared<obj::Error>("Too many arguments provided for function", obj::ErrorType::TypeError);

        if (!typing::isCompatibleType(functionObj->argumentTypes[argumentIndex], evaluatedArg.get(), nullptr))
        {
            std::string expectedTypeStr = functionObj->argumentTypes[argumentIndex]->text();
            std::string gottenTypeStr = typing::computeType(evaluatedArg.get())->text();
            return std::make_shared<obj::Error>("Incompatible type for argument " + std::to_string(argumentIndex + 1) + ", expected " + expectedTypeStr + " but got " + gottenTypeStr, obj::ErrorType::TypeError);
        }

        functionEnvironment->add(functionObj->arguments[argumentIndex].value, evaluatedArg, false, functionObj->argumentTypes[argumentIndex]);
        ++argumentIndex;
    }

    //
    // check here the return type of the return value!
    //
    auto retValue = unwrap(evalStatement(functionObj->body, std::move(functionEnvironment)));
    auto desRetValue = evalUserObjectDestructors(functionEnvironment);
    if (desRetValue->type == obj::ObjectType::Error || desRetValue->type == obj::ObjectType::Exit)
        return desRetValue;
    return retValue;
}

std::shared_ptr<obj::Object> evalCallExpression(ast::CallExpression *callExpr, const std::shared_ptr<obj::Environment> &environment)
{
    std::shared_ptr<obj::Object> function = evalFunction(callExpr->function.get(), environment);
    if (function->type == obj::ObjectType::Builtin)
    {
        auto builtinFunctionObj = static_cast<obj::Builtin *>(function.get());
        return evalBuiltin(builtinFunctionObj, &callExpr->arguments, environment);
    }
    else if (function->type == obj::ObjectType::Function)
    {
        auto functionObj = static_cast<obj::Function *>(function.get());
        auto functionEnvironment = makeNewEnvironment(functionObj->environment);
        std::vector<std::shared_ptr<obj::Object>> evaluatedArgs;
        size_t argumentIndex = 0;
        for (const auto &expr : callExpr->arguments)
        {
            evaluatedArgs.push_back(evalExpression(expr.get(), environment));
            if (evaluatedArgs.back()->type == obj::ObjectType::Error)
                return evaluatedArgs.back();

            if (argumentIndex >= functionObj->arguments.size())
                return std::make_shared<obj::Error>("Too many arguments provided for function", obj::ErrorType::TypeError, callExpr->token);

            if (argumentIndex >= functionObj->argumentTypes.size())
                return std::make_shared<obj::Error>("Too many arguments provided for function", obj::ErrorType::TypeError, callExpr->token);

            if (!typing::isCompatibleType(functionObj->argumentTypes[argumentIndex], evaluatedArgs.back().get(), nullptr))
            {
                std::string expectedTypeStr = functionObj->argumentTypes[argumentIndex]->text();
                std::string gottenTypeStr = "<invalid>";
                auto computedType = typing::computeType(evaluatedArgs.back().get());
                if (computedType)
                    gottenTypeStr = computedType->text();

                return std::make_shared<obj::Error>("Incompatible type for argument " + std::to_string(argumentIndex + 1) + ", expected " + expectedTypeStr + " but got " + gottenTypeStr, obj::ErrorType::TypeError, callExpr->token);
            }

            functionEnvironment->add(functionObj->arguments[argumentIndex].value, evaluatedArgs.back(), false, functionObj->argumentTypes[argumentIndex]);
            ++argumentIndex;
        }

        //
        // check here the return type of the return value!
        //
        auto retValue = unwrapReturnValue(evalStatement(functionObj->body, std::move(functionEnvironment)));
        auto desRetValue = evalUserObjectDestructors(functionEnvironment);
        if (desRetValue->type == obj::ObjectType::Error || desRetValue->type == obj::ObjectType::Exit)
            return desRetValue;
        return retValue;
    }
    else if (function->type == obj::ObjectType::BoundBuiltinTypeFunction)
    {
        auto functionObj = static_cast<obj::BoundBuiltinTypeFunction *>(function.get());
        std::vector<std::shared_ptr<obj::Object>> evaluatedArgs;
        size_t argumentIndex = 0;
        for (const auto &expr : callExpr->arguments)
        {
            evaluatedArgs.push_back(unwrapMemberValue(evalExpression(expr.get(), environment)));
            if (evaluatedArgs.back()->type == obj::ObjectType::Error)
                return evaluatedArgs.back();

            // if (!typing::isCompatibleType(functionObj->argumentTypes[argumentIndex], evaluatedArgs.back().get(),nullptr))
            // {
            //     std::string expectedTypeStr = functionObj->argumentTypes[argumentIndex]->text();
            //     std::string gottenTypeStr = typing::computeType(evaluatedArgs.back().get())->text();
            //     return std::make_shared<obj::Error>(obj::Error("Incompatible type for argument " + std::to_string(argumentIndex+1) + ", expected " + expectedTypeStr + " but got " + gottenTypeStr , callExpr->token));
            // }
            // if (argumentIndex >= functionObj->arguments.size())
            //     return std::make_shared<obj::Error>(obj::Error("Too many arguments provided for function", callExpr->token));
        }

        return functionObj->function(functionObj->boundTo, evaluatedArgs);
    }
    else if (function->type == obj::ObjectType::UserType)
    {
        /* the name of the type as function is a short-hand for the constructor, check if there exist
         *  a constructor
         */
        auto typeObj = static_cast<obj::UserType *>(function.get());
        auto userObj = std::make_shared<obj::UserObject>();
        userObj->userType = std::dynamic_pointer_cast<obj::UserType>(function);

        for (const auto &[k, v] : userObj->userType->properties)
            userObj->properties[k] = obj::TPropertyObj({v.obj->clone(), v.constant, v.type});

        auto createFunc = typeObj->functions.find("construct");
        if (createFunc != typeObj->functions.end())
        {
            // invoke the create function, giving it the this as context along
            auto boundCreateFunction = obj::BoundUserTypeFunction(userObj, createFunc->second);
            auto sunkenValue = evalBoundUserTypeFunction(&boundCreateFunction, callExpr, environment);
            if (sunkenValue->type == obj::ObjectType::Error)
                return sunkenValue;
            //
        }

        // add the destruct after construct so it is harder to invoke it when something fundamentally went wrong
        // and any user level code in destruct would observe a "broken" object
        auto destroyFunc = typeObj->functions.find("destruct");
        if (destroyFunc != typeObj->functions.end())
        {
            userObj->destructor = destroyFunc->second;
        }

        return userObj;
    }
    else if (function->type == obj::ObjectType::BoundUserTypeFunction)
    {
        auto boundUserTypeFunc = static_cast<obj::BoundUserTypeFunction *>(function.get());
        return evalBoundUserTypeFunction(boundUserTypeFunc, callExpr, environment);
    }
    else
    {
        return std::make_shared<obj::Error>("Function " + callExpr->text() + " not found", obj::ErrorType::TypeError, callExpr->token);
    }
}

std::shared_ptr<obj::Object> evalFunctionLiteral(ast::FunctionLiteral *funcLiteral, const std::shared_ptr<obj::Environment> &environment)
{
    auto function = std::make_shared<obj::Function>();
    function->arguments = funcLiteral->arguments;
    for (const auto &argType : funcLiteral->argumentTypes)
        function->argumentTypes.push_back(argType.get());

    function->doc = funcLiteral->doc;
    function->returnType = funcLiteral->returnType.get();
    function->body = funcLiteral->body.get();
    function->environment = environment;
    return function;
}

std::shared_ptr<obj::Object> evalTypeLiteral(ast::TypeLiteral *typeLiteral, const std::shared_ptr<obj::Environment> &environment)
{
    auto type = std::make_shared<obj::UserType>();
    type->name = typeLiteral->name;
    type->doc = typeLiteral->doc;
    type->functions.clear();
    type->properties.clear();
    for (const auto &typeDefinition : typeLiteral->definitions)
    {
        std::string propertyOrFuncName = typeDefinition->name.value;
        if (typeDefinition->value->type == ast::NodeType::FunctionLiteral)
        {
            // [TODO] capture type
            std::shared_ptr<obj::Function> functionDefinition = std::dynamic_pointer_cast<obj::Function>(evalFunctionLiteral(static_cast<ast::FunctionLiteral *>(typeDefinition->value.get()), nullptr));
            type->functions.insert_or_assign(propertyOrFuncName, functionDefinition);
        }
        else
        {
            auto obj = evalExpression(typeDefinition->value.get(), nullptr);
            type->properties.insert_or_assign(propertyOrFuncName, obj::TPropertyObj({obj, typeDefinition->constant, typeDefinition->exprType.get()}));
        }
    }
    environment->add(type->name, type, false, nullptr);
    return type;
}

std::shared_ptr<obj::Object> evalIdentifier(ast::Identifier *identifier, const std::shared_ptr<obj::Environment> &environment)
{
    switch (identifier->markedAsBuiltin)
    {
    case ast::MarkedAsBuiltin::Unknown:
    {
        auto builtInFn = builtins.find(identifier->value);
        if (builtInFn != builtins.end())
        {
            identifier->markedAsBuiltin = ast::MarkedAsBuiltin::True;
            return builtInFn->second;
        }
        identifier->markedAsBuiltin = ast::MarkedAsBuiltin::False;
        return addTokenInCaseOfError(environment->get(identifier->value), identifier->token);
    }
    case ast::MarkedAsBuiltin::True:
        return builtins.at(identifier->value);

    case ast::MarkedAsBuiltin::False:
        return addTokenInCaseOfError(environment->get(identifier->value), identifier->token);
    }
    return std::make_shared<obj::Error>("Cannot evaluate identifier", obj::ErrorType::TypeError, identifier->token);
}

std::vector<std::shared_ptr<obj::Object>> objectsFromArrayLiteral(ast::Expression *expression, const std::shared_ptr<obj::Environment> &environment)
{
    std::vector<std::shared_ptr<obj::Object>> objects;
    switch (expression->type)
    {
    case ast::NodeType::ArrayLiteral:
    {
        auto arrayExpr = static_cast<ast::ArrayLiteral *>(expression);
        for (auto &element : arrayExpr->elements)
            objects.push_back(evalExpression(element.get(), environment));
        break;
    }
    case ast::NodeType::ArrayDoubleLiteral:
    {
        auto arrayExpr = static_cast<ast::ArrayDoubleLiteral *>(expression);
        for (auto &element : arrayExpr->elements)
            objects.push_back(std::make_shared<obj::Double>(element));
        break;
    }
    case ast::NodeType::ArrayComplexLiteral:
    {
        auto arrayExpr = static_cast<ast::ArrayComplexLiteral *>(expression);
        for (auto &element : arrayExpr->elements)
            objects.push_back(std::make_shared<obj::Complex>(element));
        break;
    }
    };
    return objects;
}

std::shared_ptr<obj::Object> evalExpression(ast::Expression *expression, const std::shared_ptr<obj::Environment> &environment, ast::TypeExpression *typeHint)
{
    switch (expression->type)
    {
    case ast::NodeType::BooleanLiteral:
        return std::make_shared<obj::Boolean>(static_cast<ast::BooleanLiteral *>(expression)->value);
    case ast::NodeType::IntegerLiteral:
        return std::make_shared<obj::Integer>(static_cast<ast::IntegerLiteral *>(expression)->value);
    case ast::NodeType::RangeLiteral:
        return std::make_shared<obj::Range>(static_cast<ast::RangeLiteral *>(expression)->lower, static_cast<ast::RangeLiteral *>(expression)->upper, static_cast<ast::RangeLiteral *>(expression)->stride);
    case ast::NodeType::DoubleLiteral:
        return std::make_shared<obj::Double>(static_cast<ast::DoubleLiteral *>(expression)->value);
    case ast::NodeType::StringLiteral:
        return std::make_shared<obj::String>(static_cast<ast::StringLiteral *>(expression)->value);
    case ast::NodeType::NullLiteral:
        return NullObject;
    case ast::NodeType::ArrayLiteral:
    case ast::NodeType::ArrayDoubleLiteral:
    case ast::NodeType::ArrayComplexLiteral:
    {
        if (typeHint == nullptr)
        {
            return std::make_shared<obj::Array>(objectsFromArrayLiteral(expression, environment));
        }
        else
        {
            if (typeHint->type == ast::NodeType::TypeArray)
            {
                auto typeArray = static_cast<ast::TypeArray *>(typeHint);
                if (typeArray->elementType->type == ast::NodeType::TypeIdentifier)
                {
                    auto typeIdentifier = static_cast<ast::TypeIdentifier *>(typeArray->elementType.get());
                    if (typeIdentifier->value == "double")
                    {
                        // a type declaration of [double], we can accept a ArrayDouble
                        // as the parser will have any other type be a generic Array or ArrayComplex, but those
                        // could still be castable, for examples [1.0*1.0] is not an ArrayDoubleLiteral but would
                        // still be acceptable
                        if (expression->type == ast::NodeType::ArrayDoubleLiteral)
                        {
                            auto arrayExpr = static_cast<ast::ArrayDoubleLiteral *>(expression);
                            return std::make_shared<obj::ArrayDouble>(arrayExpr->elements);
                        }
                        else
                        {
                            // evaluate all elements, if they all evaluate to a double we are good
                            std::vector<std::shared_ptr<obj::Object>> objects = objectsFromArrayLiteral(expression, environment);
                            std::vector<double> doubleValues;
                            for (const auto &object : objects)
                            {
                                if (object->type != obj::ObjectType::Double)
                                    return std::make_shared<obj::Error>("Trying to build an array of wrong type", obj::ErrorType::TypeError);
                                doubleValues.push_back(static_cast<obj::Double *>(object.get())->value);
                            }
                            return std::make_shared<obj::ArrayDouble>(doubleValues);
                        }
                        return std::make_shared<obj::Error>("Trying to build an array of wrong type", obj::ErrorType::TypeError);
                    }
                    else if (typeIdentifier->value == "complex")
                    {
                        // a type declaration of [double], we can accept a ArrayComplex
                        // as the parser will have any other type be a generic Array or ArrayComplex, but those
                        // could still be castable, for examples [1.0*1.0] is not an ArrayDoubleLiteral but would
                        // still be acceptable
                        if (expression->type == ast::NodeType::ArrayComplexLiteral)
                        {
                            auto arrayExpr = static_cast<ast::ArrayComplexLiteral *>(expression);
                            return std::make_shared<obj::ArrayComplex>(arrayExpr->elements);
                        }
                        else
                        {
                            // evaluate all elements, if they all evaluate to a double we are good
                            std::vector<std::shared_ptr<obj::Object>> objects = objectsFromArrayLiteral(expression, environment);
                            std::vector<std::complex<double>> doubleValues;
                            for (const auto &object : objects)
                            {
                                if (object->type != obj::ObjectType::Complex)
                                    return std::make_shared<obj::Error>("Trying to make an array of wrong type", obj::ErrorType::TypeError);
                                doubleValues.push_back(static_cast<obj::Complex *>(object.get())->value);
                            }
                            return std::make_shared<obj::ArrayComplex>(doubleValues);
                        }
                        return std::make_shared<obj::Error>("Trying to make an array of wrong type", obj::ErrorType::TypeError);
                    }
                    else
                    {
                        std::vector<std::shared_ptr<obj::Object>> objects = objectsFromArrayLiteral(expression, environment);
                        for (const auto &object : objects)
                        {
                            if (!typing::isCompatibleType(typeArray->elementType.get(), object.get(), nullptr))
                                return std::make_shared<obj::Error>("Trying to make an array with elements of wrong type", obj::ErrorType::TypeError);
                        }
                        return std::make_shared<obj::Array>(objects);
                    }
                }
                else
                {
                    std::vector<std::shared_ptr<obj::Object>> objects = objectsFromArrayLiteral(expression, environment);
                    for (const auto &object : objects)
                    {
                        if (!typing::isCompatibleType(typeArray->elementType.get(), object.get(), nullptr))
                            return std::make_shared<obj::Error>("Trying to make an array with elements of wrong type", obj::ErrorType::TypeError);
                    }
                    return std::make_shared<obj::Array>(objects);
                }
            }
            return std::make_shared<obj::Error>("Trying to make an array of wrong type", obj::ErrorType::TypeError);
        }
    }
    case ast::NodeType::DictLiteral:
    {
        auto dictExpr = static_cast<ast::DictLiteral *>(expression);
        std::unordered_map<std::shared_ptr<obj::Object>, std::shared_ptr<obj::Object>, obj::Hash, obj::Equal> objects;
        for (auto &element : dictExpr->elements)
        {
            auto elementObj = evalExpression(element.first.get(), environment);
            if (!elementObj->hashAble())
            {
                return std::make_shared<obj::Error>("Trying to add unhashable item to dict as key " + elementObj->inspect(), obj::ErrorType::TypeError);
            }
            objects.insert(std::make_pair(std::move(elementObj), evalExpression(element.second.get(), environment)));
        }
        return std::make_shared<obj::Dictionary>(objects);
    }
    case ast::NodeType::SetLiteral:
    {
        auto setExpr = static_cast<ast::SetLiteral *>(expression);
        std::unordered_set<std::shared_ptr<obj::Object>, obj::Hash, obj::Equal> objects;
        for (auto &element : setExpr->elements)
        {
            auto elementObj = evalExpression(element.get(), environment);
            if (!elementObj->hashAble())
            {
                return std::make_shared<obj::Error>("Trying to add unhashable item to set " + elementObj->inspect(), obj::ErrorType::TypeError);
            }
            objects.insert(std::move(elementObj));
        }
        return std::make_shared<obj::Set>(objects);
    }
    case ast::NodeType::PrefixExpression:
    {
        auto prefExpr = static_cast<ast::PrefixExpression *>(expression);
        return addTokenInCaseOfError(evalPrefixExpression(prefExpr->operator_t.type, evalExpression(prefExpr->right.get(), environment)), prefExpr->token);
    }
    case ast::NodeType::InfixExpression:
    {
        auto infixExpr = static_cast<ast::InfixExpression *>(expression);
        // assignment is a special case of infix
        if (infixExpr->operator_t.type == TokenType::ASSIGN)
        {
            // evalOpAssignmentOperator --> to eval +=, -=, ...
            // can it/should it be implemented as a combination of a += 1 === a = a + 1?  what about side effects?
            // does that still work for  (1 + 2) += 1?

            if (infixExpr->left->type == ast::NodeType::Identifier)
            {
                auto idExpr = static_cast<ast::Identifier *>(infixExpr->left.get());
                auto rightVal = std::move(evalExpression(infixExpr->right.get(), environment));
                return evalAssignmentOperator(idExpr, rightVal, environment);
            }

            auto indexExpr = dynamic_cast<ast::IndexExpression *>(infixExpr->left.get());
            if (indexExpr)
            {
                // auto rightVal = evalExpression(infixExpr->right.get(), environment);
                // return evalIndexAssignmentExpression(indexExpr, rightVal, environment);
                return evalIndexAssignmentExpression(indexExpr, infixExpr->right.get(), environment);
            }

            auto memberExpr = dynamic_cast<ast::MemberExpression *>(infixExpr->left.get());
            if (memberExpr)
            {
                return evalMemberAssignmentExpression(memberExpr, infixExpr->right.get(), environment);
            }

            return std::make_shared<obj::Error>("Lefthand of assignment needs to be identifier or index expression, found  " + expression->text(), obj::ErrorType::TypeError, infixExpr->token);
        }

        if (infixExpr->operator_t.type == TokenType::PLUSASSIGN || infixExpr->operator_t.type == TokenType::MINUSASSIGN || infixExpr->operator_t.type == TokenType::SLASHASSIGN || infixExpr->operator_t.type == TokenType::ASTERISKASSIGN)
        {
            if (infixExpr->left->type == ast::NodeType::Identifier)
            {
                auto idExpr = static_cast<ast::Identifier *>(infixExpr->left.get());
                auto rightVal = std::move(evalExpression(infixExpr->right.get(), environment));
                return evalOpAssignmentOperator(idExpr, infixExpr->operator_t.type, rightVal, environment);
            }

            auto indexExpr = dynamic_cast<ast::IndexExpression *>(infixExpr->left.get());
            if (indexExpr)
            {
                // auto rightVal = evalExpression(infixExpr->right.get(), environment);
                // return evalIndexAssignmentExpression(indexExpr, rightVal, environment);
                return evalIndexOpAssignmentExpression(indexExpr, infixExpr->operator_t.type, infixExpr->right.get(), environment);
            }

            return std::make_shared<obj::Error>("Lefthand of operator assignment needs to be identifier found  " + expression->text(), obj::ErrorType::TypeError, infixExpr->token);
        }

        auto leftVal = unwrapMemberValue(evalExpression(infixExpr->left.get(), environment));
        if (leftVal->type == obj::ObjectType::Error)
            return leftVal;
        auto rightVal = unwrapMemberValue(evalExpression(infixExpr->right.get(), environment));
        if (rightVal->type == obj::ObjectType::Error)
            return rightVal;
        return std::shared_ptr<obj::Object>(evalInfixOperator(infixExpr->operator_t.type, leftVal.get(), rightVal.get()));
    }
    case ast::NodeType::IfExpression:
        return evalIfExpression(static_cast<ast::IfExpression *>(expression), environment);
    case ast::NodeType::WhileExpression:
        return evalWhileExpression(static_cast<ast::WhileExpression *>(expression), environment);
    case ast::NodeType::ForExpression:
        return evalForExpression(static_cast<ast::ForExpression *>(expression), environment);
    case ast::NodeType::Identifier:
        return evalIdentifier(static_cast<ast::Identifier *>(expression), environment);
    case ast::NodeType::FunctionLiteral:
        return evalFunctionLiteral(static_cast<ast::FunctionLiteral *>(expression), environment);
    case ast::NodeType::TypeLiteral:
        return evalTypeLiteral(static_cast<ast::TypeLiteral *>(expression), environment);
    case ast::NodeType::CallExpression:
        return evalCallExpression(static_cast<ast::CallExpression *>(expression), environment);
    case ast::NodeType::IndexExpression:
        return evalIndexExpression(static_cast<ast::IndexExpression *>(expression), environment);
    case ast::NodeType::MemberExpression:
        return evalMemberExpression(static_cast<ast::MemberExpression *>(expression), environment);
    case ast::NodeType::ModuleMemberExpression:
        return evalModuleMemberExpression(static_cast<ast::ModuleMemberExpression *>(expression), environment);
    };

    if (expression)
        return std::make_shared<obj::Error>("Cannot evaluate " + expression->text(), obj::ErrorType::TypeError, expression->token);
    else
        return std::make_shared<obj::Error>("Cannot evaluate NULL", obj::ErrorType::TypeError);
}

std::shared_ptr<obj::Object> evalTryExceptStatement(ast::TryExceptStatement *statement, const std::shared_ptr<obj::Environment> &environment)
{
    if (!statement || !environment)
        return NullObject;

    auto retValue = evalStatement(statement->statement.get(), environment);
    if (retValue->type == obj::ObjectType::Error)
    {
        auto newEnvironment = makeNewEnvironment(environment);
        newEnvironment->add(statement->name.value, retValue, true, nullptr);
        auto exceptRetValue = evalStatement(statement->except.get(), newEnvironment);
        return exceptRetValue;
    }
    return NullObject;
}

std::shared_ptr<obj::Object> evalLetStatement(ast::LetStatement *statement, const std::shared_ptr<obj::Environment> &environment)
{
    if (!statement || !environment)
        return NullObject;

    auto exprValue = evalExpression(statement->value.get(), environment, statement->valueType.get());

    // an error can be assigned to a LHS?
    // if (exprValue->type == obj::ObjectType::Error)
    //    return exprValue;

    if (!typing::isCompatibleType(statement->valueType.get(), exprValue.get(), nullptr))
    {
        return std::make_shared<obj::Error>("Incompatible type " + statement->valueType->text() + " for " + statement->value->tokenLiteral(), obj::ErrorType::TypeError, statement->valueType->token);
    }
    // auto retValue = environment->add(statement->name.tokenLiteral(), std::move(exprValue), statement->constant, statement->type.get());
    std::shared_ptr<obj::Object> retValue;
    if (isValueAssigned(exprValue))
    {
        retValue = environment->add(statement->name.tokenLiteral(), exprValue->clone(), statement->constant, statement->valueType.get());
    }
    else
    {
        retValue = environment->add(statement->name.tokenLiteral(), std::move(exprValue), statement->constant, statement->valueType.get());
    }

    retValue->declaredType = statement->valueType.get();
    if (retValue->type == obj::ObjectType::Error)
        return addTokenInCaseOfError(retValue, statement->token);

    return NullObject;
}

std::string
modulePathToModuleName(const std::vector<std::string> &modulePath)
{
    return modulePath.back();
}

std::string
modulePathToModuleFileName(const std::filesystem::path &currentPath, std::vector<std::string> &modulePath)
{
    if (modulePath.empty())
        return std::string();

    std::filesystem::path constructedPath = currentPath;
    for (int i = 0; i < (static_cast<int>(modulePath.size()) - 1); ++i)
        constructedPath /= modulePath[i];

    constructedPath /= modulePath.back() + ".luci";
    return constructedPath.generic_string();
}

std::shared_ptr<obj::Object> evalImportStatement(ast::ImportStatement *statement, const std::shared_ptr<obj::Environment> &environment)
{
    const bool logModuleActivity = false;
    std::vector<std::string> log;
    if (statement && environment)
    {
        std::shared_ptr<obj::Module> moduleObj;
        std::string moduleText;

        auto modulePath = statement->name.path;
        auto localModuleName = modulePathToModuleName(modulePath);
        if (builtinModules.find(modulePath.front()) != builtinModules.end())
        {
            if (logModuleActivity)
                log.push_back("builtin toplevel module=" + modulePath.front());

            // check if the full path exists by walking through the module hierarchy
            auto modulePathToWalk = modulePath;
            moduleObj = builtinModules.at(modulePath.front());
            for (int modIdx = 1; modIdx < static_cast<int>(modulePath.size()); ++modIdx)
            {
                if (moduleObj->environment->has(modulePath.at(modIdx)))
                {
                    auto obj = moduleObj->environment->get(modulePath.at(modIdx));
                    if (obj->type != obj::ObjectType::Module)
                        return std::make_shared<obj::Error>("import: " + util::join(modulePath, "::") + " failed to import, builtin module not found", obj::ErrorType::ImportError);

                    moduleObj = std::dynamic_pointer_cast<obj::Module>(obj);
                }
            }
        }
        else
        {
            auto fileName = modulePathToModuleFileName(std::filesystem::current_path(), modulePath);
            if (logModuleActivity)
            {
                log.push_back("modulePath=" + util::join(modulePath, "::"));
                log.push_back("localModuleName=" + localModuleName);
                log.push_back("fileName=" + fileName);
            }
            std::string text;
            std::string line;
            std::ifstream inputf;
            inputf.open(fileName);
            if (inputf.is_open())
            {
                while (std::getline(inputf, line))
                    text += line + "\n";
            }
            else
            {
                if (logModuleActivity)
                {
                    log.push_back("fileName not found");
                    std::cerr << util::join(log, "\n") << std::endl;
                }
                return std::make_shared<obj::Error>("import: " + util::join(modulePath, "::") + " failed to import, file " + fileName + " not found", obj::ErrorType::ImportError);
            }

            auto newEnvironment = makeNewEnvironment(nullptr);
            moduleObj = std::make_shared<obj::Module>();
            moduleObj->environment = newEnvironment;
            moduleObj->fileName = fileName;
            moduleObj->state = obj::ModuleState::Unknown;
            moduleText = std::move(text);
        }

        std::shared_ptr<obj::Environment> whereToAddModule = environment;

        // check if we need to construct module definition on the path towards a potential submodule
        for (int modIdx = 0; modIdx < (static_cast<int>(modulePath.size()) - 1); ++modIdx)
        {
            const auto &moduleName = modulePath.at(modIdx);
            if (logModuleActivity)
            {
                log.push_back("module hierarchy, moduleName=" + moduleName);
            }
            bool hasName = whereToAddModule->has(moduleName);
            if (hasName)
            {
                if (logModuleActivity)
                    log.push_back(" " + moduleName + " found");

                // check if the name refers to a module otherwise need to error out
                auto referredObj = whereToAddModule->get(moduleName);
                if (referredObj->type != obj::ObjectType::Module)
                {
                    if (logModuleActivity)
                    {
                        log.push_back(" " + moduleName + " was not a module but of type " + obj::toString(referredObj->type));
                        std::cerr << util::join(log, "\n") << std::endl;
                    }
                    return std::make_shared<obj::Error>("import: " + util::join(modulePath, "::") + " failed to import, name " + moduleName + " already used", obj::ErrorType::ImportError);
                }
                whereToAddModule = static_cast<obj::Module *>(referredObj.get())->environment;
            }
            else
            {
                if (logModuleActivity)
                    log.push_back(" defined module created " + moduleName);
                auto moduleDefinitionObj = std::make_shared<obj::Module>();
                moduleDefinitionObj->environment = makeNewEnvironment(nullptr);
                moduleDefinitionObj->state = obj::ModuleState::Defined;
                whereToAddModule->add(moduleName, moduleDefinitionObj, false, nullptr);
                whereToAddModule = moduleDefinitionObj->environment;
            }
        }

        if (whereToAddModule->has(localModuleName))
        {
            if (logModuleActivity)
                log.push_back(localModuleName + " already found");
            auto referredObj = whereToAddModule->get(localModuleName);
            if (referredObj->type != obj::ObjectType::Module)
            {
                if (logModuleActivity)
                {
                    log.push_back(localModuleName + " was not a module");
                    std::cerr << util::join(log, "\n") << std::endl;
                }
                return std::make_shared<obj::Error>("import: " + util::join(modulePath, "::") + " failed to import, name " + localModuleName + " already used", obj::ErrorType::ImportError);
            }
            auto existingModule = static_cast<obj::Module *>(referredObj.get());

            switch (existingModule->state)
            {
            case obj::ModuleState::Unknown:
                if (logModuleActivity)
                {
                    log.push_back(localModuleName + " was a module in Unknown state");
                    std::cerr << util::join(log, "\n") << std::endl;
                }
                return std::make_shared<obj::Error>("import: " + util::join(modulePath, "::") + " in unknown state", obj::ErrorType::ImportError);
            case obj::ModuleState::Loaded:
                if (logModuleActivity)
                {
                    log.push_back(localModuleName + " was a module in Loaded state");
                    std::cerr << util::join(log, "\n") << std::endl;
                }
                return NullObject;
            case obj::ModuleState::Defined:
            {
                auto runResult = builtin::run_impl(moduleText, moduleObj->fileName, moduleObj->environment);
                if (!runResult)
                {
                    if (logModuleActivity)
                    {
                        log.push_back("fatal failure to run the module src");
                        std::cerr << util::join(log, "\n") << std::endl;
                    }
                    return std::make_shared<obj::Error>("import: " + util::join(modulePath, "::") + " cannot be loaded, evaluation failed", obj::ErrorType::ImportError);
                }
                if (runResult->type == obj::ObjectType::Error)
                {
                    if (logModuleActivity)
                    {
                        log.push_back("execution of module code resulted in error");
                        std::cerr << util::join(log, "\n") << std::endl;
                    }
                    return runResult;
                }
                moduleObj->state = obj::ModuleState::Loaded;

                // transfer all previously loaded sub modules into the moduleObj, any conflicts become fatal errors
                // conflicts may arise when a module has a submodule named foo but also a variable foo
                // the submodule may succesfully load but later when then loading the parent module becomes a conflict
                log.push_back(localModuleName + " was a module in Defined state");
                for (const auto &[name, obj] : existingModule->environment->store)
                {
                    if (logModuleActivity)
                        log.push_back(" " + name + ":" + obj::toString(obj.obj->type) + " object in defined module");
                    if (obj.obj->type != obj::ObjectType::Module)
                    {
                        if (logModuleActivity)
                            std::cerr << util::join(log, "\n") << std::endl;
                        return std::make_shared<obj::Error>("import: " + util::join(modulePath, "::") + " a module definition contains other objects beyond other modules", obj::ErrorType::ImportError);
                    }

                    if (moduleObj->environment->has(name))
                    {
                        if (logModuleActivity)
                            log.push_back(" " + name + " also found in module environment");
                        auto referredObjInModule = moduleObj->environment->get(name);
                        if (referredObjInModule->type != obj::ObjectType::Module)
                        {
                            if (logModuleActivity)
                            {
                                log.push_back(" " + name + " was not a module and conflicts");
                                std::cerr << util::join(log, "\n") << std::endl;
                            }
                            return std::make_shared<obj::Error>("import: " + util::join(modulePath, "::") + " failed, sub module " + name + " is in conflict with variable/functions defined in module", obj::ErrorType::ImportError);
                        }
                        if (logModuleActivity)
                            log.push_back(" " + name + " assigned in module environment");
                        moduleObj->environment->set(name, obj.obj);
                    }
                    else
                    {
                        if (logModuleActivity)
                            log.push_back(" " + name + " added in module environment");
                        moduleObj->environment->add(name, obj.obj, obj.constant, obj.type);
                    }
                }
                whereToAddModule->set(localModuleName, moduleObj);
            }
            break;
            }
        }
        else
        {
            if (moduleObj->state == obj::ModuleState::Unknown)
            {
                // for modules that are builtin, the state will be loaded/defined, so no need to execute the code for them
                auto runResult = builtin::run_impl(moduleText, moduleObj->fileName, moduleObj->environment);
                if (runResult->type == obj::ObjectType::Error)
                {
                    if (logModuleActivity)
                    {
                        log.push_back("execution of module code resulted in error");
                        std::cerr << util::join(log, "\n") << std::endl;
                    }
                    return runResult;
                }
            }
            moduleObj->state = obj::ModuleState::Loaded;
            if (logModuleActivity)
                log.push_back(" " + localModuleName + " added in final module environment");
            whereToAddModule->add(localModuleName, moduleObj, false, nullptr);
        }
    }

    if (logModuleActivity)
        std::cerr << util::join(log, "\n") << std::endl;
    return NullObject;
}

std::shared_ptr<obj::Object> evalStatements(std::vector<std::unique_ptr<ast::Statement>> *statements, const std::shared_ptr<obj::Environment> &environment)
{
    std::shared_ptr<obj::Object> result;
    for (auto stmtIt = statements->begin(); stmtIt != statements->end(); ++stmtIt)
    {
        result = std::move(evalStatement(stmtIt->get(), environment));
        if (!result)
            continue;

        switch (result->type)
        {
        case obj::ObjectType::ReturnValue:
        case obj::ObjectType::BreakValue:
        case obj::ObjectType::ContinueValue:
        case obj::ObjectType::Error:
        case obj::ObjectType::Exit:
            return result;
        };
    }
    return result;
}

std::shared_ptr<obj::Object> evalStatement(ast::Statement *statement, const std::shared_ptr<obj::Environment> &environment)
{
    if (!statement)
        return std::make_shared<obj::Error>("Unknown NULL statement", obj::ErrorType::TypeError);

    switch (statement->type)
    {
    case ast::NodeType::ExpressionStatement:
        return addTokenInCaseOfError(evalExpression(static_cast<ast::ExpressionStatement *>(statement)->expression.get(), environment), statement->token);
    case ast::NodeType::ReturnStatement:
        return std::make_shared<obj::ReturnValue>(evalExpression(static_cast<ast::ReturnStatement *>(statement)->returnValue.get(), environment));
    case ast::NodeType::BreakStatement:
        return std::make_shared<obj::BreakValue>();
    case ast::NodeType::ContinueStatement:
        return std::make_shared<obj::ContinueValue>();
    case ast::NodeType::BlockStatement:
        return evalStatements(&static_cast<ast::BlockStatement *>(statement)->statements, environment);
    case ast::NodeType::ScopeStatement:
    {
        auto newScopedEnvironment = makeNewEnvironment(environment);
        auto retValue = evalStatements(&static_cast<ast::ScopeStatement *>(statement)->statements, newScopedEnvironment);
        auto desRetValue = evalUserObjectDestructors(newScopedEnvironment);
        if (desRetValue->type == obj::ObjectType::Error || desRetValue->type == obj::ObjectType::Exit)
            return desRetValue;
        return retValue;
    }
    case ast::NodeType::LetStatement:
        return evalLetStatement(static_cast<ast::LetStatement *>(statement), environment);
    case ast::NodeType::TryExceptStatement:
        return evalTryExceptStatement(static_cast<ast::TryExceptStatement *>(statement), environment);
    case ast::NodeType::ImportStatement:
        return evalImportStatement(static_cast<ast::ImportStatement *>(statement), environment);
    };

    return std::make_shared<obj::Error>("Unknown statement", obj::ErrorType::TypeError, statement->token);
}

std::shared_ptr<obj::Object> evalProgram(ast::Program *program, const std::shared_ptr<obj::Environment> &environment)
{
    auto result = evalStatements(&program->statements, environment);

    do
    {
        auto retResult = dynamic_cast<obj::ReturnValue *>(result.get());
        if (retResult)
            result = retResult->value;
        else
            break;
    } while (true);

    return result;
}

std::shared_ptr<obj::Object> eval(std::unique_ptr<ast::Node> node, const std::shared_ptr<obj::Environment> &environment)
{
    return eval(node.get(), environment);
}

std::shared_ptr<obj::Object> eval(ast::Node *node, const std::shared_ptr<obj::Environment> &environment)
{
    switch (node->type)
    {
    case ast::NodeType::Program:
        return evalProgram(static_cast<ast::Program *>(node), environment);
    case ast::NodeType::Expression:
        return evalExpression(static_cast<ast::Expression *>(node), environment);
    case ast::NodeType::Statement:
    case ast::NodeType::BlockStatement:
        return evalStatement(static_cast<ast::Statement *>(node), environment);
    }
    return NullObject;
}
