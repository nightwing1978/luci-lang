#include "Error.h"
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
            return obj::makeTypeError(errorPrefix + ": expected " + toString(expectedType) + ", got " + toString(self.get()->type));
        if (arguments.size() != nrExpectedArguments)
            return obj::makeTypeError(errorPrefix + ": expected " + std::to_string(nrExpectedArguments) + " arguments, got " + std::to_string(arguments.size()));
        return nullptr;
    }
}

namespace builtin
{
    std::shared_ptr<obj::Object> error_message(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("empty", self, arguments, obj::ObjectType::Error, 0);
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::String>(static_cast<obj::Error *>(self.get())->msg);
    }

    std::shared_ptr<obj::Object> error_type(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("type", self, arguments, obj::ObjectType::Error, 0);
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Integer>(static_cast<int>(static_cast<obj::Error *>(self.get())->errorType));
    }

    std::shared_ptr<obj::Object> error_file_name(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("file_name", self, arguments, obj::ObjectType::Error, 0);
        if (errorObj)
            return errorObj;

        if (static_cast<obj::Error *>(self.get())->token.fileName)
            return std::make_shared<obj::String>(*static_cast<obj::Error *>(self.get())->token.fileName);
        return std::make_shared<obj::String>(std::string());
    }

    std::shared_ptr<obj::Object> error_line(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("line", self, arguments, obj::ObjectType::Error, 0);
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Integer>(static_cast<obj::Error *>(self.get())->token.lineNumber);
    }

    std::shared_ptr<obj::Object> error_column(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("column", self, arguments, obj::ObjectType::Error, 0);
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Integer>(static_cast<obj::Error *>(self.get())->token.columnNumber);
    }

    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeError()
    {
        typedef obj::TBuiltinTypeFunctionDefinition TBuiltInFD;

        auto errorBuiltinType = std::make_shared<obj::BuiltinType>();
        errorBuiltinType->builtinObjectType = obj::ObjectType::Error;

        errorBuiltinType->functions = {
            {"message", TBuiltInFD({&builtin::error_message, typing::makeFunctionType("", "str")})},     // "error.fn() -> str"
            {"error_type", TBuiltInFD({&builtin::error_type, typing::makeFunctionType("", "int")})},     // "error.fn() -> int"
            {"file_name", TBuiltInFD({&builtin::error_file_name, typing::makeFunctionType("", "str")})}, // "error.fn() -> str"
            {"line", TBuiltInFD({&builtin::error_line, typing::makeFunctionType("", "int")})},           // "error.fn() -> int"
            {"column", TBuiltInFD({&builtin::error_column, typing::makeFunctionType("", "int")})},       // "error.fn() -> int"
        };

        return errorBuiltinType;
    }
}