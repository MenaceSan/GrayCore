//
//! @file StrNum.h
//! convert numbers to/from string.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrNum_H
#define _INC_StrNum_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"

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

    static StrLen_t GRAYCALL GetTrimCharsLen(const char* pStr, StrLen_t nLen, char ch) noexcept;

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
    /// Similar to ::strtoul(). skip leading spaces. BUT NOT newlines. We should use StrNum::GetNumberString ?
    /// May have 0x# prefix to indicate hex
    /// @note TYPE* ppszInpEnd; return( ::strtol( pszInp, &ppszInpEnd, nBaseRadix ));
    ///  -gt- X digits is an overflow ?
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

    static StrLen_t GRAYCALL ULtoAK(UINT64 uVal, OUT char* pszOut, StrLen_t iStrMax, UINT nKUnit, bool bSpaceBeforeUnit = false);

    /// <summary>
    /// Internal function to format a number right justified as a string similar to sprintf("%u") padded from right.
    /// Padded from right. No lead padding.  upper case radix default.
    /// </summary>
    /// <param name="uVal"></param>
    /// <param name="pszOut"></param>
    /// <param name="iStrMax">must include space for null</param>
    /// <param name="nBaseRadix"></param>
    /// <param name="chRadixA"></param>
    /// <returns>First digit (most significant)</returns>
    static char* GRAYCALL ULtoARev(UINT64 uVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10, char chRadixA = 'A');

    static StrLen_t GRAYCALL ULtoA(UINT64 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10);
    static StrLen_t GRAYCALL ILtoA(INT64 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10);

    static StrLen_t GRAYCALL UtoA(UINT32 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10) {
        //! Just cast up to 64. k_LEN_MAX_DIGITS_INT
        return ULtoA(nVal, pszOut, iStrMax, nBaseRadix);
    }
    static StrLen_t GRAYCALL ItoA(INT32 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10) {
        //! Just cast up to 64. k_LEN_MAX_DIGITS_INT
        return ILtoA(nVal, pszOut, iStrMax, nBaseRadix);
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
    static StrLen_t GRAYCALL DtoAG(double dVal, OUT char* pszOut, StrLen_t iStrMax, int iDecPlacesWanted = -1, char chE = -'e');

    template <typename _TYPE>
    static _TYPE inline toValue(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr);
    template <typename _TYPE>
    static StrLen_t inline ValueToA(_TYPE val, OUT char* pszOut, StrLen_t iStrMax);

    template <typename _TYPE>
    static size_t GRAYCALL ToValArray(OUT _TYPE* pOut, size_t iQtyMax, const char* pszInp);

    template <typename _TYPE>
    static StrLen_t GRAYCALL ValArrayToA(OUT char* pszOut, StrLen_t nDstMax, const _TYPE* pSrc, size_t nSrcSize);

    template <typename _TYPE>
    static StrLen_t _cdecl ValArrayToAF(OUT char* pszDst, StrLen_t iSizeDstMax, size_t nSrcQty, ...);
};

template <>
inline INT32 StrNum::toValue<INT32>(const char* pszInp, const char** ppszInpEnd) {
    return CastN(INT32,StrNum::toIL(pszInp, ppszInpEnd));
}
template <>
inline UINT32 StrNum::toValue<UINT32>(const char* pszInp, const char** ppszInpEnd) {
    return CastN(UINT32,StrNum::toUL(pszInp, ppszInpEnd));
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
inline StrLen_t StrNum::ValueToA<INT32>(INT32 val, OUT char* pszOut, StrLen_t iStrMax) {
    // assume RADIX_t = 10
    return StrNum::ILtoA(val, pszOut, iStrMax);
}
template <>
inline StrLen_t StrNum::ValueToA<UINT32>(UINT32 val, OUT char* pszOut, StrLen_t iStrMax) {
    // assume RADIX_t = 10
    return StrNum::ULtoA(val, pszOut, iStrMax);
}
template <>
inline StrLen_t StrNum::ValueToA<INT64>(INT64 val, OUT char* pszOut, StrLen_t iStrMax) {
    // assume RADIX_t = 10
    return StrNum::ILtoA(val, pszOut, iStrMax);
}
template <>
inline StrLen_t StrNum::ValueToA<UINT64>(UINT64 val, OUT char* pszOut, StrLen_t iStrMax) {
    // assume RADIX_t = 10
    return StrNum::ULtoA(val, pszOut, iStrMax);
}
template <>
inline StrLen_t StrNum::ValueToA<float>(float val, OUT char* pszOut, StrLen_t iStrMax) {
    return StrNum::DtoAG(val, pszOut, iStrMax);
}
template <>
inline StrLen_t StrNum::ValueToA<double>(double val, OUT char* pszOut, StrLen_t iStrMax) {
    return StrNum::DtoAG(val, pszOut, iStrMax);
}

template <typename _TYPE>
size_t GRAYCALL StrNum::ToValArray(OUT _TYPE* pOut, size_t iQtyMax, const char* pszInp) {  // static
    //! @todo Merge with cMem::ReadFromCSV
    //! Similar to StrT::ParseArray()

    if (pszInp == nullptr) return 0;
    size_t i = 0;
    for (; i < iQtyMax;) {
        for (; StrChar::IsSpace(*pszInp); pszInp++) {
        }
        const char* pszInpStart = pszInp;
        if (*pszInpStart == '\0') break;
        pOut[i++] = StrNum::toValue<_TYPE>(pszInpStart, &pszInp);
        if (pszInpStart == pszInp) break;  // must be the field terminator? ")},;". End.
            
        for (; StrChar::IsSpace(*pszInp); pszInp++) {
        }
        if (pszInp[0] != ',') break;
        pszInp++;
    }
    return i;
}

template <typename _TYPE>
StrLen_t GRAYCALL StrNum::ValArrayToA(OUT char* pszDst, StrLen_t iSizeDstMax, const _TYPE* pSrc, size_t nSrcQty) {  // static
    //! @todo Merge with cMem::ConvertToString
    //! Write values out to a string as comma separated base 10 numbers.
    //! Try to use SetHexDigest() instead.
    //! opposite of cMem::ReadFromCSV().
    //! @return the actual size of the string.

    iSizeDstMax -= 4;  // room to terminate < max sized number.
    StrLen_t iLenOut = 0;
    for (size_t i = 0; i < nSrcQty; i++) {
        if (i > 0) pszDst[iLenOut++] = ',';
        StrLen_t iLenThis = StrNum::ValueToA<_TYPE>(pSrc[i], pszDst + iLenOut, iSizeDstMax - iLenOut);
        if (iLenThis <= 0) break;
        iLenOut += iLenThis;
        if (iLenOut >= iSizeDstMax) break;
    }
    return iLenOut;
}

template <typename _TYPE>
StrLen_t _cdecl StrNum::ValArrayToAF(OUT char* pszDst, StrLen_t iSizeDstMax, size_t nSrcQty, ...) {  // static
    //! @todo ValArrayToAF
    //! Write values out to a string as comma separated base 10 numbers.
    return -1;
}
}  // namespace Gray

#endif
