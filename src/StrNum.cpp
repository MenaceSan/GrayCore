//! @file StrNum.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrNum.h"
#include "StrT.h"
#include "cFloat.h"
#include "cFloatDeco.h"
#include "cTypes.h"
#include "cValSpan.h"

namespace Gray {
StrLen_t GRAYCALL StrNum::GetTrimCharsLen(const cSpan<char>& src, char ch) noexcept {  // static
    //! Get Length of string if all ch chars are trimmed from the end.
    if (src.isNull()) return 0;
    StrLen_t nLen = src.get_MaxLen();
    for (; nLen > 0; nLen--) {
        if (src.get_PtrConst()[nLen - 1] != ch) break;
    }
    return nLen;
}

StrLen_t GRAYCALL StrNum::GetNumberString(cSpanX<char> ret, const wchar_t* pszInp, RADIX_t uRadix) noexcept {  // static
    if (pszInp == nullptr) return 0;
    char* pszOut = ret.get_PtrWork();
    StrLen_t nLen = 0;
    for (; nLen < ret.get_MaxLen() - 1; nLen++) {
        // Is this a possible number?
        const wchar_t ch = pszInp[nLen];
        if (uRadix) {
            if (ch == 'x' && nLen && pszInp[nLen - 1] == '0') {  // allow "0x" prefix
                uRadix = 16;
            } else if (ch == '-' && nLen == 0 && uRadix == 10) {
            } else if (!StrChar::IsDigitRadix(ch, uRadix)) {
                break;  // not a number!
            }
        } else {
            if (ch <= ' ' || ch >= StrChar::k_ASCII) break;  // not a number! for Float!
            // if (ch == ':' || ch == ';' || ch == '(' || ch == ')')	// not a number!
            // allow and characters that might be part of a number. Digit, '.', 'e','E','+', '-', 'A' - 'Z', 'a' - 'z' for hex values,
            // allow comma and math separators?
        }
        pszOut[nLen] = (char)ch;
    }

    pszOut[nLen] = '\0';  // Terminate.
    return nLen;
}

//*************************************************************************************

UINT64 GRAYCALL StrNum::toUL(const char* pszInp, const char** ppszInpEnd, RADIX_t nRadixBase) noexcept {  // static
    bool bFlexible = false;                                                                               // allow hex ?
    if (nRadixBase < StrChar::k_uRadixMin) {
        bFlexible = true;
        nRadixBase = 10;  // default.
    }
    ASSERT(nRadixBase <= StrChar::k_uRadixMax);
    if (pszInp == nullptr) return 0;

    pszInp = StrT::GetNonWhitespace(pszInp);
    char ch = pszInp[0];
    if (ch == '0') {
        ch = pszInp[1];
        if (ch == 'X' || ch == 'x') {
            // its really just hex!
            nRadixBase = 16;
            pszInp += 2;
        } else if (bFlexible) {  // Is this octal or hex ?
            nRadixBase = 16;
            pszInp++;
        }
    }

    UINT64 uVal = 0;
    for (;;) {
        const UINT uValCh = StrChar::Radix2U(*pszInp);
        if (uValCh >= nRadixBase) break;  // not valid character for this radix. end.
        uVal *= nRadixBase;
        uVal += uValCh;
        pszInp++;
    }

    if (ppszInpEnd != nullptr) *ppszInpEnd = pszInp;

    return uVal;
}

INT64 GRAYCALL StrNum::toIL(const char* pszInp, const char** ppszInpEnd, RADIX_t nRadixBase) noexcept {
    //! convert string to integer value. like strtol(), or a bit like atoi()
    //! May have 0x# prefix to indicate hex
    if (pszInp == nullptr) return 0;
    pszInp = StrT::GetNonWhitespace(pszInp);
    if (pszInp[0] == '-') {
        return -CastN(INT64, StrNum::toUL(pszInp + 1, ppszInpEnd, nRadixBase));
    }
    return CastN(INT64, StrNum::toUL(pszInp, ppszInpEnd, nRadixBase));
}

//*************************************************************************************

StrLen_t GRAYCALL StrNum::ULtoAK(UINT64 uVal, cSpanX<char> ret, UINT nKUnit, bool bSpaceBeforeUnit) {
    static const char k_chKUnits[10] = "\0KMGTPEZY";
    if (nKUnit <= 0) nKUnit = 1024;  // default.

    double dVal = (double)uVal;
    size_t i = 0;
    for (; i < (_countof(k_chKUnits) - 2) && dVal >= nKUnit; i++) {
        dVal /= nKUnit;
    }

    StrLen_t iLenUse;
    if (i == 0) {
        iLenUse = ULtoA(uVal, ret);  // _CVTBUFSIZE
    } else {
        // limit to 2 decimal places ?
        iLenUse = DtoAG(dVal, ret, 2, '\0');  // NOT 'inf' or NaN has no units.
        char* pszOut = ret.get_PtrWork();
        if (iLenUse > 0 && iLenUse <= ret.get_MaxLen() - 3 && !StrChar::IsAlpha(pszOut[0])) {
            if (bSpaceBeforeUnit) {  // insert a space or not?
                pszOut[iLenUse++] = ' ';
            }
            pszOut[iLenUse++] = k_chKUnits[i];
            pszOut[iLenUse] = '\0';
        }
    }
    return iLenUse;
}

#if 0
StrLen_t GRAYCALL StrNum::GetSizeRadix(UINT64 uVal, RADIX_t nRadixBase) {   // static
    // How many nRadixBase digits for this number?
    // TODO Highest1Bit for powers of 2? IsMask1()
    // like: GetCountDecimalDigit32()
    StrLen_t i=0;
    for (;uVal>0; i++) {
        uVal /= nRadixBase;
    }
    return i;
}
#endif

StrLen_t GRAYCALL StrNum::ULtoA(UINT64 uVal, cSpanX<char> ret, RADIX_t nRadixBase) {
    char szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 2];  // bits in int is all we really need max. (i.e. nRadixBase=2)
    cSpanX<char> spanDigits = ULtoARev(uVal, szTmp, STRMAX(szTmp), nRadixBase);

    char* pDigits = spanDigits.get_PtrWork();
    const StrLen_t iStrMax = cValT::Min(ret.get_MaxLen(), STRMAX(szTmp));

    if (nRadixBase == 16 && uVal != 0) {  // give hex a leading 0 if there is room. except if its 0 value. m_bLeadZero
        if (spanDigits.get_MaxLen() < iStrMax) {
            *(--pDigits) = '0';  // prefix with 0 if room.
        }
    }
    return StrT::CopyLen(ret.get_PtrWork(), pDigits, iStrMax);
}

StrLen_t GRAYCALL StrNum::ILtoA(INT64 nVal, cSpanX<char> ret, RADIX_t nRadixBase) {
    if (ret.isEmpty()) return 0;
    if (nVal < 0) {
        nVal = -nVal;
        ret.get_PtrWork()[0] = '-';
        ret.SetSkipBytes(1);
    }
    char szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 2];  // bits in int is all we really need max. (i.e. nRadixBase=2 + sign + '\0')
    cSpan<char> spanDigits = ULtoARev((UINT64)nVal, szTmp, STRMAX(szTmp), nRadixBase);
    return StrT::Copy(ret, spanDigits.get_PtrConst());
}

//*************************************************************************************

StrLen_t GRAYCALL StrNum::DtoAG2(double dVal, OUT char* pszOut, int iDecPlacesWanted, char chE) {  // static
    // Not handling NaN and Infinite
    ASSERT(cFloat::IsFinite(dVal));
    // @todo implement gcvt(), fcvt(), _fcvt_s locally ? no UNICODE version of fcvt().

    if (dVal < 0) {
        pszOut[0] = '-';
        return 1 + DtoAG2(-dVal, pszOut + 1, iDecPlacesWanted, chE);
    }

    int nExp10 = 0;  // decimal exponent
    StrLen_t nMantLength = cFloatDeco::Grisu2(dVal, pszOut, OUT nExp10);
    ASSERT(nMantLength > 0);

    StrLen_t nOutLen;
    if (chE < '\0') {
        // "%g" or "%G"
        // iDecPlacesWanted = precision = total mantissa digits (not true decimal places).
        if (iDecPlacesWanted >= 0 && iDecPlacesWanted < nMantLength) {
            // Chop off decimal places.
            nExp10 += nMantLength - iDecPlacesWanted;
            nMantLength = cFloatDeco::MantRound(pszOut, iDecPlacesWanted);
        }

        // Decide what format to use for the number. like gcvt() selects F or E format.
        // 26 digits. mid point = 8; +/- 13
        const StrLen_t nDecPlaceO = nMantLength + nExp10;  // 10^(nDecPlaceO-1) <= v < 10^nDecPlaceO
        if (nDecPlaceO >= -5 && nDecPlaceO <= 21) {        // 26 digits. mid point = 8; +/- 13
            // Prefer F format
            iDecPlacesWanted = (nExp10 >= 0) ? 0 : -1;                                     // Whole numbers only. No decimal places.
            nOutLen = cFloatDeco::FormatF(pszOut, nMantLength, nExp10, iDecPlacesWanted);  // iDecPlacesWanted
        } else {
            // Prefer E Format.
            nOutLen = cFloatDeco::FormatE(pszOut, nMantLength, nExp10, -chE);
        }
    } else if (chE != '\0') {
        // E format. always has 1 whole digit.
        if (iDecPlacesWanted >= 0) {  // restrict decimal places.
            const StrLen_t iDelta = cFloatDeco::MantAdjust(pszOut, nMantLength, iDecPlacesWanted + 1);
            nMantLength += iDelta;
            nExp10 -= iDelta;
        }
        nOutLen = cFloatDeco::FormatE(pszOut, nMantLength, nExp10, chE);
    } else {
        // F format.
        if (nExp10 >= 0 && iDecPlacesWanted < 0) iDecPlacesWanted = 1;  // Whole numbers only. No decimal places. // default = have at least 1 decimal place.
        nOutLen = cFloatDeco::FormatF(pszOut, nMantLength, nExp10, iDecPlacesWanted);
    }

    ASSERT(pszOut[nOutLen] == '\0');
    return nOutLen;
}

StrLen_t GRAYCALL StrNum::DtoAG(double dVal, cSpanX<char> ret, int iDecPlacesWanted, char chE) {  // static
    if (ret.get_MaxLen() >= k_LEN_MAX_DIGITS) {                                                   // buffer is big enough.
        return DtoAG2(dVal, ret.get_PtrWork(), iDecPlacesWanted, chE);
    }
    char szTmp[StrNum::k_LEN_MAX_DIGITS + 4];    // MUST allow at least this size.
    DtoAG2(dVal, szTmp, iDecPlacesWanted, chE);  // StrLen_t iStrLen =
    return StrT::Copy(ret, szTmp);               // Copy to smaller buffer. truncate.
}

double GRAYCALL StrNum::toDouble(const char* pszInp, const char** ppszInpEnd) {  // static
    //! Convert a string to a double precision decimal value. k_LEN_MAX_DIGITS
    //! MUST emulate/match the C compiler. The C++ compiler will generate a double value from a string in code.
    //! MUST be reversible using DtoA().
    //! like atof(), wcstod() or strtod() and opposite of DtoA(), dtoa()
    //! don't bother with toFloat() since it will do the same thing.
    //! e.g. decode stuff in format. "12.2", ".12", "123e234",
    //!
    //! http://www.cplusplus.com/reference/cstdlib/strtod/
    //! see http://www.exploringbinary.com/how-strtod-works-and-sometimes-doesnt/ for why this is dangerous/difficult.
    //! http://www.opensource.apple.com/source/tcl/tcl-10/tcl/compat/strtod.c
    //! It can have rounding problems? e.g. "3.1416" = 3.1415999999999999. Deal with this issue on conversion to string.
    //! http://www.ampl.com/netlib/fp/dtoa.c (David Gay)

    if (pszInp == nullptr) return 0;

    // Strip off leading blanks and check for a sign.
    pszInp = StrT::GetNonWhitespace(pszInp);

    const char* pszStart = pszInp;  // Sign would be here.
    char ch = *pszInp;
    if (ch == '-' || ch == '+') ++pszInp;

    StrLen_t nSizeMant = 0;  // Number of digits in mantissa.
    StrLen_t nSizeInt = -1;  // Number of mantissa digits BEFORE decimal point.
    for (;; nSizeMant++, pszInp++) {
        ch = *pszInp;
        if (StrChar::IsDigitA(ch)) continue;
        if ((ch != '.') || (nSizeInt >= 0)) break;  // First and only decimal point.
        nSizeInt = nSizeMant;                       // Found the decimal point.
    }

    // Now suck up the digits in the mantissa.
    // Use two integers to collect 9 digits each (this is faster than using floating-point).
    // If the mantissa has more than 18 digits, ignore the extras, since they can't affect the value anyway.

    const char* pszExp = pszInp;
    pszInp -= nSizeMant;       // back to start.
    if (nSizeInt < 0)          // had no decimal place.
        nSizeInt = nSizeMant;  // Put decimal point at the end.
    else
        nSizeMant--;  // One of the digits was the point. drop it.

    int nExpFrac;  // Exponent that derives from the fractional part
    if (nSizeMant > 18) {
        // Ignore unusable mantissa accuracy.
        nExpFrac = nSizeInt - 18;
        nSizeMant = 18;  // truncate
    } else {
        if (nSizeMant == 0) {  // No value? maybe just a decimal place. thats odd.
            if (ppszInpEnd != nullptr) {
                *ppszInpEnd = (char*)pszStart;  // Nothing here that was a number.
            }
            return 0.0;
        }
        nExpFrac = nSizeInt - nSizeMant;
    }

    // Do math as 2 integers for speed. like GetFixedIntRef()
    UINT32 fracHi = 0;
    for (; nSizeMant > 9;) {
        ch = *pszInp;
        pszInp++;
        if (ch != '.') {  // Just skip .
            fracHi = 10 * fracHi + StrChar::Dec2U(ch);
            nSizeMant--;
        }
    }

    UINT32 fracLo = 0;
    for (; nSizeMant > 0;) {
        ch = *pszInp;
        pszInp++;
        if (ch != '.') {  // Just skip .
            fracLo = 10 * fracLo + StrChar::Dec2U(ch);
            nSizeMant--;
        }
    }

    // Skim off the exponent.
    pszInp = pszExp;  // nSizeMant may have been truncated to 18.
    ch = *pszInp;
    bool bExpNegative = false;
    int nExp = 0;  // Exponent read from "e" field.
    if (ch == 'E' || ch == 'e') {
        pszInp++;
        ch = *pszInp;
        if (ch == '-') {
            bExpNegative = true;
            pszInp++;
        } else if (ch == '+') {
            pszInp++;
        }

        const char* pszExp2 = pszInp;
        while (StrChar::IsDigitA(*pszInp)) {
            nExp = nExp * 10 + StrChar::Dec2U(*pszInp);
            pszInp++;
        }
        if (pszInp == pszExp2) {
            // e NOT followed by a valid number. Ignore it.
            ASSERT(nExp == 0);
            pszInp = pszExp;
        }
    }

    if (ppszInpEnd != nullptr) {
        *ppszInpEnd = (char*)pszInp;
    }

    const double fraction = cFloatDeco::toDouble(fracHi, fracLo, (bExpNegative) ? (nExpFrac - nExp) : (nExpFrac + nExp));
    return (*pszStart == '-') ? (-fraction) : fraction;
}
}  // namespace Gray
