#ifndef GUARDIAN_OF_INCLUSION_BUILTIN_ERROR_H
#define GUARDIAN_OF_INCLUSION_BUILTIN_ERROR_H

#include "../Object.h"

namespace builtin
{
    enum class ErrorType
    {
        UndefinedError=0,       /* */
        TypeError=1,              /* wrong number of arguments, wrong arguments */
    };

    std::shared_ptr<obj::BuiltinType> makeBuiltinTypeError();
    std::shared_ptr<obj::Module> makeModuleErrorType();
}

#endif