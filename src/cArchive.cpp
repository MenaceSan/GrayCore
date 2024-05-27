//! @file cArchive.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cArchive.h"

namespace Gray {
HRESULT cArchive::Serialize(cMemSpan ret) {
    HRESULT hRes;
    if (IsStoring()) {
        hRes = ref_Out().WriteSpan(ret);
    } else {
        hRes = ref_Inp().ReadSpan(ret);
    }
    return hRes;
}

HRESULT cArchive::SerializeSize(size_t& nSize) {
    if (IsStoring()) {
        return ref_Out().WriteSize(nSize);
    } else {
        return ref_Inp().ReadSize(nSize);
    }
}
}  // namespace Gray
