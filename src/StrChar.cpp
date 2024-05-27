//! @file StrChar.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrA.h"
#include "StrChar.h"
#include "StrU.h"
#include "cLogMgr.h"
#include "cValT.h"

namespace Gray {
const char StrChar::k_Vowels[5] = {'a', 'e', 'i', 'o', 'u'};

bool GRAYCALL StrChar::IsVowel(wchar_t ch) noexcept {
    const wchar_t chName = StrChar::ToLowerW(ch);
    for (size_t x = 0; x < _countof(k_Vowels); x++) {
        if (chName == k_Vowels[x]) return true;
    }
    return false;
}

bool GRAYCALL StrChar::IsDigitF(wchar_t ch) noexcept {
    if (IsDigitA(ch)) return true;
    return StrT::HasChar(".eE-+", (char)ch);
}

bool GRAYCALL StrChar::IsDigitRadix(wchar_t ch, RADIX_t uRadix) noexcept {
    //! is digit in uRadix range? or Hex digit (if uRadix=16).
    //! replaces isxdigit() and istxdigit()
    DEBUG_CHECK(uRadix >= k_uRadixMin && uRadix <= k_uRadixMax);  // sane range.
    return Radix2U(ch) < uRadix;
}

char GRAYCALL StrChar::U2Radix(UINT uVal, RADIX_t uRadix) noexcept {  // static
    //! @return a single uRadix char for a value. Upper case.
    //! '?' = invalid value.
    ASSERT(uRadix >= k_uRadixMin && uRadix <= k_uRadixMax);  // sane range.
    if (uVal >= uRadix) return '?';                          // unknown char for this ?!
    if (uVal < 10) return (char)('0' + uVal);
    return (char)('A' + uVal - 10);
}

UINT GRAYCALL StrChar::Radix2U(wchar_t ch, RADIX_t uRadix) noexcept {  // static
    //! @return the value from the uRadix char
    //! -gte- uRadix = invalid char.
    UNREFERENCED_PARAMETER(uRadix);
    DEBUG_CHECK(uRadix >= k_uRadixMin && uRadix <= k_uRadixMax);  // sane range.
    const UINT uVal = Radix2U(ch);                                      
    if (uVal >= uRadix) return uRadix;  // invalid symbol for uRadix
    return uVal;
}

char GRAYCALL StrChar::U2Hex(UINT uVal) noexcept {  // static
    //! same as U2Radix(uVal,16)
    //! Convert a value to a single hex char. "0123456789ABCDEF"[uVal]  Upper case.
    //! '?' = invalid value.
    if (uVal <= 9) return CastN(char, '0' + uVal);
    if (uVal <= 15) return CastN(char, 'A' + uVal - 10);
    return '?';  // unknown?!
}

UINT GRAYCALL StrChar::Hex2U(wchar_t ch) noexcept {  // static
    //! same as Radix2U(ch,16)
    //! Get the value from the hex char
    //! 0x10 = invalid char.

    if (StrChar::IsDigitA(ch)) return ch - '0';        // StrChar::Dec2U
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;  // StrChar::IsUpperA(ch))
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;  // StrChar::IsLowerA(ch))
    return 0x10;  // invalid symbol.
}
}  // namespace Gray
