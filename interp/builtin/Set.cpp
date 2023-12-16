/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Set.h"
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
        size_t nrExpectedArguments)
    {
        if (self.get()->type != expectedType)
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + toString(expectedType) + ", got " + toString(self.get()->type), obj::ErrorType::TypeError);
        if (arguments.size() != nrExpectedArguments)
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + std::to_string(nrExpectedArguments) + " arguments, got " + std::to_string(arguments.size()), obj::ErrorType::TypeError);
        return nullptr;
    }
}

namespace obj
{
    std::shared_ptr<Object> Set::clone() const
    {
        TSetSet values;
        for (const auto &k : value)
            values.insert(k->clone());
        return std::make_shared<obj::Set>(values);
    };

    bool Set::hashAble() const
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

    std::string Set::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> elements;
        for (const auto &element : value)
            elements.push_back(element->inspect());

        ss << "{";
        ss << util::join(elements, ", ");
        ss << "}";
        return ss.str();
    }

    std::size_t Set::hash() const
    {
        if (frozen <= 0)
            throw std::runtime_error("Cannot hash non-frozen set");

        std::size_t hashValue = 0;
        for (const auto &v : value)
        {
            if (!v->hashAble())
                throw std::runtime_error("Non-hashable item in set");
            hashValue ^= v->hash();
        }
        return hashValue;
    };

    bool Set::eq(const Object *other) const
    {
        if (other->type != obj::ObjectType::Set)
            return false;

        const Set *otherSet = static_cast<const obj::Set *>(other);

        if (value.size() != otherSet->value.size())
            return false;

        for (const auto &v : value)
        {
            if (otherSet->value.find(v) == otherSet->value.end())
                return false;
        }

        return true;
    };

    Set::Set(const TSetSet &ivalue) : Object(ObjectType::Set), value(ivalue){};

    std::string SetIterator::inspect() const
    {
        return "SetIterator()";
    }

    std::shared_ptr<Object> SetIterator::clone() const
    {
        return std::make_shared<SetIterator>(setObj, iterator);
    }

    bool SetIterator::isValid() const
    {
        if (!setObj)
            return false;
        return iterator != setObj->value.end();
    };

    std::shared_ptr<Object> SetIterator::next()
    {
        if (isValid())
        {
            auto value = *iterator;
            ++iterator;
            return value;
        }
        return std::make_shared<obj::Error>("next referencing invalid iterator", obj::ErrorType::TypeError);
    }

    SetIterator::SetIterator(std::shared_ptr<Set> iset, TSetSet::iterator iiterator) : Iterator(), setObj(iset), freezer(iset), iterator(iiterator){};
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