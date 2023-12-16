/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Thread.h"
#include "../Evaluator.h"
#include "../Typing.h"
#include "../Util.h"

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
}

namespace obj
{
    Thread::Thread() : Object(ObjectType::Thread)
    {
    }

    Thread::~Thread()
    {
    }

    void Thread::join()
    {
        if (thread)
            thread->join();
    }

    void Thread::run()
    {
        auto environment = std::make_shared<obj::Environment>();
        environment->outer = function->environment;
        if (argument)
            functionReturnValue = evalFunctionWithArguments(function.get(), {argument}, environment);
        else
            functionReturnValue = evalFunctionWithArguments(function.get(), {}, environment);
    }

    void Thread::start()
    {
        if (thread)
        {
            throw std::runtime_error("Cannot start thread twice");
        }

        thread.reset(new std::thread(&Thread::run, this));
    }

    bool Thread::joinable() const
    {
        if (thread)
            return thread->joinable();
        return true;
    }

    void Thread::detach()
    {
        if (thread)
            thread->detach();
        return;
    }

    std::string Thread::inspect() const
    {
        return "<thread>";
    }
}

namespace builtin
{
    std::shared_ptr<obj::Object> thread_start(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("start", self, arguments, obj::ObjectType::Thread, {0, 1});
        if (errorObj)
            return errorObj;

        static_cast<obj::Thread *>(self.get())->start();
        return NullObject;
    }

    std::shared_ptr<obj::Object> thread_detach(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("detach", self, arguments, obj::ObjectType::Thread, {0});
        if (errorObj)
            return errorObj;

        static_cast<obj::Thread *>(self.get())->detach();
        return NullObject;
    }

    std::shared_ptr<obj::Object> thread_join(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("join", self, arguments, obj::ObjectType::Thread, {0});
        if (errorObj)
            return errorObj;

        static_cast<obj::Thread *>(self.get())->join();
        return NullObject;
    }

    std::shared_ptr<obj::Object> thread_joinable(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("joinable", self, arguments, obj::ObjectType::Thread, {0});
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Boolean>(static_cast<obj::Thread *>(self.get())->joinable());
    }

    std::shared_ptr<obj::Object> thread_value(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("value", self, arguments, obj::ObjectType::Thread, {0});
        if (errorObj)
            return errorObj;

        return static_cast<obj::Thread *>(self.get())->functionReturnValue;
    }

    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeThread()
    {
        typedef obj::TBuiltinTypeFunctionDefinition TBuiltInFD;

        auto threadBuiltinType = std::make_shared<obj::BuiltinType>();
        threadBuiltinType->builtinObjectType = obj::ObjectType::Thread;

        threadBuiltinType->functions = {
            {"start", TBuiltInFD({&builtin::thread_start, typing::makeFunctionType("", "null")})},
            {"detach", TBuiltInFD({&builtin::thread_detach, typing::makeFunctionType("", "null")})},
            {"join", TBuiltInFD({&builtin::thread_join, typing::makeFunctionType("", "null")})},
            {"joinable", TBuiltInFD({&builtin::thread_joinable, typing::makeFunctionType("", "bool")})},
            {"value", TBuiltInFD({&builtin::thread_value, typing::makeFunctionType("", "all")})},
        };

        return threadBuiltinType;
    }
} // namespace builtin
