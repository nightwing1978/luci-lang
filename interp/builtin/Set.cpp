#include "Set.h"
#include "../Typing.h"

namespace
{
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
        if (arguments.size() != nrExpectedArguments)
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + std::to_string(nrExpectedArguments) + " arguments, got " + std::to_string(arguments.size()), obj::ErrorType::TypeError);
        return nullptr;
    }
}

namespace builtin
{
    std::shared_ptr<obj::Object> set_size(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("size", self, arguments, obj::ObjectType::Set, 0);
        if (errorObj)
            return errorObj;
        return std::make_shared<obj::Integer>(static_cast<obj::Set *>(self.get())->value.size());
    }

    std::shared_ptr<obj::Object> set_clear(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        if (self->frozen > 0)
            return std::make_shared<obj::Error>("set clear expects a non-frozen object", obj::ErrorType::TypeError);

        auto errorObj = validateArguments("clear", self, arguments, obj::ObjectType::Set, 0);
        if (errorObj)
            return errorObj;

        static_cast<obj::Set *>(self.get())->value.clear();
        return self;
    }

    std::shared_ptr<obj::Object> set_empty(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("empty", self, arguments, obj::ObjectType::Set, 0);
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Boolean>(static_cast<obj::Set *>(self.get())->value.empty());
    }

    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeSet()
    {
        typedef obj::TBuiltinTypeFunctionDefinition TBuiltInFD;

        auto setBuiltinType = std::make_shared<obj::BuiltinType>();
        setBuiltinType->builtinObjectType = obj::ObjectType::Set;

        setBuiltinType->functions = {
            {"clear", TBuiltInFD({&builtin::set_clear, typing::makeFunctionType("", "{all:all}")})}, // "{all}.fn() -> {all:all}"
            {"empty", TBuiltInFD({&builtin::set_empty, typing::makeFunctionType("", "{all:all}")})}, // "{all}.fn() -> {all:all}"
            {"size", TBuiltInFD({&builtin::set_size, typing::makeFunctionType("", "int")})}          // "{all}.fn() -> int"
        };

        return setBuiltinType;
    }
}