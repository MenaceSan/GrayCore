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

const void* const* GRAYCALL cTypeInfo::GetVTable(const void* pObject) {  // static
    // https://reverseengineering.stackexchange.com/questions/5956/how-to-find-the-location-of-the-vtable/5957#5957?newreg=6de86b812bac40688c7f68fbafce262c
    return *PtrCast<const void* const*>(pObject);  // like typeid(void*); similar to dyanmic_cast<>
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
