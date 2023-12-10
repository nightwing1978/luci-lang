#ifndef GUARDIAN_OF_INCLUSION_BUILTIN_REGEX_H
#define GUARDIAN_OF_INCLUSION_BUILTIN_REGEX_H

#include "../Object.h"

namespace builtin
{
    std::shared_ptr<obj::Module> createRegexModule();
}

#endif