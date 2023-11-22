#ifndef GUARDIAN_OF_INCLUSION_BUILTIN_FREEZE_H
#define GUARDIAN_OF_INCLUSION_BUILTIN_FREEZE_H

#include "../Object.h"

namespace builtin
{
    std::shared_ptr<obj::Object> frozen(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment);
    std::shared_ptr<obj::Object> freeze(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment);
    std::shared_ptr<obj::Object> defrost(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment);
    std::shared_ptr<obj::Object> freezer(const std::vector<std::unique_ptr<ast::Expression>> *arguments, const std::shared_ptr<obj::Environment> &environment);
}

#endif