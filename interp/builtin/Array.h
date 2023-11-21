#ifndef GUARDIAN_OF_INCLUSION_BUILTIN_ARRAY_H
#define GUARDIAN_OF_INCLUSION_BUILTIN_ARRAY_H

#include "../Object.h"

namespace builtin
{
    std::shared_ptr<obj::Object> array_push_back(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments);
    std::shared_ptr<obj::Object> array_reverse(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments);
    std::shared_ptr<obj::Object> array_reversed(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments);
    std::shared_ptr<obj::Object> array_rotate(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments);
    std::shared_ptr<obj::Object> array_rotated(const std::shared_ptr<obj::Object>& self, const std::vector<std::shared_ptr<obj::Object> >& arguments);

    std::vector<std::shared_ptr<obj::BuiltinType>> makeBuiltinTypeArrays();
}

#endif
