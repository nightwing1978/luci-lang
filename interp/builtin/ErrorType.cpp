#include "ErrorType.h"

namespace builtin
{
    std::shared_ptr<obj::Module> makeModuleErrorType()
    {
        auto errorTypeModule = std::make_shared<obj::Module>();
        errorTypeModule->state = obj::ModuleState::Loaded;

        errorTypeModule->environment = std::make_shared<obj::Environment>();

        std::map<std::string, obj::ErrorType> errorNames = {
            {"undefined_error", obj::ErrorType::UndefinedError},
            {"type_error", obj::ErrorType::TypeError},
            {"const_error", obj::ErrorType::ConstError},
            {"identifier_not_found", obj::ErrorType::IdentifierNotFound},
            {"identifier_already_exists", obj::ErrorType::IdentifierAlreadyExists},
            {"value_error", obj::ErrorType::ValueError},
            {"key_error", obj::ErrorType::KeyError},
            {"index_error", obj::ErrorType::IndexError},
            {"import_error", obj::ErrorType::ImportError},
            {"syntax_error", obj::ErrorType::SyntaxError},
            {"os_error", obj::ErrorType::OSError},
        };

        for (const auto [name, errType] : errorNames)
            errorTypeModule->environment->add(name, std::make_shared<obj::Integer>(static_cast<int64_t>(errType)), true, nullptr);

        return errorTypeModule;
    }
}