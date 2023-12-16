/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

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