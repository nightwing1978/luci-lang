#include "IO.h"
#include "../Evaluator.h"
#include "../Util.h"
#include "../Typing.h"

#include <algorithm>

namespace
{
    std::string to_string(const std::vector<size_t> &sizes)
    {
        std::vector<std::string> strings;
        for (const auto &value : sizes)
            strings.push_back(std::to_string(value));
        return "[" + util::join(strings, ",") + "]";
    }

    std::shared_ptr<obj::Object>
    validateArguments(
        const std::string &errorPrefix,
        const std::shared_ptr<obj::Object> &self,
        const std::vector<std::shared_ptr<obj::Object>> &arguments,
        const obj::ObjectType expectedType,
        std::vector<size_t> nrExpectedArguments)
    {
        if (self.get()->type != expectedType)
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + toString(expectedType) + ", got " + toString(self.get()->type));
        if (std::find(nrExpectedArguments.begin(), nrExpectedArguments.end(), arguments.size()) == nrExpectedArguments.end())
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + to_string(nrExpectedArguments) + " arguments, got " + std::to_string(arguments.size()));
        return nullptr;
    }
}

namespace builtin
{
    std::shared_ptr<obj::Object> io_open(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("open", self, arguments, obj::ObjectType::IOObject, {1, 2});
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("open: expected argument 1 of type string");
        if (arguments.size() >= 1 && arguments[1]->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("open: expected argument 2 of type string");

        auto fileName = static_cast<obj::String *>(arguments[0].get())->value;
        std::string openModeStr = "r";
        if (arguments.size() >= 1)
            openModeStr = static_cast<obj::String *>(arguments[1].get())->value;

        std::map<std::string, std::ios_base::openmode> openModeMapping = {
            {"r", std::ios_base::in},
            {"rb", std::ios_base::in | std::ios_base::binary},
            {"w", std::ios_base::out},
            {"wb", std::ios_base::out | std::ios_base::binary},
            {"a", std::ios_base::app},
        };

        if (openModeMapping.find(openModeStr) == openModeMapping.end())
        {
            std::vector<std::string> openModeChoices;
            for (const auto &[k, v] : openModeMapping)
                openModeChoices.push_back(k);
            return std::make_shared<obj::Error>("open: openmode has to be one of " + util::join(openModeChoices, ","));
        }

        std::ios_base::openmode openMode = openModeMapping.at(openModeStr);
        static_cast<obj::IOObject *>(self.get())->open(fileName, openMode);
        return self;
    }

    std::shared_ptr<obj::Object> io_is_open(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("is_open", self, arguments, obj::ObjectType::IOObject, {0});
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Boolean>(static_cast<obj::IOObject *>(self.get())->isOpen());
    }

    std::shared_ptr<obj::Object> io_close(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("close", self, arguments, obj::ObjectType::IOObject, {0});
        if (errorObj)
            return errorObj;

        static_cast<obj::IOObject *>(self.get())->close();
        return NullObject;
    }

    std::shared_ptr<obj::Object> io_read(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("read", self, arguments, obj::ObjectType::IOObject, {0, 1});
        if (errorObj)
            return errorObj;

        size_t maxSize = std::numeric_limits<size_t>::max();
        if (arguments.size() == 1)
        {
            if (arguments[0]->type != obj::ObjectType::Integer)
                return std::make_shared<obj::Error>("read: expected argument 1 of type int");

            maxSize = static_cast<size_t>(static_cast<obj::Integer *>(arguments[0].get())->value);
        }

        std::string readStr = static_cast<obj::IOObject *>(self.get())->read(maxSize);
        return std::make_shared<obj::String>(readStr);
    }

    std::shared_ptr<obj::Object> io_readline(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("read_line", self, arguments, obj::ObjectType::IOObject, {0, 1});
        if (errorObj)
            return errorObj;

        size_t maxSize = std::numeric_limits<size_t>::max();
        if (arguments.size() == 1)
        {
            if (arguments[0]->type != obj::ObjectType::Integer)
                return std::make_shared<obj::Error>("read_line: expected argument 1 of type int");

            maxSize = static_cast<size_t>(static_cast<obj::Integer *>(arguments[0].get())->value);
        }

        std::string readLineStr = static_cast<obj::IOObject *>(self.get())->readLine(maxSize);
        return std::make_shared<obj::String>(readLineStr);
    }

    std::shared_ptr<obj::Object> io_readlines(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("read_lines", self, arguments, obj::ObjectType::IOObject, {0, 1});
        if (errorObj)
            return errorObj;

        size_t hint = std::numeric_limits<size_t>::max();
        if (arguments.size() == 1)
        {
            if (arguments[0]->type != obj::ObjectType::Integer)
                return std::make_shared<obj::Error>("read_lines: expected argument 1 of type int");

            hint = static_cast<size_t>(static_cast<obj::Integer *>(arguments[0].get())->value);
        }

        std::vector<std::string> readLinesStr = static_cast<obj::IOObject *>(self.get())->readLines(hint);
        std::vector<std::shared_ptr<obj::Object>> arrayObj;
        for (const auto &element : readLinesStr)
            arrayObj.push_back(std::make_shared<obj::String>(element));
        return std::make_shared<obj::Array>(arrayObj);
    }

    std::shared_ptr<obj::Object> io_seek(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("seek", self, arguments, obj::ObjectType::IOObject, {1, 2});
        if (errorObj)
            return errorObj;

        size_t offset = 0;
        int whence = 0;
        if (arguments.size() >= 1)
        {
            if (arguments[0]->type != obj::ObjectType::Integer)
                return std::make_shared<obj::Error>("seek: expected argument 1 of type int");

            offset = static_cast<size_t>(static_cast<obj::Integer *>(arguments[0].get())->value);
        }
        if (arguments.size() == 2)
        {
            if (arguments[1]->type != obj::ObjectType::Integer)
                return std::make_shared<obj::Error>("seek: expected argument 2 of type int");

            whence = static_cast<int>(static_cast<obj::Integer *>(arguments[0].get())->value);
            if (whence != 0 && whence != 1 && whence != 2)
            {
                return std::make_shared<obj::Error>("seek: expected argument 2 to be either 0,1,2");
            }
        }

        static_cast<obj::IOObject *>(self.get())->seek(offset, whence);
        return NullObject;
    }

    std::shared_ptr<obj::Object> io_tell(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("tell", self, arguments, obj::ObjectType::IOObject, {0});
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Integer>(static_cast<obj::IOObject *>(self.get())->tell());
    }

    std::shared_ptr<obj::Object> io_write(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("write", self, arguments, obj::ObjectType::IOObject, {1});
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::String)
            return std::make_shared<obj::Error>("write: expected argument 1 of type str");

        const std::string &value = static_cast<obj::String *>(arguments[0].get())->value;
        static_cast<obj::IOObject *>(self.get())->write(value);
        return NullObject;
    }

    std::shared_ptr<obj::Object> io_flush(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("flush", self, arguments, obj::ObjectType::IOObject, {0});
        if (errorObj)
            return errorObj;

        static_cast<obj::IOObject *>(self.get())->flush();
        return NullObject;
    }

    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeIo()
    {
        typedef obj::TBuiltinTypeFunctionDefinition TBuiltInFD;

        auto ioBuiltinType = std::make_shared<obj::BuiltinType>();
        ioBuiltinType->builtinObjectType = obj::ObjectType::IOObject;

        ioBuiltinType->functions = {
            {"open", TBuiltInFD({&builtin::io_open, typing::makeFunctionType("str,str", "io")})},  // "io.fn() -> str"
            {"is_open", TBuiltInFD({&builtin::io_is_open, typing::makeFunctionType("", "bool")})}, // "io.fn() -> str"
            {"close", TBuiltInFD({&builtin::io_close, typing::makeFunctionType("", "null")})},     // "io.fn() -> str"

            {"read", TBuiltInFD({&builtin::io_read, typing::makeFunctionType("", "str")})},              // "io.fn() -> str"
            {"read_line", TBuiltInFD({&builtin::io_readline, typing::makeFunctionType("", "str")})},     // "io.fn() -> str"
            {"read_lines", TBuiltInFD({&builtin::io_readlines, typing::makeFunctionType("", "[str]")})}, // "io.fn() -> str"

            {"seek", TBuiltInFD({&builtin::io_seek, typing::makeFunctionType("int,int", "null")})}, // "io.fn() -> str"
            {"tell", TBuiltInFD({&builtin::io_tell, typing::makeFunctionType("", "int")})},         // "io.fn() -> str"
            {"write", TBuiltInFD({&builtin::io_write, typing::makeFunctionType("str", "null")})},   // "io.fn() -> str"
            {"flush", TBuiltInFD({&builtin::io_flush, typing::makeFunctionType("", "null")})},      // "io.fn() -> str"
        };

        return ioBuiltinType;
    }
}