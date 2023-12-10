#ifndef GUARDIAN_OF_INCLUSION_BUILTIN_ERROR_H
#define GUARDIAN_OF_INCLUSION_BUILTIN_ERROR_H

#include "../Object.h"

namespace builtin
{
    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeError();
}

#endif