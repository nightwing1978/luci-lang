/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Dictionary.h"
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
    std::shared_ptr<Object> Dictionary::clone() const
    {
        TDictionaryMap values;
        for (const auto &[k, v] : value)
            values.insert(std::make_pair(k->clone(), v->clone()));
        return std::make_shared<obj::Dictionary>(values);
    };

    std::string Dictionary::inspect() const
    {
        std::stringstream ss;
        std::vector<std::string> pairs;
        for (const auto &element : value)
            pairs.push_back(util::join({element.first->inspect(), element.second->inspect()}, ":"));

        ss << "{";
        ss << util::join(pairs, ", ");
        ss << "}";
        return ss.str();
    }

    bool Dictionary::hashAble() const
    {
        if (frozen <= 0)
            return false;
        for (const auto &[k, v] : value)
        {
            if (!v->hashAble())
                return false;
            if (!k->hashAble())
                return false;
        }
        return true;
    };

    std::size_t Dictionary::hash() const
    {
        if (frozen <= 0)
            throw std::runtime_error("Cannot hash non-frozen dictionary");

        std::size_t hashValue = 0;
        for (const auto &[k, v] : value)
        {
            if (!k->hashAble())
                throw std::runtime_error("Non-hashable key in dictionary");
            if (!v->hashAble())
                throw std::runtime_error("Non-hashable value in dictionary");
            hashValue ^= k->hash() ^ v->hash();
        }
        return hashValue;
    };

    bool Dictionary::eq(const Object *other) const
    {
        if (other->type != obj::ObjectType::Dictionary)
            return false;

        const Dictionary *otherDict = static_cast<const obj::Dictionary *>(other);

        if (value.size() != otherDict->value.size())
            return false;

        for (const auto &[k, v] : value)
        {
            auto otherV = otherDict->value.find(v);
            if (otherV == otherDict->value.end())
                return false;

            if (otherV->second->type != v->type)
                return false;

            if (!otherV->second->eq(v.get()))
                return false;
        }

        return true;
    };

    Dictionary::Dictionary(const TDictionaryMap &ivalue) : Object(ObjectType::Dictionary), value(ivalue){};

    std::string DictionaryIterator::inspect() const
    {
        return "DictionaryIterator()";
    }

    std::shared_ptr<Object> DictionaryIterator::clone() const
    {
        return std::make_shared<DictionaryIterator>(dict, iterator);
    }

    bool DictionaryIterator::isValid() const
    {
        if (!dict)
            return false;
        return iterator != dict->value.end();
    };

    std::shared_ptr<Object> DictionaryIterator::next()
    {
        if (isValid())
        {
            auto value = iterator->first;
            ++iterator;
            return value;
        }
        return std::make_shared<obj::Error>("next referencing invalid iterator", obj::ErrorType::TypeError);
    }

    DictionaryIterator::DictionaryIterator(std::shared_ptr<Dictionary> idict, TDictionaryMap::iterator iiterator) : Iterator(), dict(idict), freezer(idict), iterator(iiterator){};
}

namespace builtin
{
    std::shared_ptr<obj::Object> dictionary_size(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("size", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;
        return std::make_shared<obj::Integer>(static_cast<obj::Dictionary *>(self.get())->value.size());
    }

    std::shared_ptr<obj::Object> dictionary_clear(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("clear", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;

        static_cast<obj::Dictionary *>(self.get())->value.clear();
        return self;
    }

    std::shared_ptr<obj::Object> dictionary_empty(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("empty", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;

        return std::make_shared<obj::Boolean>(static_cast<obj::Dictionary *>(self.get())->value.empty());
    }

    std::shared_ptr<obj::Object> dictionary_keys(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("keys", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;

        auto dictObj = static_cast<const obj::Dictionary *>(self.get());
        std::vector<std::shared_ptr<obj::Object>> values;
        for (auto it : dictObj->value)
            values.push_back(it.first);
        return std::make_shared<obj::Array>(obj::Array(values));
    }

    std::shared_ptr<obj::Object> dictionary_values(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("values", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;

        auto dictObj = static_cast<const obj::Dictionary *>(self.get());
        std::vector<std::shared_ptr<obj::Object>> values;
        for (auto it : dictObj->value)
            values.push_back(it.second);
        return std::make_shared<obj::Array>(obj::Array(values));
    }

    std::shared_ptr<obj::Object> dictionary_items(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("items", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;

        auto dictObj = static_cast<const obj::Dictionary *>(self.get());
        std::vector<std::shared_ptr<obj::Object>> values;
        for (auto it : dictObj->value)
        {
            std::vector<std::shared_ptr<obj::Object>> keyValuePair;
            keyValuePair.push_back(it.first);
            keyValuePair.push_back(it.second);
            auto keyValueObj = std::make_shared<obj::Array>(keyValuePair);
            values.push_back(keyValueObj);
        }
        return std::make_shared<obj::Array>(obj::Array(values));
    }

    std::shared_ptr<obj::Object> dictionary_update(const std::shared_ptr<obj::Object> &self, const std::vector<std::shared_ptr<obj::Object>> &arguments)
    {
        auto errorObj = validateArguments("update", self, arguments, obj::ObjectType::Dictionary, 1);
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::Dictionary)
            return std::make_shared<obj::Error>("update: expected argument 1 of type dictionary", obj::ErrorType::TypeError);
        auto otherDictionary = static_cast<obj::Dictionary *>(arguments[0].get());

        auto dictObj = static_cast<obj::Dictionary *>(self.get());
        for (auto it : otherDictionary->value)
        {
            dictObj->value.insert_or_assign(it.first, it.second);
        }
        return self;
    }

    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeDictionary()
    {
        typedef obj::TBuiltinTypeFunctionDefinition TBuiltInFD;

        auto dictionaryBuiltinType = std::make_shared<obj::BuiltinType>();
        dictionaryBuiltinType->builtinObjectType = obj::ObjectType::Dictionary;

        dictionaryBuiltinType->functions = {
            {"clear", TBuiltInFD({&builtin::dictionary_clear, typing::makeFunctionType("", "{all:all}")})}, // "{all:all}.fn() -> {all:all}"
            {"empty", TBuiltInFD({&builtin::dictionary_empty, typing::makeFunctionType("", "{all:all}")})}, // "{all:all}.fn() -> {all:all}"
            {"size", TBuiltInFD({&builtin::dictionary_size, typing::makeFunctionType("", "int")})},         // "{all:all}.fn() -> int"

            {"keys", TBuiltInFD({&builtin::dictionary_keys, typing::makeFunctionType("", "[all]")})},     // "{all:all}.fn() -> [all]"
            {"values", TBuiltInFD({&builtin::dictionary_values, typing::makeFunctionType("", "[all]")})}, // "{all:all}.fn() -> [all]""
            {"items", TBuiltInFD({&builtin::dictionary_items, typing::makeFunctionType("", "[[all]]")})}, // "{all:all}.fn() -> [[all,all]]"

            {"update", TBuiltInFD({&builtin::dictionary_update, typing::makeFunctionType("{all:all}", "{all:all}")})} // "{all:all}.fn({all:all}) -> {all:all}.fn()"
        };

        return dictionaryBuiltinType;
    }
}