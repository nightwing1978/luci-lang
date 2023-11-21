#ifndef GUARDIAN_OF_INCLUSION_BUILTIN_THREADING_H
#define GUARDIAN_OF_INCLUSION_BUILTIN_THREADING_H

#include "../Object.h"

namespace builtin
{
    std::shared_ptr<obj::Module> createThreadingModule();
}

#endif