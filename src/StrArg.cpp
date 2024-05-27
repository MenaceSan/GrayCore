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
GRAYCORE_LINK cSpan<wchar_t> GRAYCALL StrArg2<wchar_t>(const cSpan<char>& src) noexcept {  // static
    const StrLen_t lenU = StrU::UTF8toUNICODELen(src);                                     // needed UNICODE size is variable and <= Len(pszStr).
    cSpanX<wchar_t> spanTmp = cTempPool::GetSpan<wchar_t>(lenU);                           // add space for '/0'
    StrU::UTF8toUNICODE(spanTmp, src);                                                     // true size is variable
    return cSpan<wchar_t>(spanTmp, lenU);
}
template <>
GRAYCORE_LINK cSpan<char> GRAYCALL StrArg2<char>(const cSpan<wchar_t>& src) noexcept {  // static
    const StrLen_t lenA = StrU::UNICODEtoUTF8Size(src);                                 // needed UTF8 size is variable and >= Len(pwStr)!
    cSpanX<char> spanTmp = cTempPool::GetSpan<char>(lenA);                              // add space for '/0'
    StrU::UNICODEtoUTF8(spanTmp, src);
    return cSpan<char>(spanTmp, lenA);
}

template <>
GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(const char* pszStrInp) noexcept {  // static
    if (pszStrInp == nullptr) return __TOW("NULL");
    return StrArg2<wchar_t>(StrT::ToSpanStr(pszStrInp));
}
template <>
GRAYCORE_LINK const char* GRAYCALL StrArg<char>(const wchar_t* pwStrInp) noexcept {  // static
    if (pwStrInp == nullptr) return __TOA("NULL");
    return StrArg2<char>(StrT::ToSpanStr(pwStrInp));
}

template <typename TYPE>
GRAYCORE_LINK const TYPE* GRAYCALL StrArg(TYPE ch, StrLen_t nRepeat) noexcept {  // static
    //! Get a temporary string that is nRepeat chars repeating
    auto spanTmp = cTempPool::GetSpan<TYPE>(nRepeat);
    cValSpan::FillQty<TYPE>(spanTmp.get_PtrWork(), nRepeat, (TYPE)ch);
    spanTmp.get_PtrWork()[nRepeat] = '\0';
    return spanTmp.get_PtrWork();
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
template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(const char* pszStrInp) noexcept;  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(const wchar_t* pszStrInp) noexcept;     // Force implementation/instantiate for DLL/SO.

template GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(wchar_t ch, StrLen_t nRepeat) noexcept;  // Force implementation/instantiate for DLL/SO.
template GRAYCORE_LINK const char* GRAYCALL StrArg<char>(char ch, StrLen_t nRepeat) noexcept;           // Force implementation/instantiate for DLL/SO.

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
