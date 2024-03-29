/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#ifndef GUARDIAN_OF_INCLUSION_AST_H
#define GUARDIAN_OF_INCLUSION_AST_H

#include <string>
#include <vector>
#include <complex>
#include <map>
#include <set>
#include <memory>
#include <iostream>

#include "Token.h"

namespace ast
{
    enum class NodeType
    {
        Unknown = -1,
        Statement = 0,
        Expression = 1,
        BlockStatement = 2,
        IfExpression = 3,
        WhileExpression = 4,
        InfixExpression = 5,
        PrefixExpression = 6,
        OperatorExpression = 7,
        BooleanLiteral = 8,
        IntegerLiteral = 9,
        DoubleLiteral = 10,
        ComplexLiteral = 11,
        StringLiteral = 12,
        NullLiteral = 13,
        Identifier = 14,
        FunctionLiteral = 15,
        CallExpression = 16,
        ArrayLiteral = 17,
        ArrayDoubleLiteral = 18,
        ArrayComplexLiteral = 19,
        DictLiteral = 20,
        SetLiteral = 21,
        IndexExpression = 22,
        LetStatement = 23,
        ReturnStatement = 24,
        BreakStatement = 25,
        ExpressionStatement = 26,
        Program = 27,
        TypeExpression = 28,
        TypeNull = 29,
        TypeIdentifier = 30,
        TypeChoice = 31,
        TypeArray = 32,
        TypeDictionary = 33,
        TypeSet = 34,
        TypeFunction = 35,
        TypeAny = 36,
        TypeAll = 37,
        MemberExpression = 38,
        TypeType = 39,
        TypeLiteral = 40,
        TypeStatement = 41,
        BoundType = 42,
        ScopeStatement = 43,
        ForExpression = 44,
        ModuleMemberExpression = 45,
        ImportStatement = 46,
        ModuleIdentifier = 47,
        TryExceptStatement = 48,
        ContinueStatement = 49,
        RangeLiteral = 50,
    };

    enum class MarkedAsBuiltin
    {
        Unknown = -1,
        False = 0,
        True = 1,
    };

    struct Node
    {
        NodeType type = NodeType::Unknown;
        Token token;
        virtual std::string tokenLiteral() const;
        virtual std::string text(int indent = 0) const = 0;
        Node(NodeType itype = NodeType::Unknown) : type(itype){};
    };

    struct Statement : public Node
    {
        Statement(NodeType itype = NodeType::Statement) : Node(itype){};
    };

    struct TypeExpression : public Node
    {
        virtual std::unique_ptr<TypeExpression> clone() const = 0;
        TypeExpression(NodeType itype = NodeType::TypeExpression) : Node(itype){};
    };

    struct TypeNull : public TypeExpression
    {
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override { return std::make_unique<ast::TypeNull>(); };
        TypeNull(NodeType itype = NodeType::TypeNull) : TypeExpression(itype){};
    };

    struct TypeAny : public TypeExpression
    {
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override { return std::make_unique<ast::TypeAny>(); };
        TypeAny(NodeType itype = NodeType::TypeAny) : TypeExpression(itype){};
    };

    struct TypeAll : public TypeExpression
    {
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override { return std::make_unique<ast::TypeAll>(); };
        TypeAll(NodeType itype = NodeType::TypeAll) : TypeExpression(itype){};
    };

    struct TypeIdentifier : public TypeExpression
    {
        std::string value;
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override
        {
            auto typeIdent = std::make_unique<ast::TypeIdentifier>();
            typeIdent->value = value;
            return typeIdent;
        };
        TypeIdentifier() : TypeExpression(NodeType::TypeIdentifier){};
        TypeIdentifier(const std::string &ivalue) : TypeExpression(NodeType::TypeIdentifier), value(ivalue){};
    };

    struct TypeChoice : public TypeExpression
    {
        std::vector<std::unique_ptr<TypeExpression>> choices;
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override
        {
            auto typeChoice = std::make_unique<ast::TypeChoice>();
            for (const auto &type : choices)
                typeChoice->choices.push_back(type->clone());
            return typeChoice;
        };
        TypeChoice() : TypeExpression(NodeType::TypeChoice){};
    };

    struct TypeArray : public TypeExpression
    {
        std::unique_ptr<TypeExpression> elementType;
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override
        {
            auto typeArray = std::make_unique<ast::TypeArray>();
            if (elementType)
                typeArray->elementType = elementType->clone();
            return typeArray;
        };
        TypeArray() : TypeExpression(NodeType::TypeArray){};
    };

    struct TypeDictionary : public TypeExpression
    {
        std::unique_ptr<TypeExpression> keyType;
        std::unique_ptr<TypeExpression> valueType;
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override
        {
            auto typeDict = std::make_unique<ast::TypeDictionary>();
            if (keyType)
                typeDict->keyType = keyType->clone();
            if (valueType)
                typeDict->valueType = valueType->clone();
            return typeDict;
        };

        TypeDictionary() : TypeExpression(NodeType::TypeDictionary){};
    };

    struct TypeSet : public TypeExpression
    {
        std::unique_ptr<TypeExpression> elementType;
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override
        {
            auto typeSet = std::make_unique<ast::TypeSet>();
            if (elementType)
                typeSet->elementType = elementType->clone();
            return typeSet;
        };

        TypeSet() : TypeExpression(NodeType::TypeSet){};
    };

    struct TypeFunction : public TypeExpression
    {
        std::unique_ptr<TypeExpression> returnType;
        std::vector<std::unique_ptr<TypeExpression>> argTypes;
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override
        {
            auto typeFunc = std::make_unique<ast::TypeFunction>();
            if (returnType)
                typeFunc->returnType = returnType->clone();
            for (const auto &argType : argTypes)
            {
                if (argType)
                    typeFunc->argTypes.push_back(argType->clone());
                else
                    typeFunc->argTypes.push_back(nullptr);
            }
            return typeFunc;
        };
        TypeFunction() : TypeExpression(NodeType::TypeFunction){};
    };

    struct TypeType : public TypeExpression
    {
        std::string value;
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override
        {
            auto typeType = std::make_unique<ast::TypeType>();
            typeType->value = value;
            return typeType;
        };
        TypeType() : TypeExpression(NodeType::TypeType){};
    };

    struct BoundType : public TypeExpression
    {
        std::unique_ptr<TypeExpression> boundTo;
        std::unique_ptr<TypeExpression> boundType;
        virtual std::string text(int indent = 0) const override;
        virtual std::unique_ptr<TypeExpression> clone() const override
        {
            auto boundType = std::make_unique<ast::BoundType>();
            boundType->boundTo = boundTo->clone();
            boundType->boundType = boundType->clone();
            return boundType;
        };
        BoundType() : TypeExpression(NodeType::BoundType){};
    };

    struct Expression : public Node
    {
        Expression(NodeType itype = NodeType::Expression) : Node(itype){};
    };

    struct BlockStatement : public Statement
    {
        std::vector<std::unique_ptr<Statement>> statements;
        virtual std::string text(int indent = 0) const override;
        BlockStatement() : Statement(NodeType::BlockStatement){};
    };

    struct ScopeStatement : public Statement
    {
        std::vector<std::unique_ptr<Statement>> statements;
        virtual std::string text(int indent = 0) const override;
        ScopeStatement() : Statement(NodeType::ScopeStatement){};
    };

    struct IfExpression : public Expression
    {
        std::unique_ptr<Expression> condition;
        std::unique_ptr<BlockStatement> consequence;
        std::unique_ptr<BlockStatement> alternative;
        virtual std::string text(int indent = 0) const override;
        IfExpression() : Expression(NodeType::IfExpression){};
    };

    struct WhileExpression : public Expression
    {
        std::unique_ptr<Expression> condition;
        std::unique_ptr<BlockStatement> statement;
        virtual std::string text(int indent = 0) const override;
        WhileExpression() : Expression(NodeType::WhileExpression){};
    };

    struct InfixExpression : public Expression
    {
        std::unique_ptr<Expression> left;
        Token operator_t;
        std::unique_ptr<Expression> right;
        virtual std::string text(int indent = 0) const override;
        InfixExpression() : Expression(NodeType::InfixExpression){};
    };

    struct PrefixExpression : public Expression
    {
        Token operator_t;
        std::unique_ptr<Expression> right;
        virtual std::string text(int indent = 0) const override;
        PrefixExpression() : Expression(NodeType::PrefixExpression){};
    };

    struct OperatorExpression : public Expression
    {
        std::unique_ptr<Expression> left;
        Token operator_t;
        std::unique_ptr<Expression> right;
        virtual std::string text(int indent = 0) const override;
        OperatorExpression() : Expression(NodeType::OperatorExpression){};
    };

    struct BooleanLiteral : public Expression
    {
        bool value = false;
        virtual std::string text(int indent = 0) const override;
        BooleanLiteral() : Expression(NodeType::BooleanLiteral){};
    };

    struct IntegerLiteral : public Expression
    {
        int64_t value = 0;
        virtual std::string text(int indent = 0) const override;
        IntegerLiteral() : Expression(NodeType::IntegerLiteral){};
    };

    struct RangeLiteral : public Expression
    {
        int64_t lower = 0;
        int64_t upper = 1;
        int64_t stride = 1;
        virtual std::string text(int indent = 0) const override;
        RangeLiteral() : Expression(NodeType::RangeLiteral){};
    };
 

    struct DoubleLiteral : public Expression
    {
        double value = 0.0;
        virtual std::string text(int indent = 0) const override;
        DoubleLiteral() : Expression(NodeType::DoubleLiteral){};
    };

    struct ComplexLiteral : public Expression
    {
        std::complex<double> value;
        virtual std::string text(int indent = 0) const override;
        ComplexLiteral() : Expression(NodeType::ComplexLiteral){};
    };

    struct StringLiteral : public Expression
    {
        std::string value;
        virtual std::string text(int indent = 0) const override;
        StringLiteral() : Expression(NodeType::StringLiteral){};
    };

    struct NullLiteral : public Expression
    {
        std::string value;
        virtual std::string text(int indent = 0) const override;
        NullLiteral() : Expression(NodeType::NullLiteral){};
    };

    struct Identifier : public Expression
    {
        // tracks if this indentifier clashes with a built-in function/variable name
        MarkedAsBuiltin markedAsBuiltin = ast::MarkedAsBuiltin::Unknown;

        std::string value;
        virtual std::string text(int indent = 0) const override;
        Identifier() : Expression(NodeType::Identifier){};
    };

    struct ModuleIdentifier : public Expression
    {
        std::vector<std::string> path;
        virtual std::string text(int indent = 0) const override;
        ModuleIdentifier() : Expression(NodeType::ModuleIdentifier){};
    };

    struct FunctionLiteral : public Expression
    {
        std::string doc;
        std::string value;
        std::vector<Identifier> arguments;
        std::vector<std::unique_ptr<TypeExpression>> argumentTypes;
        std::unique_ptr<TypeExpression> returnType;
        std::unique_ptr<BlockStatement> body;

        virtual std::string text(int indent = 0) const override;
        FunctionLiteral() : Expression(NodeType::FunctionLiteral){};
    };

    struct CallExpression : public Expression
    {
        std::unique_ptr<ast::Expression> function;
        std::vector<std::unique_ptr<Expression>> arguments;

        virtual std::string text(int indent = 0) const override;
        CallExpression() : Expression(NodeType::CallExpression){};
    };

    struct MemberExpression : public Expression
    {
        std::unique_ptr<ast::Expression> expr;
        Identifier value;

        virtual std::string text(int indent = 0) const override;
        MemberExpression() : Expression(NodeType::MemberExpression){};
    };

    struct ModuleMemberExpression : public Expression
    {
        std::unique_ptr<ast::Expression> expr;
        Identifier value;

        virtual std::string text(int indent = 0) const override;
        ModuleMemberExpression() : Expression(NodeType::ModuleMemberExpression){};
    };

    struct ArrayLiteral : public Expression
    {
        std::vector<std::unique_ptr<Expression>> elements;

        virtual std::string text(int indent = 0) const override;
        ArrayLiteral() : Expression(NodeType::ArrayLiteral){};
    };

    struct ArrayDoubleLiteral : public Expression
    {
        std::vector<double> elements;

        virtual std::string text(int indent = 0) const override;
        ArrayDoubleLiteral() : Expression(NodeType::ArrayDoubleLiteral){};
    };

    struct ArrayComplexLiteral : public Expression
    {
        std::vector<std::complex<double>> elements;

        virtual std::string text(int indent = 0) const override;
        ArrayComplexLiteral() : Expression(NodeType::ArrayComplexLiteral){};
    };

    struct ExpressionPtrCompare
    {
        const bool operator()(const std::unique_ptr<Expression> &a, const std::unique_ptr<Expression> &b) const
        {
            return a.get() < b.get();
        }
    };

    struct DictLiteral : public Expression
    {
        std::map<std::unique_ptr<Expression>, std::unique_ptr<Expression>, ExpressionPtrCompare> elements;

        virtual std::string text(int indent = 0) const override;
        DictLiteral() : Expression(NodeType::DictLiteral){};
    };

    struct SetLiteral : public Expression
    {
        std::set<std::unique_ptr<Expression>, ExpressionPtrCompare> elements;

        virtual std::string text(int indent = 0) const override;
        SetLiteral() : Expression(NodeType::SetLiteral){};
    };

    struct IndexExpression : public Expression
    {
        std::unique_ptr<Expression> expression;
        std::unique_ptr<Expression> index;

        virtual std::string text(int indent = 0) const override;
        IndexExpression() : Expression(NodeType::IndexExpression){};
    };

    struct LetStatement : public Statement
    {
        bool constant = false;
        Identifier name;
        std::unique_ptr<TypeExpression> valueType;
        std::unique_ptr<Expression> value;
        virtual std::string text(int indent = 0) const override;
        LetStatement() : Statement(NodeType::LetStatement){};
    };

    struct ImportStatement : public Statement
    {
        ModuleIdentifier name;
        virtual std::string text(int indent = 0) const override;
        ImportStatement() : Statement(NodeType::ImportStatement){};
    };

    struct ForExpression : public Expression
    {
        bool constant = false;
        Identifier name;
        std::unique_ptr<TypeExpression> iterType;
        std::unique_ptr<Expression> iterable;
        std::unique_ptr<BlockStatement> statement;
        virtual std::string text(int indent = 0) const override;
        ForExpression() : Expression(NodeType::ForExpression){};
    };

    struct TypeStatement : public Statement
    {
        bool constant = false;                    /*< when the member was defined as constant */
        Identifier name;                          /*< name of the member of the type */
        std::unique_ptr<TypeExpression> exprType; /*< optionally a declared type of the type statement */
        std::unique_ptr<Expression> value;        /*< the content of the type statement*/
        virtual std::string text(int indent = 0) const override;
        TypeStatement() : Statement(NodeType::TypeStatement){};
    };

    struct TypeLiteral : public Expression
    {
        std::string name;                                        /*< name of the type */
        std::string doc;                                         /*< optionally associated documentation */
        std::vector<std::unique_ptr<TypeStatement>> definitions; /*< list of definitions belong to the type */
        virtual std::string text(int indent = 0) const override;
        TypeLiteral() : Expression(NodeType::TypeLiteral){};
    };

    struct ReturnStatement : public Statement
    {
        std::unique_ptr<Expression> returnValue;
        virtual std::string text(int indent = 0) const override;
        ReturnStatement() : Statement(NodeType::ReturnStatement){};
    };

    struct BreakStatement : public Statement
    {
        virtual std::string text(int indent = 0) const override;
        BreakStatement() : Statement(NodeType::BreakStatement){};
    };

    struct ContinueStatement : public Statement
    {
        virtual std::string text(int indent = 0) const override;
        ContinueStatement() : Statement(NodeType::ContinueStatement){};
    };

    struct TryExceptStatement : public Statement
    {
        virtual std::string text(int indent = 0) const override;
        std::unique_ptr<BlockStatement> statement;
        std::unique_ptr<BlockStatement> except;
        Identifier name;
        std::unique_ptr<TypeExpression> errorType;
        TryExceptStatement() : Statement(NodeType::TryExceptStatement){};
    };

    struct ExpressionStatement : public Statement
    {
        std::unique_ptr<Expression> expression;
        virtual std::string text(int indent = 0) const override;
        ExpressionStatement() : Statement(NodeType::ExpressionStatement){};
    };

    struct Program : public Node
    {
        std::vector<std::unique_ptr<Statement>> statements;
        virtual std::string text(int indent = 0) const override;
        Program() : Node(NodeType::Program){};
    };
}

#endif