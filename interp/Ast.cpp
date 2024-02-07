/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Ast.h"
#include "Util.h"

#include <sstream>
#include <vector>

namespace
{
    std::string indentation(int amount)
    {
        if (amount == 0)
            return "";

        std::stringstream ss;
        for (int i = 0; i < amount; ++i)
            ss << " ";
        return ss.str();
    }
}

namespace ast
{
    std::string Node::tokenLiteral() const
    {
        return token.literal;
    }

    std::string TypeIdentifier::text(int indent) const
    {
        return indentation(indent) + value;
    };

    std::string TypeNull::text(int indent) const
    {
        return indentation(indent) + "null";
    };

    std::string TypeAny::text(int indent) const
    {
        return indentation(indent) + "any";
    };

    std::string TypeAll::text(int indent) const
    {
        return indentation(indent) + "all";
    };

    std::string TypeChoice::text(int indent) const
    {
        std::vector<std::string> args;
        for (const auto &element : choices)
            args.push_back(element->text());

        std::stringstream ss;
        ss << "<";
        ss << util::join(args, ", ");
        ss << ">";
        return ss.str();
    };

    std::string TypeArray::text(int indent) const
    {
        std::stringstream ss;
        ss << "[";
        if (elementType)
            ss << elementType->text();
        else
            ss << "unknown";
        ss << "]";
        return ss.str();
    };

    std::string TypeDictionary::text(int indent) const
    {
        std::stringstream ss;
        ss << "{";
        ss << keyType->text();
        ss << ":";
        ss << valueType->text();
        ss << "}";
        return ss.str();
    };

    std::string TypeSet::text(int indent) const
    {
        std::stringstream ss;
        ss << "{";
        ss << elementType->text();
        ss << "}";
        return ss.str();
    };

    std::string TypeFunction::text(int indent) const
    {
        std::vector<std::string> args;
        for (const auto &argType : argTypes)
            args.push_back(argType->text());

        std::stringstream ss;
        ss << "fn(";
        ss << util::join(args, ",");
        ss << ") -> ";
        if (returnType)
            ss << returnType->text();
        else
            ss << "unknown";
        return ss.str();
    };

    std::string TypeType::text(int indent) const
    {
        return value;
    };

    std::string BoundType::text(int indent) const
    {
        std::stringstream ss;
        ss << boundTo->text();
        ss << ".";
        ss << boundType->text();
        return ss.str();
    };

    std::string Identifier::text(int indent) const
    {
        return indentation(indent) + token.literal;
    };

    std::string ModuleIdentifier::text(int indent) const
    {
        return indentation(indent) + token.literal;
    };

    std::string BooleanLiteral::text(int indent) const
    {
        return indentation(indent) + token.literal;
    };

    std::string IntegerLiteral::text(int indent) const
    {
        return indentation(indent) + token.literal;
    };

    std::string RangeLiteral::text(int indent) const
    {
        return indentation(indent) + std::to_string(lower) + ".." + std::to_string(upper) + ":" + std::to_string(stride);
    };

    std::string DoubleLiteral::text(int indent) const
    {
        return indentation(indent) + token.literal;
    };

    std::string ComplexLiteral::text(int indent) const
    {
        return indentation(indent) + token.literal;
    };

    std::string StringLiteral::text(int indent) const
    {
        return indentation(indent) + token.literal;
    };

    std::string NullLiteral::text(int indent) const
    {
        return indentation(indent) + token.literal;
    };

    std::string FunctionLiteral::text(int indent) const
    {
        std::vector<std::string> params;
        for (size_t index = 0; index < arguments.size(); ++index)
        {
            const auto &param = arguments[index];
            const auto &paramType = argumentTypes[index];
            std::stringstream ss;
            ss << param.text();
            if (paramType.get())
                ss << " : " << paramType->text();
            params.push_back(ss.str());
        }

        std::stringstream ss;
        ss << token.literal << "(";
        ss << util::join(params, ", ");
        ss << ")";

        if (returnType.get())
            ss << " -> " << returnType->text();

        ss << "\n";
        ss << body->text(indent);
        return ss.str();
    };

    std::string CallExpression::text(int indent) const
    {
        std::vector<std::string> args;
        for (const auto &argument : arguments)
            args.push_back(argument->text());

        std::stringstream ss;
        if (function)
            ss << function->text();
        else
            ss << "<NULL>";
        ss << "(";
        ss << util::join(args, ", ");
        ss << ")";
        return ss.str();
    };

    std::string MemberExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << expr->text();
        ss << ".";
        ss << value.text();
        return ss.str();
    };

    std::string ModuleMemberExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << expr->text();
        ss << "::";
        ss << value.text();
        return ss.str();
    };

    std::string ArrayLiteral::text(int indent) const
    {
        std::vector<std::string> args;
        for (const auto &element : elements)
        {
            if (element)
                args.push_back(element->text());
        }

        std::stringstream ss;
        ss << "[";
        ss << util::join(args, ", ");
        ss << "]";
        return ss.str();
    };

    std::string ArrayDoubleLiteral::text(int indent) const
    {
        std::vector<std::string> args;
        for (const auto &element : elements)
            args.push_back(std::to_string(element));

        std::stringstream ss;
        ss << "[";
        ss << util::join(args, ", ");
        ss << "]";
        return ss.str();
    };

    std::string ArrayComplexLiteral::text(int indent) const
    {
        std::vector<std::string> args;
        // for (const auto& element : elements)
        // 	args.push_back(std::to_string(element));

        std::stringstream ss;
        ss << "[";
        ss << util::join(args, ", ");
        ss << "]";
        return ss.str();
    };

    std::string DictLiteral::text(int indent) const
    {
        std::stringstream ss;
        ss << "{";

        std::vector<std::string> pairs;
        for (const auto &element : elements)
            pairs.push_back(util::join({element.first->text(), element.second->text()}, " : "));

        ss << util::join(pairs, ", ");
        ss << "}";
        return ss.str();
    };

    std::string SetLiteral::text(int indent) const
    {
        std::stringstream ss;
        ss << "{";

        std::vector<std::string> elementsStr;
        for (const auto &element : elements)
            elementsStr.push_back(element->text());

        ss << util::join(elementsStr, ", ");
        ss << "}";
        return ss.str();
    };

    std::string IndexExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << expression->text();
        ss << "[";
        ss << index->text();
        ss << "]";
        return ss.str();
    };

    std::string IfExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << "if (" << condition->text() << ")\n";
        ss << consequence->text(indent);
        if (alternative)
            ss << "else\n"
               << alternative->text(indent);
        return ss.str();
    }

    std::string WhileExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << "while";
        ss << " (" << condition->text() << ")\n";
        ss << statement->text(indent);
        return ss.str();
    }

    std::string InfixExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << "(";
        ss << left->text();
        ss << " " << operator_t.literal << " ";
        ss << right->text();
        ss << ")";
        return ss.str();
    }

    std::string PrefixExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << "(";
        ss << operator_t.literal;
        ss << right->text();
        ss << ")";
        return ss.str();
    }

    std::string OperatorExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << left->text();
        ss << " " << operator_t.literal << " ";
        ss << right->text();
        return ss.str();
    }

    std::string ExpressionStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent);
        if (expression)
            ss << expression->text();
        ss << ";";
        return ss.str();
    }

    std::string ReturnStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << tokenLiteral() << " ";
        if (returnValue)
            ss << returnValue->text();
        ss << ";";
        return ss.str();
    }

    std::string BreakStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << tokenLiteral() << ";";
        return ss.str();
    }

    std::string ContinueStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << tokenLiteral() << ";";
        return ss.str();
    }

    std::string TryExceptStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << "try"
           << "\n";
        ss << indentation(indent) << statement->text() << "\n";
        ss << indentation(indent) << "except (" << name.text();
        if (errorType)
            ss << " : " << errorType->text();
        ss << ")\n";
        ss << indentation(indent) << except->text() << "\n";
        return ss.str();
    }

    std::string LetStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << tokenLiteral() << " ";
        if (constant)
        {
            ss << "const ";
        }
        ss << name.token.literal;
        if (valueType.get())
        {
            ss << " : ";
            ss << valueType->text();
        }
        ss << " = ";
        if (value)
            ss << value->text();
        ss << ";";
        return ss.str();
    }

    std::string ImportStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << tokenLiteral() << " ";
        ss << name.token.literal;
        ss << ";";
        return ss.str();
    }

    std::string ForExpression::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << "for ";
        ss << "(" << name.value << " in " << iterable->text() << ")\n";
        ss << statement->text(indent) << ";";
        return ss.str();
    }

    std::string TypeStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent);
        if (constant)
        {
            ss << "const ";
        }
        ss << name.token.literal;
        if (exprType.get())
        {
            ss << " : ";
            ss << exprType->text();
        }
        ss << " = ";
        if (value)
            ss << value->text();
        ss << ";";
        return ss.str();
    }

    std::string TypeLiteral::text(int indent) const
    {
        std::stringstream ss;
        ss << token.literal << "{";
        for (const auto &def : definitions)
        {
            ss << def->text(indent + 2);
            ss << "\n";
        }
        ss << "}";
        return ss.str();
    }

    std::string BlockStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << "{\n";
        for (const auto &statement : statements)
            ss << statement->text(indent + 2) << "\n";
        ss << indentation(indent) << "}";

        return ss.str();
    }

    std::string ScopeStatement::text(int indent) const
    {
        std::stringstream ss;
        ss << indentation(indent) << "scope {\n";
        for (const auto &statement : statements)
            ss << statement->text(indent + 2) << "\n";
        ss << indentation(indent) << "}";

        return ss.str();
    }

    std::string Program::text(int indent) const
    {
        std::stringstream ss;
        for (const auto &statement : statements)
            ss << statement->text(indent) << "\n";

        return ss.str();
    }
}
