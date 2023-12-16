/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#ifndef GUARDIAN_OF_INCLUSION_BUILTIN_THREAD_H
#define GUARDIAN_OF_INCLUSION_BUILTIN_THREAD_H

#include "../Object.h"

namespace builtin
{
    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeThread();
}

#endif