//
//! @file StrArg.h
//! Make some argument into a string of desired format.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrArg_H
#define _INC_StrArg_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"

namespace Gray {
/// <summary>
/// TEMPORARY Convert some type into a string. ALA ToString()
/// Define (thread safe) temporary string values for use as sprintf() arguments.
/// Use cTempPool
/// </summary>
/// <typeparam name="TYPE">char or wchar_t</typeparam>
/// <param name="pszStr"></param>
/// <returns></returns>
template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(const char* pszStr);
template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(const wchar_t* pszStr);

template <typename TYPE>
inline const TYPE* StrArg(const BYTE* pszStr) {
    // auto convert BYTE to a char type arg. // Special use of BYTE as char by SQLCHAR
    return StrArg<TYPE>(reinterpret_cast<const char*>(pszStr));
}

template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(TYPE ch, StrLen_t nRepeat = 1);

template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(INT32 iVal);
template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(UINT32 uVal, RADIX_t uRadix = 10);

#ifdef USE_INT64
template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(INT64 iVal);
template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(UINT64 uVal, RADIX_t uRadix = 10);
#endif

template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(double dVal);  // , int iPlaces, bool bFormat=false

/// <summary>
/// safe temporary convert arguments for sprintf("%s") type params. ONLY if needed.
/// for string args to _cdecl (variable ...) functions
/// inline this because no processing is needed.
/// </summary>
/// <param name="pszStr"></param>
/// <returns></returns>
template <>
inline const char* GRAYCALL StrArg<char>(const char* pszStr) noexcept {  // static
    return pszStr;
}

/// <summary>
/// safe temporary convert arguments for sprintf("%s") type params. ONLY if needed.
/// for string args to _cdecl (variable ...) functions
/// inline this because no processing is needed.
/// </summary>
/// <param name="pszStr"></param>
/// <returns></returns>
template <>
inline const wchar_t* GRAYCALL StrArg<wchar_t>(const wchar_t* pszStr) noexcept {  // static
    return pszStr;
}

// Special use of BYTE as char by SQLCHAR

template <>
inline const BYTE* GRAYCALL StrArg<BYTE>(const BYTE* pszStr) noexcept {  // static
    return pszStr;                                                       // do nothing.
}
template <>
inline const char* GRAYCALL StrArg<char>(const BYTE* pszStr) noexcept {  // static
    return PtrCast<const char>(pszStr);
}
template <>
inline const BYTE* GRAYCALL StrArg<BYTE>(const char* pszStr) noexcept {  // static
    return PtrCast<const BYTE>(pszStr);
}

#ifdef _NATIVE_WCHAR_T_DEFINED
template <typename TYPE>
inline const TYPE* StrArg(const WORD* pszStr) {
    // auto convert WORD to a wchar_t type arg.
    return StrArg<TYPE>(reinterpret_cast<const wchar_t*>(pszStr));
}
#endif

#if 0  // def _NATIVE_WCHAR_T_DEFINED	// is wchar_t the same type as WORD ?
template <>
inline const BYTE* GRAYCALL StrArg<BYTE>(const wchar_t* pszStr) noexcept {  // static
    return (const BYTE*)StrArg<char>(pszStr);
}
template <>
inline const BYTE* GRAYCALL StrArg<BYTE>(const WORD* pszStr) noexcept {  // static
    return (const BYTE*)StrArg<char>((const wchar_t*)pszStr);
}
template <>
inline const WORD* GRAYCALL StrArg<WORD>(const WORD* pszStr) noexcept {  // static
    return pszStr;
}
template <>
inline const WORD* GRAYCALL StrArg<WORD>(const wchar_t* pszStr) noexcept {  // static
    return PtrCast<const WORD>(pszStr);
}
template <>
inline const WORD* GRAYCALL StrArg<WORD>(const char* pszStr) noexcept {  // static
    return (const WORD*)StrArg<wchar_t>(pszStr);
}
template <>
inline const WORD* GRAYCALL StrArg<WORD>(const BYTE* pszStr) noexcept {  // static
    return (const WORD*)StrArg<wchar_t>((const char*)pszStr);
}
#endif
}  // namespace Gray
#endif
