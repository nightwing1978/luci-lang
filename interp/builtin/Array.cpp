/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Array.h"
#include "../Typing.h"
#include "../Util.h"
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

namespace obj
{
    std::shared_ptr<Object> ArrayDouble::valueConstruct(const double &value)
    {
        return std::make_shared<Double>(value);
    }

    std::string ArrayDouble::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> elements;
        for (const auto &element : value)
            elements.push_back(std::to_string(element));

        ss << "[";
        ss << util::join(elements, ", ");
        ss << "]";
        return ss.str();
    }

    std::shared_ptr<Object> ArrayDouble::clone() const
    {
        return std::make_shared<obj::ArrayDouble>(value);
    };

    bool ArrayDouble::eq(const Object *other) const
    {
        return static_cast<const obj::ArrayDouble *>(other)->value == value;
    };

    ArrayDouble::ArrayDouble(const std::vector<double> &ivalue) : Object(ObjectType::ArrayDouble), value(ivalue){};

    std::shared_ptr<Object> ArrayComplex::valueConstruct(const std::complex<double> &value)
    {
        return std::make_shared<Complex>(value);
    }

    std::string ArrayComplex::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> elements;
        for (const auto &element : value)
            elements.push_back(std::to_string(element.real()) + "+" + std::to_string(element.imag()) + "j");

        ss << "[";
        ss << util::join(elements, ", ");
        ss << "]";
        return ss.str();
    }

    std::shared_ptr<Object> ArrayComplex::clone() const
    {
        return std::make_shared<ArrayComplex>(value);
    };

    bool ArrayComplex::eq(const Object *other) const
    {
        return static_cast<const ArrayComplex *>(other)->value == value;
    };

    ArrayComplex::ArrayComplex(const std::vector<std::complex<double>> &ivalue) : Object(ObjectType::ArrayComplex), value(ivalue){};

    std::shared_ptr<obj::Object> Array::valueConstruct(std::shared_ptr<Object> obj)
    {
        return obj;
    }

    std::string Array::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> elements;
        for (const auto &element : value)
        {
            if (element)
                elements.push_back(element->inspect());
            else
                elements.push_back("<Null>");
        }

        ss << "[";
        ss << util::join(elements, ", ");
        ss << "]";
        return ss.str();
    }

    std::shared_ptr<Object> Array::clone() const
    {
        std::vector<std::shared_ptr<Object>> values;
        for (const auto &v : value)
            values.push_back(v->clone());
        return std::make_shared<obj::Array>(values);
    };

    bool Array::eq(const Object *other) const
    {
        if (other->type != obj::ObjectType::Array)
            return false;

        const obj::Array *otherArray = static_cast<const obj::Array *>(other);
        if (value.size() != otherArray->value.size())
            return false;

        for (size_t i = 0; i < value.size(); ++i)
        {
            if (value[i]->type != otherArray->value[i]->type)
                return false;

            if (!value[i]->eq(otherArray->value[i].get()))
                return false;
        }
        return true;
    };

    bool Array::hashAble() const
    {
        if (frozen <= 0)
            return false;
        for (const auto &v : value)
        {
            if (!v->hashAble())
                return false;
        }
        return true;
    };

    std::size_t Array::hash() const
    {
        if (frozen <= 0)
            throw std::runtime_error("Cannot hash non-frozen array");

        std::size_t hashValue = 0;
        for (const auto &v : value)
        {
            if (!v->hashAble())
                throw std::runtime_error("Non-hashable item in array");
            hashValue ^= v->hash();
        }
        return hashValue;
    };

    Array::Array(const std::vector<std::shared_ptr<Object>> &ivalue) : Object(ObjectType::Array), value(ivalue){};

    Array::Array(const std::vector<double> &ivalue) : Object(ObjectType::Array)
    {
        value.reserve(ivalue.size() + 1);
        for (const auto &element : ivalue)
            value.push_back(std::make_shared<obj::Double>(element));
    };

    Array::Array(const std::vector<std::complex<double>> &ivalue) : Object(ObjectType::Array)
    {
        value.reserve(ivalue.size() + 1);
        for (const auto &element : ivalue)
            value.push_back(std::make_shared<obj::Complex>(element));
    };

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