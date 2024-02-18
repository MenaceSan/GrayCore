//! @file StrArg.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrArg.h"
#include "StrT.h"
#include "StrU.h"
#include "cDebugAssert.h"
#include "cTempPool.h"

namespace Gray {
template <>
GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(const char* pszStrInp) {  // static
    //! Get a temporary string that only lives long enough to satisfy a sprintf() argument.
    //! @note the UNICODE size is variable and <= Len(pszStr)
    if (pszStrInp == nullptr) return __TOW("NULL");
    const auto spanSrc = StrT::ToSpanStr(pszStrInp);
    const StrLen_t lenU = StrU::UTF8toUNICODELen(spanSrc);  // needed UNICODE size is variable and <= Len(pszStr).
    auto spanTmp = cTempPool::GetSpan<wchar_t>(lenU);
    StrU::UTF8toUNICODE(spanTmp, spanSrc);  // true size is variable and < iLen
    return spanTmp;
}
template <>
GRAYCORE_LINK const char* GRAYCALL StrArg<char>(const wchar_t* pwStrInp) {  // static
    //! Get a temporary string that only lives long enough to satisfy a sprintf() argument.
    //! @note the UTF8 size is variable and >= Len(pwStr)
    if (pwStrInp == nullptr) return __TOA("NULL");
    const auto spanSrc = StrT::ToSpanStr(pwStrInp);
    const StrLen_t iLenOut = StrU::UNICODEtoUTF8Size(spanSrc);  // needed UTF8 size is variable and >= Len(pwStr)!
    auto spanTmp = cTempPool::GetSpan<char>(iLenOut);
    StrU::UNICODEtoUTF8(spanTmp, spanSrc);
    return spanTmp;
}

template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(TYPE ch, StrLen_t nRepeat) {  // static
    //! Get a temporary string that is nRepeat chars repeating
    auto spanTmp = cTempPool::GetSpan<TYPE>(nRepeat);
    cValSpan::FillQty<TYPE>(spanTmp.get_DataWork(), nRepeat, (TYPE)ch);
    spanTmp.get_DataWork()[nRepeat] = '\0';
    return spanTmp.get_DataWork();
}

template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(INT32 iVal) {
    //! Get a temporary string that only lives long enough to satisfy the sprintf()
    //! Assume auto convert char, short to int/INT32.
    TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 1];
    const StrLen_t nLen = StrT::ItoA(iVal, TOSPAN(szTmp), 10);
    return cTempPool::GetT<TYPE>(ToSpan(szTmp, nLen));
}
template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(UINT32 uVal, RADIX_t uRadix) {
    //! Get a temporary string that only lives long enough to satisfy the sprintf()
    //! Assume auto convert BYTE, WORD to UINT/UINT32/DWORD.
    TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 1];
    const StrLen_t nLen = StrT::UtoA(uVal, TOSPAN(szTmp), uRadix);
    return cTempPool::GetT<TYPE>(ToSpan(szTmp, nLen));
}

#ifdef USE_INT64
template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(INT64 iVal) {
    //! Get a temporary string that only lives long enough to satisfy the sprintf()
    TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 1];
    const StrLen_t nLen = StrT::ILtoA(iVal, TOSPAN(szTmp), 10);
    return cTempPool::GetT<TYPE>(ToSpan(szTmp, nLen));
}
template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(UINT64 uVal, RADIX_t uRadix) {
    //! Get a temporary string that only lives long enough to satisfy the sprintf()
    TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 1];
    const StrLen_t nLen = StrT::ULtoA(uVal, TOSPAN(szTmp), uRadix);
    return cTempPool::GetT<TYPE>(ToSpan(szTmp, nLen));
}
#endif

template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(double dVal) {
    //! Get a temporary string that only lives long enough to satisfy the sprintf()
    //! assume float gets converted to double.
    TYPE szTmp[StrNum::k_LEN_MAX_DIGITS + 1];
    const StrLen_t nLen = StrT::DtoA(dVal, TOSPAN(szTmp));
    return cTempPool::GetT<TYPE>(ToSpan(szTmp, nLen));
}

// force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(const char* pszStrInp);  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(const wchar_t* pszStrInp);     // Force implementation/instantiate for DLL/SO.

template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(wchar_t ch, StrLen_t nRepeat);  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(char ch, StrLen_t nRepeat);           // Force implementation/instantiate for DLL/SO.

template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(INT32 v);  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(INT32 v);        // Force implementation/instantiate for DLL/SO.

template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(UINT32 uVal, RADIX_t uRadix);  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(UINT32 uVal, RADIX_t uRadix);        // Force implementation/instantiate for DLL/SO.

#ifdef USE_INT64
template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(INT64 v);  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(INT64 v);        // Force implementation/instantiate for DLL/SO.

template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(UINT64 uVal, RADIX_t uRadix);  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(UINT64 uVal, RADIX_t uRadix);        // Force implementation/instantiate for DLL/SO.
#endif

template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(double v);  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(double v);        // Force implementation/instantiate for DLL/SO.
}  // namespace Gray
