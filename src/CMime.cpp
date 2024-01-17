//
//! @file cMime.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrT.h"
#include "cMime.h"

namespace Gray {
const cMime cMime::k_Type[static_cast<int>(MIME_t::_QTY)] = {
// static
#define cMimeType(a, b, c, d, e) {b, c, d},
#include "cMimeTypes.tbl"
#undef cMimeType
};

MIME_t GRAYCALL cMime::FindMimeTypeForExt(const RESCHAR_t* pszExt, MIME_t eMimeTypeDefault) {  // static
    //! For a given file '.ext', find the MIME_t for it. read from cMimeTypes.tbl
    //! @note we could check for text files vs binary files ?

    for (COUNT_t i = 0; i < _countof(k_Type); i++) {
        if (StrT::CmpI(pszExt, k_Type[i].m_pExt)) return CastN(MIME_t, i);
        if (StrT::CmpI(pszExt, k_Type[i].m_pExt2)) return CastN(MIME_t, i);
    }
    return eMimeTypeDefault;
}

const RESCHAR_t* GRAYCALL cMime::GetMimeTypeName(MIME_t eMimeType) {  // static
    if (IS_INDEX_BAD(eMimeType, static_cast<int>(MIME_t::_QTY))) eMimeType = MIME_t::_TEXT;
    return k_Type[static_cast<int>(eMimeType)].m_pszName;
}

MIME_t GRAYCALL cMime::FindMimeTypeName(const RESCHAR_t* pszName) {  // static
    //! NOT exactly the same as Str_TableFindHead() ?
    if (pszName == nullptr) return MIME_t::_UNKNOWN;
    for (COUNT_t i = 0; i < _countof(k_Type); i++) {
        const char* pszNamePrefix = k_Type[i].m_pszName;
        if (StrT::StartsWithI(pszName, pszNamePrefix)) return CastN(MIME_t, i);
    }
    return MIME_t::_UNKNOWN;
}
}  // namespace Gray
