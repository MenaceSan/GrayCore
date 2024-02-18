//! @file cArchive.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cArchive.h"

namespace Gray {
HRESULT cArchive::Serialize(cMemSpan& m) {
    //! @return <0 = error HRESULT_WIN32_C(ERROR_IO_INCOMPLETE)
    HRESULT hRes;
    if (IsStoring()) {
        hRes = ref_Out().WriteSpan(m);
    } else {
        hRes = ref_Inp().ReadSpan(m);
    }
    return hRes;
}

HRESULT cArchive::SerializeSize(size_t& nSize) {
    //! Write a compressed size. high bit of byte is reserved to say there is more to come.
    //! bytes stored low to high (of course)
    //! MFC calls this "Count"
    if (IsStoring()) {
        return ref_Out().WriteSize(nSize);
    } else {
        return ref_Inp().ReadSize(nSize);
    }
}
}  // namespace Gray
