//! @file cArray.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cArray.h"
#include "cArraySortString.h"
#include "cString.h"

namespace Gray {

#ifndef GRAY_STATICLIB  // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cArrayString<char>;
template struct GRAYCORE_LINK cArrayString<wchar_t>;

template class GRAYCORE_LINK cArraySorted<cStringT<char>, const char*, const char*>;
template class GRAYCORE_LINK cArraySorted<cStringT<wchar_t>, const wchar_t*, const wchar_t*>;

template struct GRAYCORE_LINK cArraySortString<char>;
template struct GRAYCORE_LINK cArraySortString<wchar_t>;
#endif

}  // namespace Gray
