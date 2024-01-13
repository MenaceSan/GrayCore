//
//! @file StrT.cpp
//! String global functions as a template.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrFormat.h"
#include "StrT.h"

namespace Gray {
const char StrT::k_szBlockStart[static_cast<int>(STR_BLOCK_t::_QTY) + 1] = "\"{[(";  // STR_BLOCK_t
const char StrT::k_szBlockEnd[static_cast<int>(STR_BLOCK_t::_QTY) + 1] = "\"}])";

const char StrT::k_szEscEncode[12] = "\'\"\?\\abtnvfr";         // encoded form.
const char StrT::k_szEscDecode[12] = "\'\"\?\\\a\b\t\n\v\f\r";  // decoded form.
}  // namespace Gray

//*************************************************************
#include "StrT.inl"

//! force a template function to instantiate for TYPE of char and wchar_t. Explicit instantiation

namespace Gray {
// force implementation/instantiate for DLL/SO and GRAY_STATICLIB.
template struct GRAYCORE_LINK StrX<char>;
template struct GRAYCORE_LINK StrX<wchar_t>;

// even GRAY_STATICLIB needs this to expose the symbols. but only in vs2019.
#define TYPE char
#define StrTTbl(returntype, name, args) template GRAYCORE_LINK returntype GRAYCALL StrT::name<TYPE> args;
#include "StrT.tbl"
#undef TYPE

#define TYPE wchar_t
#include "StrT.tbl"
#undef TYPE
#undef StrTTbl

// use the C lib else internal StrFormat
// #undef USE_CRT

template <>
GRAYCORE_LINK StrLen_t GRAYCALL StrT::vsprintfN<char>(OUT char* pszOut, StrLen_t iLenOutMax, const char* pszFormat, va_list vlist) {
    //! UTF8 sprintf version.
    //! @note Use StrArg<char>(s) for safe "%s" args.
    //! @note Windows _snprintf and _vsnprintf are not compatible to Linux versions.
    //!  Linux result is the size of the buffer that it should have.
    //!  Windows Result value is not size of buffer needed, but -1 if no fit is possible.
    //!  Newer Windows versions have a _TRUNCATE option to just truncate the string.
    //!  using a 2 pass method we can estimate the size needed in advance. if pszOut = nullptr.
    //! @arg
    //!  pszOut = vsnprintf is OK with nullptr and size=0.
    //!  iLenOutMax = size in characters. (Not Bytes) Must allow space for '\0'
    //! @return
    //!  size in characters.  negative value if an output error occurs.
#if defined(__linux__)
    return ::vsnprintf(pszOut, iLenOutMax, pszFormat, vlist);  // C99
#elif USE_CRT && (_MSC_VER >= 1400) && !defined(UNDER_CE)
    // CRT version. act as _TRUNCATE
    return ::_vsnprintf_s(pszOut, (size_t)(iLenOutMax), (size_t)(iLenOutMax - 1), pszFormat, vlist);  // to shut up the deprecated warnings.
#elif USE_CRT
    // OLD CRT version.
    return ::_vsnprintf(pszOut, iLenOutMax, pszFormat, vlist);
#else  // _WIN32
       // Use internal version of _vsnprintf
       // @note dont use _WIN32 System version (No floats). e.g. return ::FormatMessageA(0, pszFormat, 0, 0, pszOut, iLenOutMax, &vlist);
    return StrFormat<char>::V(pszOut, iLenOutMax, pszFormat, vlist);
#endif
}

template <>
GRAYCORE_LINK StrLen_t GRAYCALL StrT::vsprintfN<wchar_t>(OUT wchar_t* pszOut, StrLen_t iLenOutMax, const wchar_t* pszFormat, va_list vlist) {
    //! UNICODE sprintf version.
    //! @note Use StrArg<wchar_t>(s) for safe "%s" args.
    //! @arg
    //!  pszOut = vsnprintf is ok with nullptr and size=0.
    //!  iLenOutMax = size in characters. (Not Bytes) Must allow space for '\0'
    //! @return
    //!  size in characters. -1 = too small.
#if defined(__linux__)
    return ::vswprintf(pszOut, iLenOutMax, pszFormat, vlist);  // C99
#elif USE_CRT && (_MSC_VER >= 1400) && !defined(UNDER_CE)
    // CRT version. act as _TRUNCATE
    return ::_vsnwprintf_s(pszOut, (size_t)(iLenOutMax), (size_t)(iLenOutMax - 1), pszFormat, vlist);  // to shut up the deprecated warnings.
#elif USE_CRT
    // OLD CRT version.
    return ::_vsnwprintf(pszOut, iLenOutMax, pszFormat, vlist);
#else  // _WIN32
       // Use internal version of _vsnprintf
       // @note dont use _WIN32 System version (No floats)  return ::FormatMessageW(0, pszFormat, 0, 0, pszOut, iLenOutMax, &vlist);
    return StrFormat<wchar_t>::V(pszOut, iLenOutMax, pszFormat, vlist);
#endif
}
}  // namespace Gray
