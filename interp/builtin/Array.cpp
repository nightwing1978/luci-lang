#include "Array.h"
#include "../Typing.h"
#include <algorithm>

namespace
{
    std::shared_ptr<obj::Object>
    validateArguments(
        const std::string &errorPrefix,
        const std::shared_ptr<obj::Object> &self,
        const std::vector<std::shared_ptr<obj::Object>> &arguments,
        size_t nrExpectedArguments)
    {
        if (self.get()->type != obj::ObjectType::Array && self.get()->type != obj::ObjectType::ArrayDouble && self.get()->type != obj::ObjectType::ArrayComplex)
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + toString(obj::ObjectType::Array) + ", got " + toString(self.get()->type), obj::ErrorType::TypeError);
        if (arguments.size() != nrExpectedArguments)
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + std::to_string(nrExpectedArguments) + " arguments, got " + std::to_string(arguments.size()), obj::ErrorType::TypeError);
        return nullptr;
    }
}

namespace builtin
{
    std::shared_ptr<obj::Object> array_size(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("size", self, arguments, 0);
        if (errorObj)
            return errorObj;
        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
            return std::make_shared<obj::Integer>(static_cast<obj::Array *>(self.get())->value.size());
        case obj::ObjectType::ArrayDouble:
            return std::make_shared<obj::Integer>(static_cast<obj::ArrayDouble *>(self.get())->value.size());
        case obj::ObjectType::ArrayComplex:
            return std::make_shared<obj::Integer>(static_cast<obj::ArrayComplex *>(self.get())->value.size());
        default:
            return std::make_shared<obj::Error>("Method unavailable for type", obj::ErrorType::TypeError);
        }
        return std::make_shared<obj::Error>("Method unavailable for type", obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> array_capacity(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("capacity", self, arguments, 0);
        if (errorObj)
            return errorObj;
        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
            return std::make_shared<obj::Integer>(static_cast<obj::Array *>(self.get())->value.capacity());
        case obj::ObjectType::ArrayDouble:
            return std::make_shared<obj::Integer>(static_cast<obj::ArrayDouble *>(self.get())->value.capacity());
        case obj::ObjectType::ArrayComplex:
            return std::make_shared<obj::Integer>(static_cast<obj::ArrayComplex *>(self.get())->value.capacity());
        }
        return std::make_shared<obj::Error>("Method unavailable for type", obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> array_clear(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        if (self->frozen > 0)
            return std::make_shared<obj::Error>("array clear expects a non-frozen object", obj::ErrorType::TypeError);

        auto errorObj = validateArguments("clear", self, arguments, 0);
        if (errorObj)
            return errorObj;

        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
            static_cast<obj::Array *>(self.get())->value.clear();
            return self;
        case obj::ObjectType::ArrayDouble:
            static_cast<obj::ArrayDouble *>(self.get())->value.clear();
            return self;
        case obj::ObjectType::ArrayComplex:
            static_cast<obj::ArrayComplex *>(self.get())->value.clear();
            return self;
        }
        return std::make_shared<obj::Error>("Method unavailable for type", obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> array_empty(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("empty", self, arguments, 0);
        if (errorObj)
            return errorObj;

        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
            return std::make_shared<obj::Boolean>(static_cast<obj::Array *>(self.get())->value.empty());
        case obj::ObjectType::ArrayDouble:
            return std::make_shared<obj::Boolean>(static_cast<obj::ArrayDouble *>(self.get())->value.empty());
        case obj::ObjectType::ArrayComplex:
            return std::make_shared<obj::Boolean>(static_cast<obj::ArrayComplex *>(self.get())->value.empty());
        }
        return std::make_shared<obj::Error>("Method unavailable for type", obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> array_pop_back(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        if (self->frozen > 0)
            return std::make_shared<obj::Error>("array pop_back expects a non-frozen object", obj::ErrorType::TypeError);

        auto errorObj = validateArguments("pop_back", self, arguments, 0);
        if (errorObj)
            return errorObj;

        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
            static_cast<obj::Array *>(self.get())->value.pop_back();
            break;
        case obj::ObjectType::ArrayDouble:
            static_cast<obj::ArrayDouble *>(self.get())->value.pop_back();
            break;
        case obj::ObjectType::ArrayComplex:
            static_cast<obj::ArrayComplex *>(self.get())->value.pop_back();
            break;
        }
        return self;
    }

    std::shared_ptr<obj::Object> array_push_back(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        if (self->frozen > 0)
            return std::make_shared<obj::Error>("array push_back expects a non-frozen object", obj::ErrorType::TypeError);

        auto errorObj = validateArguments("push_back", self, arguments, 1);
        if (errorObj)
            return errorObj;

        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
        {
            auto arrayObj = static_cast<obj::Array *>(self.get());
            arrayObj->value.push_back(arguments[0]);
            return self;
        }
        case obj::ObjectType::ArrayDouble:
        {
            if (arguments[0]->type == obj::ObjectType::Double)
            {
                static_cast<obj::ArrayDouble *>(self.get())->value.push_back(static_cast<obj::Double *>(arguments[0].get())->value);
                return self;
            }
            return std::make_shared<obj::Error>("Cannot push a non-double to a [double]", obj::ErrorType::TypeError);
        }
        case obj::ObjectType::ArrayComplex:
        {
            if (arguments[0]->type == obj::ObjectType::Complex)
            {
                static_cast<obj::ArrayComplex *>(self.get())->value.push_back(static_cast<obj::Complex *>(arguments[0].get())->value);
                return self;
            }
            return std::make_shared<obj::Error>("Cannot push a non-double to a [complex]", obj::ErrorType::TypeError);
        }
        }
        return std::make_shared<obj::Error>("Method unavailable for type", obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> array_reserve(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("reserve", self, arguments, 1);
        if (errorObj)
            return errorObj;
        if (arguments[0]->type != obj::ObjectType::Integer)
            return std::make_shared<obj::Error>("reserve: expected argument 1 of type integer", obj::ErrorType::TypeError);
        auto capacity = static_cast<obj::Integer *>(arguments[0].get());
        if (capacity->value < 0)
            return std::make_shared<obj::Error>("reserve: capacity needs to be zero or larger", obj::ErrorType::TypeError);

        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
            static_cast<obj::Array *>(self.get())->value.reserve(static_cast<size_t>(capacity->value));
            break;
        case obj::ObjectType::ArrayDouble:
            static_cast<obj::ArrayDouble *>(self.get())->value.reserve(static_cast<size_t>(capacity->value));
            break;
        case obj::ObjectType::ArrayComplex:
            static_cast<obj::ArrayComplex *>(self.get())->value.reserve(static_cast<size_t>(capacity->value));
            break;
        }
        return self;
    }

    std::shared_ptr<obj::Object> array_reverse(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        if (self->frozen > 0)
            return std::make_shared<obj::Error>("array reverse expects a non-frozen object", obj::ErrorType::TypeError);

        auto errorObj = validateArguments("reverse", self, arguments, 0);
        if (errorObj)
            return errorObj;

        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
        {
            auto arrayObj = static_cast<obj::Array *>(self.get());
            std::reverse(arrayObj->value.begin(), arrayObj->value.end());
            break;
        }
        case obj::ObjectType::ArrayDouble:
        {
            auto arrayObj = static_cast<obj::ArrayDouble *>(self.get());
            std::reverse(arrayObj->value.begin(), arrayObj->value.end());
            break;
        }
        case obj::ObjectType::ArrayComplex:
        {
            auto arrayObj = static_cast<obj::ArrayComplex *>(self.get());
            std::reverse(arrayObj->value.begin(), arrayObj->value.end());
            break;
        }
        }
        return self;
    }

    std::shared_ptr<obj::Object> array_reversed(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("reversed", self, arguments, 0);
        if (errorObj)
            return errorObj;

        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
        {
            auto arrayObj = static_cast<obj::Array *>(self.get());
            std::vector<std::shared_ptr<obj::Object>> values(arrayObj->value);
            std::reverse(values.begin(), values.end());
            return std::make_shared<obj::Array>(obj::Array(values));
        }
        case obj::ObjectType::ArrayDouble:
        {
            auto arrayObj = static_cast<obj::ArrayDouble *>(self.get());
            std::vector<double> values(arrayObj->value);
            std::reverse(values.begin(), values.end());
            return std::make_shared<obj::ArrayDouble>(obj::ArrayDouble(values));
        }
        case obj::ObjectType::ArrayComplex:
        {
            auto arrayObj = static_cast<obj::ArrayComplex *>(self.get());
            std::vector<std::complex<double>> values(arrayObj->value);
            std::reverse(values.begin(), values.end());
            return std::make_shared<obj::ArrayComplex>(obj::ArrayComplex(values));
        }
        }
        return std::make_shared<obj::Error>("Method unavailable for type", obj::ErrorType::TypeError);
    }

    std::shared_ptr<obj::Object> array_rotate(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        if (self->frozen > 0)
            return std::make_shared<obj::Error>("array rotate expects a non-frozen object", obj::ErrorType::TypeError);

        auto errorObj = validateArguments("rotate", self, arguments, 1);
        if (errorObj)
            return errorObj;
        if (arguments[0]->type != obj::ObjectType::Integer)
            return std::make_shared<obj::Error>("rotate: expected argument 1 of type integer", obj::ErrorType::TypeError);
        auto rotation = static_cast<obj::Integer *>(arguments[0].get());
        const int rotationValue = static_cast<const int>(rotation->value);

        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
        {
            auto arrayObj = static_cast<obj::Array *>(self.get());
            std::rotate(arrayObj->value.begin(), arrayObj->value.begin() + rotationValue, arrayObj->value.end());
        }
        case obj::ObjectType::ArrayDouble:
        {
            auto arrayObj = static_cast<obj::ArrayDouble *>(self.get());
            std::rotate(arrayObj->value.begin(), arrayObj->value.begin() + rotationValue, arrayObj->value.end());
        }
        case obj::ObjectType::ArrayComplex:
        {
            auto arrayObj = static_cast<obj::ArrayComplex *>(self.get());
            std::rotate(arrayObj->value.begin(), arrayObj->value.begin() + rotationValue, arrayObj->value.end());
        }
        }
        return self;
    }

    std::shared_ptr<obj::Object> array_rotated(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("rotated", self, arguments, 1);
        if (errorObj)
            return errorObj;
        if (arguments[0]->type != obj::ObjectType::Integer)
            return std::make_shared<obj::Error>("rotate: expected argument 1 of type integer", obj::ErrorType::TypeError);

        auto rotation = static_cast<obj::Integer *>(arguments[0].get());
        // [TODO] should we flag when the rotation->value falls outside the int range, as int64 -> int can be problematic?
        const int rotationValue = static_cast<const int>(rotation->value);
        switch (self.get()->type)
        {
        case obj::ObjectType::Array:
        {
            auto arrayObj = static_cast<obj::Array *>(self.get());
            std::vector<std::shared_ptr<obj::Object>> values(arrayObj->value);
            std::rotate(values.begin(), values.begin() + rotationValue, values.end());
            return std::make_shared<obj::Array>(obj::Array(values));
        }
        case obj::ObjectType::ArrayDouble:
        {
            auto arrayObj = static_cast<obj::ArrayDouble *>(self.get());
            std::vector<double> values(arrayObj->value);
            std::rotate(values.begin(), values.begin() + rotationValue, values.end());
            return std::make_shared<obj::ArrayDouble>(obj::ArrayDouble(values));
        }
        case obj::ObjectType::ArrayComplex:
        {
            auto arrayObj = static_cast<obj::ArrayComplex *>(self.get());
            std::vector<std::complex<double>> values(arrayObj->value);
            std::rotate(values.begin(), values.begin() + rotationValue, values.end());
            return std::make_shared<obj::ArrayComplex>(obj::ArrayComplex(values));
        }
        }
        return std::make_shared<obj::Error>("Method unavailable for type", obj::ErrorType::TypeError);
    }

    std::vector<std::shared_ptr<obj::BuiltinType>> makeBuiltinTypeArrays()
    {
        std::vector<std::shared_ptr<obj::BuiltinType>> arrayTypes;

        typedef obj::TBuiltinTypeFunctionDefinition TBuiltInFD;

        for (const auto &arrayType : {obj::ObjectType::Array, obj::ObjectType::ArrayDouble, obj::ObjectType::ArrayComplex})
        {
            auto arrayBuiltinType = std::make_shared<obj::BuiltinType>();
            arrayBuiltinType->builtinObjectType = arrayType;

            arrayBuiltinType->functions = {
                {"capacity", TBuiltInFD({&builtin::array_capacity, typing::makeFunctionType("", "int")})}, // "[all].fn() -> int"
                {"clear", TBuiltInFD({&builtin::array_clear, typing::makeFunctionType("", "[all]")})},     // "[all].fn() -> [all]"
                {"empty", TBuiltInFD({&builtin::array_empty, typing::makeFunctionType("", "[all]")})},     // "[all].fn() -> [all]"

                {"push_back", TBuiltInFD({&builtin::array_push_back, typing::makeFunctionType("all", "[all]")})}, // "[all].fn(all) -> [all]"
                {"pop_back", TBuiltInFD({&builtin::array_pop_back, typing::makeFunctionType("", "[all]")})},      // "[all].fn() -> [all]"
                {"reserve", TBuiltInFD({&builtin::array_reserve, typing::makeFunctionType("int", "[all]")})},     // "[all].fn(int) -> [all]"
                {"reverse", TBuiltInFD({&builtin::array_reverse, typing::makeFunctionType("", "[all]")})},        // "[all].fn() -> [all]"
                {"reversed", TBuiltInFD({&builtin::array_reversed, typing::makeFunctionType("", "[all]")})},      // "[all].fn() -> [all]"

                {"size", TBuiltInFD({&builtin::array_size, typing::makeFunctionType("", "int")})},           // "[all].fn() -> int"
                {"rotate", TBuiltInFD({&builtin::array_rotate, typing::makeFunctionType("int", "[all]")})},  // "[all].fn(int) -> [all]"
                {"rotated", TBuiltInFD({&builtin::array_rotated, typing::makeFunctionType("int", "[all]")})} // "[all].fn(int) -> [all]"
            };

            arrayTypes.push_back(std::move(arrayBuiltinType));
        }

        return arrayTypes;
    }

}