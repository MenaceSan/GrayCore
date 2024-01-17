//
//! @file cTypeInfo.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrChar.h"
#include "cTypeInfo.h"
#include "cTypes.h"

namespace Gray {
const char* GRAYCALL cTypeInfo::GetSymName(const char* name) noexcept {  // static
    // Turn "class GrayLib::cXObject" into "XObject"
    if (name == nullptr) return name;
    char ch = name[0];
    if (ch != '\0' && StrChar::ToLowerA(ch) == 'c' && StrChar::IsUpperA(name[1])) return name + 1;
    return name;
}

#ifdef _CPPRTTI

const void** GRAYCALL cTypeInfo::Get_vtable(void* p) {  // static
    // Get the vtable of the object.
    // Assume this is a 'new' returned pointer.
    UNREFERENCED_PARAMETER(p);
    return nullptr;
}

const char* GRAYCALL cTypeInfo::GetMemberName_TODO(const void** vtable, PROPIDX_t i) {  // static
    //! read the __vfptr/vtable AKA _vptr, vftable to get a list of names of the virtual exposed members.
    //! @todo enum List of members.
    //! @return nullptr = end of list.
    if (i < 0) return nullptr;
    if (i > 1) return nullptr;

    // _MSC_VER
    // __GNUC__
    UNREFERENCED_PARAMETER(vtable);
    return "test";
}
#endif
}  // namespace Gray
