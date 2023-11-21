#include "Dictionary.h"
#include "../Typing.h"

namespace
{
    std::shared_ptr<obj::Object> 
    validateArguments(
        const std::string& errorPrefix, 
        const std::shared_ptr<obj::Object>& self,
        const std::vector<std::shared_ptr<obj::Object> >& arguments,
        const obj::ObjectType expectedType,
        size_t nrExpectedArguments
        )
    {
        if ( self.get()->type != expectedType )
            return std::make_shared<obj::Error>(errorPrefix + ": expected " + toString(expectedType) + ", got " + toString(self.get()->type));
		if (arguments.size()!=nrExpectedArguments)
			return std::make_shared<obj::Error>(errorPrefix + ": expected " + std::to_string(nrExpectedArguments) + " arguments, got " + std::to_string(arguments.size()));
        return nullptr;
    }
}

namespace builtin
{
    std::shared_ptr<obj::Object> dictionary_size(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments)
	{
        auto errorObj = validateArguments("size", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;
        return std::make_shared<obj::Integer>(static_cast<obj::Dictionary*>(self.get())->value.size());
	}


    std::shared_ptr<obj::Object> dictionary_clear(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments)
	{
        auto errorObj = validateArguments("clear", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;
    
        static_cast<obj::Dictionary*>(self.get())->value.clear();
        return self;
	}


    std::shared_ptr<obj::Object> dictionary_empty(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments)
	{
        auto errorObj = validateArguments("empty", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;
    
        return std::make_shared<obj::Boolean>(static_cast<obj::Dictionary*>(self.get())->value.empty());
	}


    std::shared_ptr<obj::Object> dictionary_keys(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments)
	{
        auto errorObj = validateArguments("keys", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;
    
        auto dictObj = static_cast<const obj::Dictionary*>(self.get());
        std::vector< std::shared_ptr< obj::Object > > values;
        for (auto it : dictObj->value)
            values.push_back(it.first);
        return std::make_shared<obj::Array>(obj::Array(values));
	}


    std::shared_ptr<obj::Object> dictionary_values(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments)
	{
        auto errorObj = validateArguments("values", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;
    
        auto dictObj = static_cast<const obj::Dictionary*>(self.get());
        std::vector< std::shared_ptr< obj::Object > > values;
        for (auto it : dictObj->value)
            values.push_back(it.second);
        return std::make_shared<obj::Array>(obj::Array(values));
	}


    std::shared_ptr<obj::Object> dictionary_items(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments)
	{
        auto errorObj = validateArguments("items", self, arguments, obj::ObjectType::Dictionary, 0);
        if (errorObj)
            return errorObj;
    
        auto dictObj = static_cast<const obj::Dictionary*>(self.get());
        std::vector< std::shared_ptr< obj::Object > > values;
        for (auto it : dictObj->value)
        {
            std::vector< std::shared_ptr< obj::Object > > keyValuePair;
            keyValuePair.push_back(it.first);
            keyValuePair.push_back(it.second);
            auto keyValueObj = std::make_shared<obj::Array>(keyValuePair);
            values.push_back(keyValueObj);
        }
        return std::make_shared<obj::Array>(obj::Array(values));
	}


    std::shared_ptr<obj::Object> dictionary_update(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments)
	{
        auto errorObj = validateArguments("update", self, arguments, obj::ObjectType::Dictionary, 1);
        if (errorObj)
            return errorObj;

        if (arguments[0]->type != obj::ObjectType::Dictionary)
            return std::make_shared<obj::Error>("update: expected argument 1 of type dictionary");
        auto otherDictionary = static_cast<obj::Dictionary*>(arguments[0].get());

        auto dictObj = static_cast<obj::Dictionary*>(self.get());
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
            { "clear", TBuiltInFD({&builtin::dictionary_clear, typing::makeFunctionType("", "{all:all}")})},                // "{all:all}.fn() -> {all:all}"
            { "empty", TBuiltInFD({&builtin::dictionary_empty, typing::makeFunctionType("", "{all:all}")})},                // "{all:all}.fn() -> {all:all}"
            { "size", TBuiltInFD({&builtin::dictionary_size, typing::makeFunctionType("", "int")})},                        // "{all:all}.fn() -> int"

            { "keys", TBuiltInFD({&builtin::dictionary_keys, typing::makeFunctionType("", "[all]")})},                      // "{all:all}.fn() -> [all]"
            { "values", TBuiltInFD({&builtin::dictionary_values, typing::makeFunctionType("", "[all]")})},                  // "{all:all}.fn() -> [all]""
            { "items", TBuiltInFD({&builtin::dictionary_items, typing::makeFunctionType("", "[[all]]")})},                  // "{all:all}.fn() -> [[all,all]]"

            { "update", TBuiltInFD({&builtin::dictionary_update, typing::makeFunctionType("{all:all}", "{all:all}")})}      // "{all:all}.fn({all:all}) -> {all:all}.fn()"
        };

        return dictionaryBuiltinType;
    }
}