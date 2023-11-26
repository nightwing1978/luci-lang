#include "Typing.h"
#include "Lexer.h"
#include "Parser.h"

namespace typing
{
    bool CompareTypes::operator()(ast::TypeExpression *a, ast::TypeExpression *b) const
    {
        if (a == b)
            return true;

        if (a->type != b->type)
            return false;

        return a->text() < b->text();
    };

    std::unique_ptr<ast::TypeExpression> computeType(obj::Object *obj)
    {
        if (obj->declaredType)
            return obj->declaredType->clone();

        switch (obj->type)
        {
        case obj::ObjectType::Null:
        case obj::ObjectType::Integer:
        case obj::ObjectType::Double:
        case obj::ObjectType::Complex:
        case obj::ObjectType::Boolean:
        case obj::ObjectType::String:
        case obj::ObjectType::Error:
        case obj::ObjectType::Range:
        {
            std::map<obj::ObjectType, std::string> builtInRevTypeMapping = {
                {obj::ObjectType::Null, "null"},
                {obj::ObjectType::Integer, "int"},
                {obj::ObjectType::Complex, "complex"},
                {obj::ObjectType::Double, "double"},
                {obj::ObjectType::Boolean, "bool"},
                {obj::ObjectType::String, "str"},
                {obj::ObjectType::Error, "error"},
                {obj::ObjectType::IOObject, "io"},
                {obj::ObjectType::Module, "module"},
                {obj::ObjectType::Thread, "thread"},
                {obj::ObjectType::Range, "range"},
            };
            auto typeIdentifier = std::make_unique<ast::TypeIdentifier>();
            typeIdentifier->value = builtInRevTypeMapping.at(obj->type);
            return std::move(typeIdentifier);
        }
        case obj::ObjectType::Array:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> elementTypes;
            auto array = static_cast<obj::Array *>(obj);
            for (const auto &element : array->value)
            {
                auto typeOfElement = std::move(computeType(element.get()));
                elementTypes.insert_or_assign(typeOfElement.get(), std::move(typeOfElement));
            }
            auto typeArray = std::make_unique<ast::TypeArray>();
            if (elementTypes.size() == 1)
            {
                typeArray->elementType = std::move(elementTypes.begin()->second);
            }
            else
            {
                auto typeChoice = std::make_unique<ast::TypeChoice>();
                for (auto &[k, v] : elementTypes)
                    typeChoice->choices.push_back(std::move(v));
                typeArray->elementType = std::move(typeChoice);
            }
            return std::move(typeArray);
        }
        case obj::ObjectType::ArrayDouble:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> elementTypes;
            auto typeArray = std::make_unique<ast::TypeArray>();
            auto doubleTypeIndentifier = std::make_unique<ast::TypeIdentifier>();
            doubleTypeIndentifier->value = "double";
            typeArray->elementType = std::move(doubleTypeIndentifier);
            return std::move(typeArray);
        }
        case obj::ObjectType::ArrayComplex:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> elementTypes;
            auto typeArray = std::make_unique<ast::TypeArray>();
            auto doubleTypeIndentifier = std::make_unique<ast::TypeIdentifier>();
            doubleTypeIndentifier->value = "complex";
            typeArray->elementType = std::move(doubleTypeIndentifier);
            return std::move(typeArray);
        }
        case obj::ObjectType::Dictionary:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> keyElementTypes;
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> valueElementTypes;
            auto dict = static_cast<obj::Dictionary *>(obj);
            for (const auto &[k, v] : dict->value)
            {
                auto keyTypeOfElement = std::move(computeType(k.get()));
                auto valueTypeOfElement = std::move(computeType(v.get()));
                keyElementTypes.insert_or_assign(keyTypeOfElement.get(), std::move(keyTypeOfElement));
                valueElementTypes.insert_or_assign(valueTypeOfElement.get(), std::move(valueTypeOfElement));
            }
            auto typeDict = std::make_unique<ast::TypeDictionary>();
            if (keyElementTypes.size() == 1)
            {
                typeDict->keyType = std::move(keyElementTypes.begin()->second);
            }
            else
            {
                auto typeChoice = std::make_unique<ast::TypeChoice>();
                for (auto &[k, v] : keyElementTypes)
                    typeChoice->choices.push_back(std::move(v));
                typeDict->keyType = std::move(typeChoice);
            }
            if (valueElementTypes.size() == 1)
            {
                typeDict->valueType = std::move(valueElementTypes.begin()->second);
            }
            else
            {
                auto typeChoice = std::make_unique<ast::TypeChoice>();
                for (auto &[k, v] : valueElementTypes)
                    typeChoice->choices.push_back(std::move(v));
                typeDict->valueType = std::move(typeChoice);
            }
            return std::move(typeDict);
        }
        case obj::ObjectType::Set:
        {
            std::map<ast::TypeExpression *, std::unique_ptr<ast::TypeExpression>, CompareTypes> elementTypes;
            auto setObj = static_cast<obj::Set *>(obj);
            for (const auto &element : setObj->value)
            {
                auto typeOfElement = std::move(computeType(element.get()));
                elementTypes.insert_or_assign(typeOfElement.get(), std::move(typeOfElement));
            }
            auto typeSet = std::make_unique<ast::TypeSet>();
            if (elementTypes.size() == 1)
            {
                typeSet->elementType = std::move(elementTypes.begin()->second);
            }
            else
            {
                auto typeChoice = std::make_unique<ast::TypeChoice>();
                for (auto &[k, v] : elementTypes)
                    typeChoice->choices.push_back(std::move(v));
                typeSet->elementType = std::move(typeChoice);
            }
            return std::move(typeSet);
        }
        case obj::ObjectType::Function:
        {
            auto typeFunction = std::make_unique<ast::TypeFunction>();
            auto function = static_cast<obj::Function *>(obj);
            for (const auto &argType : function->argumentTypes)
                typeFunction->argTypes.push_back(argType->clone());

            typeFunction->returnType = function->returnType->clone();
            return std::move(typeFunction);
        }
        case obj::ObjectType::UserType:
        {
            auto typeType = std::make_unique<ast::TypeType>();
            auto userType = static_cast<obj::UserType *>(obj);
            typeType->value = userType->name;
            return std::move(typeType);
        }
        case obj::ObjectType::UserObject:
        {
            auto typeIdentifier = std::make_unique<ast::TypeIdentifier>();
            auto userObj = static_cast<obj::UserObject *>(obj);
            typeIdentifier->value = userObj->userType->name;
            return std::move(typeIdentifier);
        }
        case obj::ObjectType::BoundUserTypeFunction:
        {
            // [TODO] introduce a compound type that can convey the boundedness to a type
            auto typeFunction = std::make_unique<ast::TypeFunction>();
            auto function = static_cast<obj::BoundUserTypeFunction *>(obj);
            for (const auto &argType : function->function->argumentTypes)
                typeFunction->argTypes.push_back(argType->clone());

            typeFunction->returnType = function->function->returnType->clone();

            auto boundType = std::make_unique<ast::BoundType>();
            boundType->boundTo = computeType(function->boundTo.get());
            boundType->boundType = std::move(typeFunction);
            return std::move(boundType);
        }
        case obj::ObjectType::BoundUserTypeProperty:
        {
            // [TODO] introduce a compound type that can convey the boundedness to a type
            auto property = static_cast<obj::BoundUserTypeProperty *>(obj);
            return computeType(property->property->obj.get());
        }
        case obj::ObjectType::BoundBuiltinTypeFunction:
        {
            auto function = static_cast<obj::BoundBuiltinTypeFunction *>(obj);
            auto boundType = std::make_unique<ast::BoundType>();
            boundType->boundTo = computeType(function->boundTo.get());
            boundType->boundType = function->functionType->clone();
            return std::move(boundType);
        }
        case obj::ObjectType::BoundBuiltinTypeProperty:
        {
            // [TODO] introduce a compound type that can convey the boundedness to a type
            auto property = static_cast<obj::BoundBuiltinTypeProperty *>(obj);
            return computeType(property->property->obj.get());
        }
        }
        return std::unique_ptr<ast::TypeExpression>(nullptr);
    }

    /* returns true when type1 is compatible with type2
     *  In general a type1 is compatible with type2 if it is a subset of type2
     *   For instance
     *       isCompatible( "double", "double") -> True
     *       isCompatible( "double", "all") -> True
     *       isCompatible( "all", "double") -> False         all is not a subset of double
     *       isCompatible( "any", "double") -> True          any can be a subset of double
     *       isCompatible( "any", "any") -> True             any is a subset of any
     *       isCompatible( "double", "any") -> True          double is a subset of any
     *       isCompatible( "any", "all") -> True             any is a subset of all
     *       isCompatible( "all", "any") -> False            all is not a subset of any
     */
    bool isCompatibleType(ast::TypeExpression *type1, ast::TypeExpression *type2)
    {
        if (type1 == nullptr && type2 == nullptr)
            return true;

        if (type1->type == ast::NodeType::TypeAny)
            return true;

        if (type2 == nullptr)
            return false;

        switch (type2->type)
        {
        case ast::NodeType::TypeAll:
            return true;
        case ast::NodeType::TypeAny:
            return type1->type != ast::NodeType::TypeAll;
        case ast::NodeType::TypeNull:
            return type1->type == type2->type;
        case ast::NodeType::TypeIdentifier:
        {
            if (type1->type != type2->type)
                return false;
            auto type1IdentifierValue = static_cast<ast::TypeIdentifier *>(type1)->value;
            auto type2IdentifierValue = static_cast<ast::TypeIdentifier *>(type2)->value;
            return type1IdentifierValue == type2IdentifierValue;
        }
        case ast::NodeType::TypeChoice:
        {
            const auto &type2Choices = static_cast<ast::TypeChoice *>(type2)->choices;
            for (const auto &choice : type2Choices)
            {
                if (isCompatibleType(type1, choice.get()))
                    return true;
            }
            return false;
        }
        case ast::NodeType::TypeArray:
        {
            if (type1->type != type2->type)
                return false;
            auto type1Array = static_cast<ast::TypeArray *>(type1);
            auto type2Array = static_cast<ast::TypeArray *>(type2);
            return isCompatibleType(type1Array->elementType.get(), type2Array->elementType.get());
        }
        case ast::NodeType::TypeDictionary:
        {
            if (type1->type != type2->type)
                return false;
            auto type1Dict = static_cast<ast::TypeDictionary *>(type1);
            auto type2Dict = static_cast<ast::TypeDictionary *>(type2);
            if (!isCompatibleType(type1Dict->keyType.get(), type2Dict->keyType.get()))
                return false;
            if (!isCompatibleType(type1Dict->valueType.get(), type2Dict->valueType.get()))
                return false;
            return true;
        }
        }
        throw std::runtime_error("Failed to compute type compatibility");
    }

    bool isCompatibleType(ast::TypeExpression *type, obj::Object *obj, obj::Object *existingObj)
    {
        if (type == nullptr)
            return true;

        if (obj == nullptr)
            return false; // [TODO] should this be an exception?  This should not be occuring

        switch (type->type)
        {
        case ast::NodeType::TypeAll:
            return true;
        case ast::NodeType::TypeAny:
        {
            if (existingObj == nullptr)
                return true;
            // extract the type from the existingObj and then compare it to the type
            auto existingType = computeType(existingObj);
            return isCompatibleType(existingType.get(), obj, nullptr);
        }
        case ast::NodeType::TypeNull:
            return obj->type == obj::ObjectType::Null;
        case ast::NodeType::TypeIdentifier:
        {
            auto typeIdentifierValue = static_cast<ast::TypeIdentifier *>(type)->value;
            std::map<std::string, obj::ObjectType> builtInTypeMapping = {
                {"null", obj::ObjectType::Null},
                {"int", obj::ObjectType::Integer},
                {"double", obj::ObjectType::Double},
                {"complex", obj::ObjectType::Complex},
                {"bool", obj::ObjectType::Boolean},
                {"str", obj::ObjectType::String},
                {"error", obj::ObjectType::Error},
                {"io", obj::ObjectType::IOObject},
                {"module", obj::ObjectType::Module},
                {"thread", obj::ObjectType::Thread},
                {"range", obj::ObjectType::Range},
            };
            auto expectedObjType = builtInTypeMapping.find(typeIdentifierValue);
            if (expectedObjType == builtInTypeMapping.end())
                return false;
            return expectedObjType->second == obj->type;
        }
        case ast::NodeType::TypeChoice:
        {
            auto typeChoice = static_cast<ast::TypeChoice *>(type);
            for (const auto &aType : typeChoice->choices)
            {
                if (isCompatibleType(aType.get(), obj, existingObj))
                    return true;
            }
            return false;
        }
        case ast::NodeType::TypeArray:
        {
            if (obj->type != obj::ObjectType::Array && obj->type != obj::ObjectType::ArrayDouble && obj->type != obj::ObjectType::ArrayComplex)
                return false;

            auto typeArray = static_cast<ast::TypeArray *>(type);
            switch (obj->type)
            {
            case obj::ObjectType::Array:
            {
                auto array = static_cast<obj::Array *>(obj);
                for (const auto &element : array->value)
                {
                    if (!isCompatibleType(typeArray->elementType.get(), element.get(), nullptr))
                        return false;
                }
                break;
            }
            case obj::ObjectType::ArrayDouble:
            {
                if (typeArray->elementType->type != ast::NodeType::TypeIdentifier)
                    return false;
                return (static_cast<ast::TypeIdentifier *>(typeArray->elementType.get())->value == "double");
            }
            case obj::ObjectType::ArrayComplex:
            {
                if (typeArray->elementType->type != ast::NodeType::TypeIdentifier)
                    return false;
                return (static_cast<ast::TypeIdentifier *>(typeArray->elementType.get())->value == "complex");
            }
            }
            return true;
        }
        case ast::NodeType::TypeDictionary:
        {
            if (obj->type != obj::ObjectType::Dictionary)
                return false;

            auto typeDictionary = static_cast<ast::TypeDictionary *>(type);
            auto dict = static_cast<obj::Dictionary *>(obj);
            for (const auto &[k, v] : dict->value)
            {
                if (!isCompatibleType(typeDictionary->keyType.get(), k.get(), nullptr))
                    return false;

                if (!isCompatibleType(typeDictionary->valueType.get(), v.get(), nullptr))
                    return false;
            }
            return true;
        }
        case ast::NodeType::TypeSet:
        {
            if (obj->type != obj::ObjectType::Set)
                return false;

            auto typeSet = static_cast<ast::TypeSet *>(type);
            auto setObj = static_cast<obj::Set *>(obj);
            for (const auto &element : setObj->value)
            {
                if (!isCompatibleType(typeSet->elementType.get(), element.get(), nullptr))
                    return false;
            }
            return true;
        }
        case ast::NodeType::TypeFunction:
        {
            if (obj->type != obj::ObjectType::Function)
                return false;

            auto typeFunction = static_cast<ast::TypeFunction *>(type);
            auto func = static_cast<obj::Function *>(obj);
            if (!isCompatibleType(typeFunction->returnType.get(), func->returnType))
                return false;

            if (typeFunction->argTypes.size() != func->argumentTypes.size())
                return false;

            for (size_t i = 0; i < typeFunction->argTypes.size(); ++i)
            {
                if (!isCompatibleType(typeFunction->argTypes[i].get(), func->argumentTypes[i]))
                    return false;
            }
            return true;
        }
        }
        return false;
    }

    ast::TypeFunction *makeFunctionType(const std::string &argTypeStr, const std::string &returnTypeStr)
    {
        ast::TypeFunction *functionType = new ast::TypeFunction();
        {
            auto lexer = createLexer(returnTypeStr, "");
            auto parser = createParser(std::move(lexer));
            functionType->returnType = parser->parseTypeExpression(Parser::Precedence::LOWEST);
            if (!functionType->returnType)
                throw std::runtime_error("Internal error: invalid return type string: " + returnTypeStr);
        }
        {
            auto lexer = createLexer("(" + argTypeStr + ")", "");
            auto parser = createParser(std::move(lexer));
            functionType->argTypes = parser->parseTypeExpressionList(TokenType::RPAREN);
            for (const auto &typeExpr : functionType->argTypes)
            {
                if (!typeExpr)
                    throw std::runtime_error("Internal error: invalid arguments type string: " + argTypeStr);
            }
        }

        return functionType;
    }
}
