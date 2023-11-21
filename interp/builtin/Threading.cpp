#include "Threading.h"
#include "../Evaluator.h"
#include "../Typing.h"

namespace builtin
{
	std::shared_ptr<obj::Object> thread(const std::vector<std::unique_ptr<ast::Expression> >* arguments, const std::shared_ptr<obj::Environment>& environment)
	{
		if (!arguments)
			return NullObject;

		if (arguments->size() != 1)
			return std::make_shared<obj::Error>("thread: expected 1 argument");

    	auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr1->type != obj::ObjectType::Function)
            return std::make_shared<obj::Error>("thread: expected argument 1 to be a function");

        auto threadObj = std::make_shared<obj::Thread>();
        threadObj->function = std::dynamic_pointer_cast<obj::Function>(evaluatedExpr1);
        threadObj->start();
		return threadObj;
	}


	std::shared_ptr<obj::Object> sleep(const std::vector<std::unique_ptr<ast::Expression> >* arguments, const std::shared_ptr<obj::Environment>& environment)
	{
		if (!arguments)
			return NullObject;

		if (arguments->size() != 1)
			return std::make_shared<obj::Error>("sleep: expected 1 argument");

    	auto evaluatedExpr1 = evalExpression(arguments->front().get(), environment);
        if (evaluatedExpr1->type != obj::ObjectType::Double)
            return std::make_shared<obj::Error>("sleep: expected argument 1 to be a double");

        int64_t nanoseconds = static_cast<int64_t>(1e9*static_cast<obj::Double*>(evaluatedExpr1.get())->value);
        std::this_thread::sleep_for(std::chrono::nanoseconds(nanoseconds));
		return NullObject;
	}


    std::shared_ptr<obj::Module> createThreadingModule()
    {
        auto threadingModule = std::make_shared<obj::Module>();
        threadingModule->environment->add("thread", builtin::makeBuiltInFunctionObj(&builtin::thread,"","thread"), false, nullptr);
        threadingModule->environment->add("sleep", builtin::makeBuiltInFunctionObj(&builtin::sleep,"double","null"), false, nullptr);
        threadingModule->state = obj::ModuleState::Loaded;
        return threadingModule;
    }
} // namespace builtin
