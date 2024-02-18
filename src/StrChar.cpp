//! @file StrChar.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrA.h"
#include "StrChar.h"
#include "StrU.h"
#include "cLogMgr.h"

namespace Gray {
const char StrChar::k_Vowels[5] = {'a', 'e', 'i', 'o', 'u'};

bool GRAYCALL StrChar::IsDigitF(wchar_t ch) {
    //! Float digit.
    if (StrChar::IsDigitA(ch)) return true;
    if (StrT::HasChar(".eE-+", (char)ch)) return true;
    return false;
}

bool GRAYCALL StrChar::IsDigitX(wchar_t ch, RADIX_t uRadix) {
    //! is digit in uRadix range? or Hex digit (if uRadix=16).
    //! replaces isxdigit() and istxdigit()

    const RADIX_t wRadix10 = (uRadix > 10) ? 10U : uRadix;
    if (ch >= '0' && ch < (wchar_t)('0' + wRadix10)) return true;
    uRadix -= 10;
    if (ch >= ((wchar_t)'a') && ch < (wchar_t)('a' + uRadix)) return true;
    if (ch >= ((wchar_t)'A') && ch < (wchar_t)('A' + uRadix)) return true;
    return false;
}

bool GRAYCALL StrChar::IsVowel(wchar_t ch) {
    const wchar_t chName = StrChar::ToLowerW(ch);
    for (size_t x = 0; x < _countof(k_Vowels); x++) {
        if (chName == k_Vowels[x]) return true;
    }
    return false;
}

char GRAYCALL StrChar::U2Radix(UINT uVal, RADIX_t uRadix) {  // static
    //! @return a single uRadix char for a value. Upper case.
    //! '?' = invalid value.
    ASSERT(uRadix >= k_uRadixMin && uRadix <= k_uRadixMax);  // sane range.
    if (uVal >= uRadix) return '?';                          // unknown char for this ?!
    if (uVal < 10) return (char)('0' + uVal);
    return (char)('A' + uVal - 10);
}
int GRAYCALL StrChar::Radix2U(wchar_t ch, RADIX_t uRadix) {  // static
    //! @return the value from the uRadix char
    //! -1 = invalid char.
    UNREFERENCED_PARAMETER(uRadix);
    DEBUG_CHECK(uRadix >= k_uRadixMin && uRadix <= k_uRadixMax);  // sane range.
    int iVal;
    if (StrChar::IsDigitA(ch))
        iVal = ch - '0';    // StrChar::Dec2U
    else if (IsUpperA(ch))  // allow > Radix ?
        iVal = ch - 'A' + 10;
    else if (IsLowerA(ch))  // allow > Radix ?
        iVal = ch - 'a' + 10;
    else                            // > uRadix
        return -1;                  // invalid symbol for uRadix
    if (iVal >= uRadix) return -1;  // invalid symbol for uRadix
    return iVal;
}

char GRAYCALL StrChar::U2Hex(UINT uVal) {  // static
    //! same as U2Radix(uVal,16)
    //! Convert a value to a single hex char. "0123456789ABCDEF"[uVal]  Upper case.
    //! '?' = invalid value.
    if (uVal <= 9) return CastN(char, '0' + uVal);
    if (uVal <= 15) return CastN(char, 'A' + uVal - 10);
    return '?';  // unknown?!
}

int GRAYCALL StrChar::Hex2U(wchar_t ch) {  // static
    //! same as Radix2U(ch,16)
    //! Get the value from the hex char
    //! -1 = invalid char.

    if (StrChar::IsDigitA(ch))
        return ch - '0';              // StrChar::Dec2U
    else if (ch >= 'A' && ch <= 'F')  // StrChar::IsUpperA(ch))
        return ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'f')  // StrChar::IsLowerA(ch))
        return ch - 'a' + 10;
    return -1;  // invalid symbol.
}
}  // namespace Gray
