//! @file StrNum.h
//! convert numbers to/from string.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrNum_H
#define _INC_StrNum_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"
#include "cSpan.h"

namespace Gray {
/// <summary>
/// Convert ASCII (8 bit) string to/from numbers.
/// Numbers are in a restricted set of ASCII (8 bit) characters.
/// It doesn't make sense to have UTF8 and UNICODE versions of these. All actions are in 'char' type.
/// Just convert to/from UNICODE as needed using StrU::UTF8toUNICODE and GetNumberString.
/// </summary>
struct GRAYCORE_LINK StrNum {                             // static
    static const StrLen_t k_LEN_MAX_DIGITS = (309 + 40);  /// Largest number we can represent in double format + some extra places for post decimal. (). like _CVTBUFSIZE or k_LEN_MAX_CSYM
    static const StrLen_t k_LEN_MAX_DIGITS_INT = 64;      /// Largest 64 bits base 2 not including sign or '\0' is only 64 digits.

    static StrLen_t GRAYCALL GetTrimCharsLen(const cSpan<char>& src, char ch) noexcept;

    /// <summary>
    /// convert a UNICODE string to ASCII string that represents a number.
    /// Only get numeric type chars.
    /// Like a simple (numbers only) version of StrU::UNICODEtoUTF8() and Opposite of StrU::UTF8toUNICODE()
    /// @note ASSUME pszOut[k_LEN_MAX_DIGITS]
    /// </summary>
    /// <returns>the length of the string that might be a number. Hit end or first char that cant be a number.</returns>
    static StrLen_t GRAYCALL GetNumberString(OUT char* pszOut, const wchar_t* pszInp, StrLen_t iStrMax = k_LEN_MAX_DIGITS) noexcept;

    /// <summary>
    /// Parse a string and convert to a number.
    /// Similar to strtoul(), strtol(). skip leading spaces. BUT NOT newlines. We should use StrNum::GetNumberString ?
    /// May have "0x" prefix to indicate hex
    /// -gt- X digits is an overflow ?
    /// </summary>
    /// <param name="pszInp"></param>
    /// <param name="ppszInpEnd">the non number digit at the end . might be '\0';</param>
    /// <param name="nBaseRadix">Radix, 0 = default to 10 and allow the string to override this. '0x' prefix will override.</param>
    /// <returns></returns>
    static UINT64 GRAYCALL toUL(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr, RADIX_t nBaseRadix = 0) noexcept;
    static INT64 GRAYCALL toIL(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr, RADIX_t nBaseRadix = 10) noexcept;

    static UINT32 GRAYCALL toU(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr, RADIX_t nBaseRadix = 0) noexcept {
        //! Read number from string. Just cast down from 64.
        return (UINT32)toUL(pszInp, ppszInpEnd, nBaseRadix);
    }
    static INT32 GRAYCALL toI(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr, RADIX_t nBaseRadix = 10) noexcept {
        //! Just cast down from 64.
        return (INT32)toIL(pszInp, ppszInpEnd, nBaseRadix);
    }

    /// <summary>
    /// Make string describing a value in K/M/G/T/P/E/Z/Y units. (Kilo,Mega,Giga,Tera,Peta,Exa,Zetta,Yotta)
    /// @note Kilo=10^3,2^10, Exa=10^18,2^60, Zetta=10^21,2^70, (http://searchstorage.techtarget.com/sDefinition/0,,sid5_gci499008,00.html)
    /// </summary>
    /// <param name="uVal"></param>
    /// <param name="pszOut"></param>
    /// <param name="iStrMax"></param>
    /// <param name="nKUnit">1024 default</param>
    /// <param name="bSpaceBeforeUnit"></param>
    /// <returns>length of string created.</returns>
    static StrLen_t GRAYCALL ULtoAK(UINT64 uVal, cSpanX<char>& ret, UINT nKUnit, bool bSpaceBeforeUnit = false);

    /// <summary>
    /// Internal function to format a number right justified as a string similar to sprintf("%u") padded from right.
    /// Padded from right. No lead padding.  upper case radix default.
    /// </summary>
    /// <param name="uVal"></param>
    /// <param name="pszOut"></param>
    /// <param name="iStrMax">must include space for null</param>
    /// <param name="nBaseRadix">10</param>
    /// <param name="chRadixA">'A'</param>
    /// <returns>First digit (most significant)</returns>
    template <typename TYPE>
    static cSpan<TYPE> GRAYCALL ULtoARev(UINT64 uVal, OUT TYPE* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10, char chRadixA = 'A') {
        ASSERT_NN(pszOut);
        ASSERT(iStrMax > 0);
        ASSERT(nBaseRadix <= StrChar::k_uRadixMax);
        if (nBaseRadix < StrChar::k_uRadixMin) nBaseRadix = 10;
        TYPE* pEnd = pszOut + iStrMax;
        *pEnd = '\0';
        // TODO ? Shortcut for nBaseRadix = 16 = no modulus
        TYPE* pDigits = pEnd;
        do {
            if (pDigits <= pszOut) break;  // Overflow ! This truncates front / high values?!
            
            const UINT64 d = uVal % nBaseRadix;
            *(--pDigits) = CastN(TYPE, d + (d < 10 ? '0' : (chRadixA - 10)));  // StrChar::U2Radix
            uVal /= nBaseRadix;
        } while (uVal);  // done?
        return ToSpan(pDigits, StrT::Diff(pEnd, pDigits));
    }

    /// <summary>
    /// Format a number as a string similar to sprintf("%u")  upper case radix default.
    /// like _itoa(iValue,pszOut,iRadix), FromInteger() and RtlIntegerToUnicodeString() or _itoa_s()
    /// Leading zero on hex string. (if room)
    /// </summary>
    /// <param name="nVal"></param>
    /// <param name="pszOut"></param>
    /// <param name="iStrMax">_CVTBUFSIZE = _countof(Dst) = includes room for '\0'. (just like cMem::Copy)</param>
    /// <param name="nBaseRadix">10 default</param>
    /// <returns>length of the string.</returns>
    static StrLen_t GRAYCALL ULtoA(UINT64 nVal, cSpanX<char>& ret, RADIX_t nBaseRadix = 10);

    /// <summary>
    /// Make a string from a number. like ltoa(). upper case radix default.
    /// </summary>
    /// <param name="nVal"></param>
    /// <param name="pszOut"></param>
    /// <param name="iStrMax">_countof(Dst) = includes room for '\0'. (just like cMem::Copy)</param>
    /// <param name="nBaseRadix"></param>
    /// <returns>length of the string.</returns>
    static StrLen_t GRAYCALL ILtoA(INT64 nVal, cSpanX<char>& ret, RADIX_t nBaseRadix = 10);

    static StrLen_t GRAYCALL UtoA(UINT32 nVal, cSpanX<char>& ret, RADIX_t nBaseRadix = 10) {
        //! Just cast up to 64. k_LEN_MAX_DIGITS_INT
        return ULtoA(nVal, ret, nBaseRadix);
    }
    static StrLen_t GRAYCALL ItoA(INT32 nVal, cSpanX<char>& ret, RADIX_t nBaseRadix = 10) {
        //! Just cast up to 64. k_LEN_MAX_DIGITS_INT
        return ILtoA(nVal, ret, nBaseRadix);
    }

    static double GRAYCALL toDouble(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr);

    /// <summary>
    /// Make a string from a double float number. like _gcvt() or "%g"
    /// like dtoa(), gcvt(), fcvt(), ecvt(), _fcvt() and the opposite of toDouble(),
    /// e.g. 123.449997 should equal "123.45" ?
    //! @note _WIN32 wsprintf() does NOT do floats of any sort!!!
    //!		there is no UNICODE version of fcvt()
    //! @note %g looks nice but can lose precision! Non reversible.
    /// </summary>
    /// <param name="dVal"></param>
    /// <param name="pszOut">ASSUME size >= k_LEN_MAX_DIGITS</param>
    /// <param name="iDecPlacesWanted">count of decimal places i desire. Number of digits after the decimal point. -1 = don't care.</param>
    /// <param name="chE">0=%f, -lt- 0=%g, -gt- 0=%e</param>
    /// <returns>length of the string. pszOut = 'inf' or 'NaN'</returns>
    static StrLen_t GRAYCALL DtoAG2(double dVal, OUT char* pszOut, int iDecPlacesWanted = -1, char chE = -'e');
    static StrLen_t GRAYCALL DtoAG(double dVal, cSpanX<char>& ret, int iDecPlacesWanted = -1, char chE = -'e');

    template <typename _TYPE>
    static _TYPE inline toValue(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr);
    template <typename _TYPE>
    static StrLen_t inline ValueToA(cSpanX<char>& ret, _TYPE val);

    template <typename _TYPE>
    static size_t GRAYCALL ToValArray(cSpanX<_TYPE>& ret, const char* pszInp);
};

template <>
inline INT32 StrNum::toValue<INT32>(const char* pszInp, const char** ppszInpEnd) {
    return CastN(INT32, StrNum::toIL(pszInp, ppszInpEnd));
}
template <>
inline UINT32 StrNum::toValue<UINT32>(const char* pszInp, const char** ppszInpEnd) {
    return CastN(UINT32, StrNum::toUL(pszInp, ppszInpEnd));
}
template <>
inline INT64 StrNum::toValue<INT64>(const char* pszInp, const char** ppszInpEnd) {
    return StrNum::toIL(pszInp, ppszInpEnd);
}
template <>
inline UINT64 StrNum::toValue<UINT64>(const char* pszInp, const char** ppszInpEnd) {
    return StrNum::toUL(pszInp, ppszInpEnd);
}
template <>
inline float StrNum::toValue<float>(const char* pszInp, const char** ppszInpEnd) {
    return CastN(float, StrNum::toDouble(pszInp, ppszInpEnd));
}
template <>
inline double StrNum::toValue<double>(const char* pszInp, const char** ppszInpEnd) {
    return StrNum::toDouble(pszInp, ppszInpEnd);
}

template <>
inline StrLen_t StrNum::ValueToA<INT32>(cSpanX<char>& ret, INT32 val) {
    // assume RADIX_t = 10
    return StrNum::ILtoA(val, ret);
}
template <>
inline StrLen_t StrNum::ValueToA<UINT32>(cSpanX<char>& ret, UINT32 val) {
    // assume RADIX_t = 10
    return StrNum::ULtoA(val, ret);
}
template <>
inline StrLen_t StrNum::ValueToA<INT64>(cSpanX<char>& ret, INT64 val) {
    // assume RADIX_t = 10
    return StrNum::ILtoA(val, ret);
}
template <>
inline StrLen_t StrNum::ValueToA<UINT64>(cSpanX<char>& ret, UINT64 val) {
    // assume RADIX_t = 10
    return StrNum::ULtoA(val, ret);
}
template <>
inline StrLen_t StrNum::ValueToA<float>(cSpanX<char>& ret, float val) {
    return StrNum::DtoAG(val, ret);
}
template <>
inline StrLen_t StrNum::ValueToA<double>(cSpanX<char>& ret, double val) {
    return StrNum::DtoAG(val, ret);
}

template <typename _TYPE>
size_t GRAYCALL StrNum::ToValArray(cSpanX<_TYPE>& ret, const char* pszInp) {  // static
    // Parse string
    //! @TODO Merge with cMemSpan::ReadFromCSV
    //! Similar to StrT::ParseArray()

    if (ret.isEmpty()) return 0;
    size_t i = 0;
    for (; i < ret.get_Count();) {
        for (; StrChar::IsSpace(*pszInp); pszInp++) {
        }
        const char* pszInpStart = pszInp;
        if (*pszInpStart == '\0') break;
        ret.get_DataWork()[i++] = StrNum::toValue<_TYPE>(pszInpStart, &pszInp);
        if (pszInpStart == pszInp) break;  // must be the field terminator? ")},;". End.

        for (; StrChar::IsSpace(*pszInp); pszInp++) {
        }
        if (pszInp[0] != ',') break;
        pszInp++;
    }
    return i;
}

}  // namespace Gray

#endif
