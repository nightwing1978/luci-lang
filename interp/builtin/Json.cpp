/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Json.h"
#include "../Evaluator.h"
#include "../Typing.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <iomanip>

namespace
{
    enum class JsonValueType
    {
        ARRAY,
        BOOLEAN,
        NULL_VALUE,
        DOUBLE,
        INT,
        OBJECT,
        STRING,
    };

    struct JsonValue;

    using JsonObject = std::map<std::string, JsonValue>;
    using JsonArray = std::vector<JsonValue>;

    struct JsonValue
    {
        JsonValueType type;
        std::string strValue;
        double numValue = 0.0;
        int64_t intValue = 0;
        JsonObject objValue;
        JsonArray arrValue;
        bool boolValue = false;

        JsonValue(const std::string &value) : type(JsonValueType::STRING), strValue(value) {}
        JsonValue(double value) : type(JsonValueType::DOUBLE), numValue(value) {}
        JsonValue(int64_t value) : type(JsonValueType::INT), intValue(value) {}
        JsonValue(const JsonObject &value) : type(JsonValueType::OBJECT), objValue(value) {}
        JsonValue(const JsonArray &value) : type(JsonValueType::ARRAY), arrValue(value) {}
        JsonValue(bool value) : type(JsonValueType::BOOLEAN), boolValue(value) {}
        JsonValue() : type(JsonValueType::NULL_VALUE) {}

        std::string serialize(int indentation = 0) const
        {
            std::ostringstream result;

            switch (type)
            {
            case JsonValueType::STRING:
                result << '"' << strValue << '"';
                break;
            case JsonValueType::INT:
                result << intValue;
                break;
            case JsonValueType::DOUBLE:
                result << std::fixed << std::setprecision(17) << numValue;
                break;
            case JsonValueType::OBJECT:
                result << "{";
                if (!objValue.empty())
                {
                    result << "\n";
                    for (const auto &pair : objValue)
                    {
                        result << std::string(indentation + 4, ' '); // 4 spaces per level
                        result << '"' << pair.first << "\": " << pair.second.serialize(indentation + 4) << ",\n";
                    }
                    result.seekp(-2, std::ios_base::end); // Remove the trailing comma and newline
                    result << "\n";
                    result << std::string(indentation, ' '); // Indentation for the closing brace
                }
                result << "}";
                break;
            case JsonValueType::ARRAY:
                result << "[";
                if (!arrValue.empty())
                {
                    result << "\n";
                    for (const auto &element : arrValue)
                    {
                        result << std::string(indentation + 4, ' '); // 4 spaces per level
                        result << element.serialize(indentation + 4) << ",\n";
                    }
                    result.seekp(-2, std::ios_base::end); // Remove the trailing comma and newline
                    result << "\n";
                    result << std::string(indentation, ' '); // Indentation for the closing bracket
                }
                result << "]";
                break;
            case JsonValueType::BOOLEAN:
                result << (boolValue ? "true" : "false");
                break;
            case JsonValueType::NULL_VALUE:
                result << "null";
                break;
            }

            return result.str();
        };
    };

    class JsonParser
    {
    public:
        JsonParser(const std::string &json) : json_(json), index_(0) {}

        JsonValue parse()
        {
            skipWhitespace();
            return parseValue();
        }

    private:
        std::string json_;
        size_t index_;

        char getCurrentChar() const
        {
            return (index_ < json_.size()) ? json_[index_] : '\0';
        }

        static bool isWhitespace(char val)
        {
            return val == ' ' || val == '\t' || val == '\n' || val == '\r';
        }

        void skipWhitespace()
        {

            while (isWhitespace(getCurrentChar()))
                ++index_;
        }

        std::string parseString()
        {
            std::string result;
            ++index_; // Skip the opening quote

            while (getCurrentChar() != '"')
            {
                if (getCurrentChar() == '\\')
                {
                    ++index_;
                    char escapeChar = getCurrentChar();

                    switch (escapeChar)
                    {
                    case '"':
                    case '\\':
                    case '/':
                        result += escapeChar;
                        break;
                    case 'b':
                        result += '\b';
                        break;
                    case 'f':
                        result += '\f';
                        break;
                    case 'n':
                        result += '\n';
                        break;
                    case 'r':
                        result += '\r';
                        break;
                    case 't':
                        result += '\t';
                        break;
                    default:
                        result += '\\';
                        result += escapeChar;
                        break;
                    }
                }
                else
                {
                    result += getCurrentChar();
                }

                ++index_;
            }

            ++index_; // Skip the closing quote
            return result;
        }

        JsonValue parseNumber()
        {
            size_t end;
            std::string numberStr = json_.substr(index_);
            double result = std::stod(numberStr, &end);

            std::string extractedString = numberStr.substr(0, end);
            if (extractedString.find('.') == std::string::npos && extractedString.find('e') == std::string::npos && extractedString.find('E') == std::string::npos)
            {
                // No decimal point or exponent notation, consider it an integer
                int64_t intResult = static_cast<int64_t>(result);
                index_ += end;
                return intResult;
            }

            index_ += end;
            return result;
        }

        JsonValue parseObject()
        {
            JsonObject obj;
            ++index_; // Skip the opening brace

            while (getCurrentChar() != '}')
            {
                skipWhitespace();

                std::string key = parseString();

                skipWhitespace();
                if (getCurrentChar() != ':')
                {
                    throw std::runtime_error("Expected ':' in object");
                }
                ++index_; // Skip the colon

                JsonValue value = parseValue();
                obj[key] = value;

                skipWhitespace();
                if (getCurrentChar() == ',')
                {
                    ++index_; // Skip the comma
                }
            }

            ++index_; // Skip the closing brace
            return obj;
        }

        JsonValue parseArray()
        {
            JsonArray arr;
            ++index_; // Skip the opening bracket

            while (getCurrentChar() != ']')
            {
                skipWhitespace();
                JsonValue value = parseValue();
                arr.push_back(value);

                skipWhitespace();
                if (getCurrentChar() == ',')
                {
                    ++index_; // Skip the comma
                }
            }

            ++index_; // Skip the closing bracket
            return arr;
        }

        JsonValue parseValue()
        {
            skipWhitespace();

            char currentChar = getCurrentChar();
            switch (currentChar)
            {
            case '"':
                return parseString();
            case '{':
                return parseObject();
            case '[':
                return parseArray();
            case 't':
                index_ += 4; // skip and check for true
                return true;
            case 'f':
                index_ += 5; // skip and check for false
                return false;
            case 'n':
                index_ += 4; // skip and check for null
                return JsonValue();
            case '-':
                return parseNumber();
            default:
                if (std::isdigit(currentChar))
                {
                    return parseNumber();
                }
                else
                    throw std::runtime_error("Unexpected character in JSON");
            }
        }
    };

    JsonValue objectToJsonValue(std::shared_ptr<obj::Object> value)
    {
        if (!value)
            return JsonValue();

        switch (value->type)
        {
        case obj::ObjectType::Array:
        {
            std::vector<JsonValue> values;
            auto array = static_cast<obj::Array *>(value.get());
            for (const auto &element : array->value)
                values.push_back(objectToJsonValue(element));
            return JsonValue(values);
        }
        case obj::ObjectType::ArrayDouble:
        {
            std::vector<JsonValue> values;
            auto array = static_cast<obj::ArrayDouble *>(value.get());
            for (const auto &element : array->value)
                values.push_back(JsonValue(element));
            return JsonValue(values);
        }
        break;
        case obj::ObjectType::Boolean:
            return JsonValue(static_cast<obj::Boolean *>(value.get())->value);
        case obj::ObjectType::Null:
            return JsonValue();
        case obj::ObjectType::Double:
            return JsonValue(static_cast<obj::Double *>(value.get())->value);
        case obj::ObjectType::Integer:
            return JsonValue(static_cast<obj::Integer *>(value.get())->value);
        case obj::ObjectType::Dictionary:
        {
            JsonObject jsonObjectDict;
            for (const auto &element : static_cast<obj::Dictionary *>(value.get())->value)
            {
                if (element.first->type != obj::ObjectType::String)
                    throw std::runtime_error("Cannot convert a non-string key of dictionary to JSON type");
                std::string key = static_cast<obj::String *>(element.first.get())->value;
                JsonValue mapValue = objectToJsonValue(element.second);
                jsonObjectDict[key] = mapValue;
            }
            return JsonValue(jsonObjectDict);
        }
        case obj::ObjectType::String:
            return JsonValue(static_cast<obj::String *>(value.get())->value);
        default:
            throw std::runtime_error("Cannot convert object to JSON");
        }
    }

    std::shared_ptr<obj::Object> jsonValueToObject(const JsonValue &value)
    {
        switch (value.type)
        {
        case JsonValueType::ARRAY:
        {
            std::vector<std::shared_ptr<obj::Object>> objects;
            objects.reserve(value.arrValue.size());
            for (const auto &element : value.arrValue)
                objects.push_back(jsonValueToObject(element));
            return std::make_shared<obj::Array>(objects);
        }
        break;
        case JsonValueType::BOOLEAN:
            return std::make_shared<obj::Boolean>(value.boolValue);
        case JsonValueType::NULL_VALUE:
            return NullObject;
        case JsonValueType::DOUBLE:
            return std::make_shared<obj::Double>(value.numValue);
        case JsonValueType::INT:
            return std::make_shared<obj::Integer>(value.intValue);
        case JsonValueType::OBJECT:
        {
            obj::TDictionaryMap objectMap;
            for (const auto &element : value.objValue)
                objectMap.insert_or_assign(jsonValueToObject(element.first), jsonValueToObject(element.second));
            return std::make_shared<obj::Dictionary>(objectMap);
        }
        break;
        case JsonValueType::STRING:
            return std::make_shared<obj::String>(value.strValue);
        }

        return NullObject;
    }
}

namespace builtin
{
    std::shared_ptr<obj::Object> jsonLoad(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("load: expected 1 arguments of type str", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        RETURN_TYPE_ERROR_ON_MISMATCH(evaluatedExpr1, String, "load: expected argument 1 to be a str");

        std::string arg = static_cast<obj::String *>(evaluatedExpr1.get())->value;

        JsonParser parser(arg);
        try
        {
            JsonValue value = parser.parse();
            return jsonValueToObject(value);
        }
        catch (std::runtime_error & /*e*/)
        {
            return std::make_shared<obj::Error>("Failed to parse JSON", obj::ErrorType::ValueError);
        }

        return NullObject;
    }

    std::shared_ptr<obj::Object> jsonDump(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return std::make_shared<obj::Error>("load: expected 1 argument", obj::ErrorType::TypeError);

        auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr1->type == obj::ObjectType::Error)
            return evaluatedExpr1;

        try
        {
            JsonValue value = objectToJsonValue(evaluatedExpr1);
            return std::make_shared<obj::String>(value.serialize());
        }
        catch (std::runtime_error & /*e*/)
        {
            return std::make_shared<obj::Error>("Failed to generate JSON", obj::ErrorType::ValueError);
        }
        return NullObject;
    }

    std::shared_ptr<obj::Module> createJsonModule()
    {
        auto jsonModule = std::make_shared<obj::Module>();
        jsonModule->environment->add("load", builtin::makeBuiltInFunctionObj(&builtin::jsonLoad, "str", "all"), false, nullptr);
        jsonModule->environment->add("dump", builtin::makeBuiltInFunctionObj(&builtin::jsonDump, "all", "str"), false, nullptr);
        jsonModule->state = obj::ModuleState::Loaded;
        return jsonModule;
    }
} // namespace builtin
