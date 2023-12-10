#include "Freeze.h"
#include "../Evaluator.h"

namespace builtin
{
    std::shared_ptr<obj::Object> frozen(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("frozen: expected 1 arguments");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        bool eq = evaluatedExpr->frozen > 0;
        return std::make_shared<obj::Boolean>(eq);
    }

    std::shared_ptr<obj::Object> freeze(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("freeze: expected 1 arguments");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        ++evaluatedExpr->frozen;
        return evaluatedExpr;
    }

    std::shared_ptr<obj::Object> defrost(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("defrost: expected 1 arguments");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr->frozen > 0)
            --evaluatedExpr->frozen;
        return evaluatedExpr;
    }

    std::shared_ptr<obj::Object> freezer(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment)
    {
        if (!arguments)
            return NullObject;

        if (arguments->size() != 1)
            return obj::makeTypeError("freezer: expected 1 arguments");

        auto evaluatedExpr = evalExpression(arguments->front().get(), environment);

        return std::make_shared<obj::ObjectFreezer>(evaluatedExpr);
    }
}