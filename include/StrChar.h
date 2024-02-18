//! @file StrChar.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrChar_H
#define _INC_StrChar_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cValSpan.h"

#ifdef _WIN32
#include <wtypes.h>  // BSTR

#elif defined(__linux__)
#include <wchar.h>  // wcslen()
#else

#error NOOS
#endif

namespace Gray {
typedef BYTE RADIX_t;  /// Base for convert of numbers to strings. e.g. 10 base vs 16 base hex numbers.

#ifdef _WIN32
typedef UINT CODEPAGE_t;  /// text code page = a limited set of characters (not UNICODE). CP_ACP=default ANSI, CP_OEMCP, CP_UTF8
#else
enum CODEPAGE_t {
    CP_ACP = 0,       /// default to ANSI code page. All the _WIN32 A suffix functions.
    CP_OEMCP = 1,     /// default to OEM  code page
    CP_UTF8 = 65001,  /// UTF-8 translation
};
#endif

/// <summary>
/// Replace some of the std lib char functions because they can crash on chars outside range.
/// POSIX __linux__ and _WIN32 calls are not always the same.
/// ASSUME ASCII char set, extended ASCII, UNICODE or UTF8
/// </summary>
struct GRAYCORE_LINK StrChar {  // static

    static const char k_BEL = '\a';  /// Audible bell = 7
    static const char k_BS = '\b';   /// Backspace = 8
    static const char k_HT = '\t';   /// Horizontal tab = 9
    static const char k_NL = '\n';   /// New line, line feed = 0x0a = 10
    static const char k_VT = '\v';   /// Vertical tab = 0x0b = 11
    static const char k_FF = '\f';   /// Form feed, new page = 0x0c = 12
    static const char k_CR = '\r';   /// Carriage return = 0x0d = 13

    // Other C escape chars are "\'\"\?\\"

    static const char k_Space = ' ';  /// space = start of the visible ASCII set. 0x20 = 32
    static const char k_ASCII = 127;  /// Max for normal ASCII characters.

    static const RADIX_t k_uRadixMin = 2;        /// binary
    static const RADIX_t k_uRadixDef = 10;       /// base 10.
    static const RADIX_t k_uRadixMax = 10 + 26;  /// arbitrary max of numbers + letters. Allow Base64 ??

    static const char k_Vowels[5];  /// Does not include Y. AEIOU

    /// Some compilers consider char to be signed. Ignore that.
    static constexpr wchar_t ToW(char c) noexcept {
        return CastN(BYTE, c);  // remove sign.
    }
    static constexpr wchar_t ToW(wchar_t c) noexcept {
        return c;  // do nothing. supports templates.
    }

    static constexpr bool IsAscii(wchar_t ch) noexcept {
        //! like POSIX isascii()
        return CastN(unsigned, ch) <= k_ASCII;
    }

    /// <summary>
    /// printable? -gt- k_ASCII is not printable or -lt- ' '. e.g. DEL.
    /// NOT iswprint() ? NOT IsSpaceX()
    /// </summary>
    static constexpr bool IsPrintA(wchar_t ch) noexcept {
        return ch >= k_Space && ch < k_ASCII;
    }
    static constexpr bool IsAlNum(wchar_t ch) noexcept {
        //! a-z, 0-9
        return IsAlphaA(ch) || IsDigitA(ch);
    }

    // single space, tab, vertical tab, form feed, carriage return, or newline
    static constexpr bool IsNL(wchar_t ch) noexcept {
        //! is newline.
        return ch == k_NL || ch == k_CR;
    }
    static constexpr bool IsSpace(wchar_t ch) noexcept {
        //! is a horizontal separator space. not newline.
        return ch == k_HT || ch == k_Space;
    }
    /// <summary>
    /// Is this any sort of spacer ? Horizontal or vertical space or new lines.
    /// almost same as C isspace() = "' ','\t','\n'" etc. but we also include "'\b'".
    /// https://en.wikipedia.org/wiki/Newline
    /// NEL: Next Line, U + 0085.
    /// LS : Line Separator, U + 2028.
    /// PS : Paragraph Separator, U + 2029.
    /// </summary>
    /// <param name="ch"></param>
    /// <returns></returns>
    static constexpr bool IsSpaceX(wchar_t ch) noexcept {
        return ch == k_Space || (ch >= k_BS && ch <= k_CR);
    }

    /// <summary>
    /// same as C isdigit(). Does not include 1/2, etc.
    /// </summary>
    /// <param name="ch"></param>
    /// <returns></returns>
    static constexpr bool IsDigitA(wchar_t ch) noexcept {
        return ch >= '0' && ch <= '9';
    }
    static constexpr bool IsUpperA(wchar_t ch) noexcept {
        //! isupper(ch) in base ASCII set
        return ch >= 'A' && ch <= 'Z';
    }
    static constexpr bool IsLowerA(wchar_t ch) noexcept {
        //! islower(ch) in base ASCII set
        return ch >= 'a' && ch <= 'z';
    }
    static constexpr bool IsAlphaA(wchar_t ch) noexcept {
        //! _isalpha() isalpha()
        return IsUpperA(ch) || IsLowerA(ch);
    }

    static const BYTE k_AXU = 0xC0;  /// extended ASCII Upper. NOT UTF8
    static const BYTE k_AXL = 0xE0;  /// extended ASCII Lower. NOT UTF8

    // Is upper case in extended ASCII, NOT UTF8
    static constexpr bool IsUpperAXSet(wchar_t ch) noexcept {
        //! isupper(ch) = 'À' 'Ý' // extended ASCII. NOT UTF8
        return ((unsigned)ch) >= k_AXU && ((unsigned)ch) <= 0xDF;
    }
    static constexpr bool IsLowerAXSet(wchar_t ch) noexcept {
        //! islower(ch) = 'à' 'ý' // extended ASCII. NOT UTF8
        return ((unsigned)ch) >= k_AXL && ((unsigned)ch) <= 0xFF;
    }

    static constexpr bool IsAlphaUSet(wchar_t ch) noexcept {
        //! _isalpha() UNICODE set
        return ((unsigned)ch) >= 0x100 && ((unsigned)ch) <= 0x1FF;
    }
    static constexpr bool IsUpperUSet(wchar_t ch) noexcept {
        //! Unicode set. even is upper case.
        return IsAlphaUSet(ch) && (ch & 1) == 0;
    }
    static constexpr bool IsLowerUSet(wchar_t ch) noexcept {
        //! Unicode set. odd if lower case.
        return IsAlphaUSet(ch) && (ch & 1) == 1;
    }

    static constexpr bool IsUpperAX(wchar_t ch) noexcept {
        //! isupper(ch) in extended ASCII set.
        return IsUpperA(ch) || IsUpperAXSet(ch);
    }
    static constexpr bool IsLowerAX(wchar_t ch) noexcept {
        //! islower(ch) in extended ASCII set.
        return IsLowerA(ch) || IsLowerAXSet(ch);
    }

    static constexpr bool IsUpper(wchar_t ch) noexcept {
        //! isupper(ch) in UNICODE
        if (IsAscii(ch))  return IsUpperA(ch);
         
        return IsUpperAXSet(ch) || IsUpperUSet(ch);
    }
    static constexpr bool IsLower(wchar_t ch) noexcept {
        //! islower(ch) in UNICODE
        if (IsAscii(ch))  return IsLowerA(ch);
        
        return IsLowerAXSet(ch) || IsLowerUSet(ch);
    }

    static constexpr bool IsAlpha(wchar_t ch) noexcept {
        //! _isalpha() isalpha() for English/ASCII set. NOT numeric or other.
        //! _WIN32 IsCharAlpha()
        return IsLowerAX(ch) || IsUpperAX(ch) || IsAlphaUSet(ch);
    }
    /// <summary>
    /// Would this be a valid 'c' symbolic name first character?
    /// __iscsym or __iscsymf (first char)
    /// </summary>
    /// <param name="ch"></param>
    /// <returns></returns>
    static constexpr bool IsCSymF(wchar_t ch) noexcept {
        return IsAlphaA(ch) || ch == '_';
    }
    /// <summary>
    /// Would this be a valid 'c' symbolic name character? might also check k_LEN_MAX_CSYM.
    /// like: __iscsym or __iscsymf()
    /// </summary>
    static constexpr bool IsCSym(wchar_t ch) noexcept {
        return IsAlphaA(ch) || ch == '_' || IsDigitA(ch);
    }

    static constexpr char ToUpperA(char ch) noexcept {
        //! replace std::toupper() for ASCII
        if (IsLowerA(ch)) return (ch - 'a') + 'A';
        return ch;
    }
    static constexpr wchar_t ToUpperW(wchar_t ch) noexcept {
        //! std::toupper() for UNICODE
        if (IsAscii(ch)) {
            if (IsLowerA(ch)) return (ch - 'a') + 'A';
        } else {
            if (IsLowerAXSet(ch)) return (ch - k_AXL) + k_AXU;
            if (IsLowerUSet(ch)) return CastN(wchar_t, ch + 1);
        }
        return ch;
    }
    static constexpr char ToLowerA(char ch) noexcept {
        //! replace std::tolower() for ASCII // ch|0x20 sort of
        if (IsUpperA(ch)) return (ch - 'A') + 'a';
        return ch;
    }
    static constexpr wchar_t ToLowerW(wchar_t ch) noexcept {
        //! std::tolower() for UNICODE
        if (IsAscii(ch)) {
            if (IsUpperA(ch)) return (ch - 'A') + 'a';
        } else {
            if (IsUpperAXSet(ch))
                return (ch - k_AXU) + k_AXL;
            else if (IsUpperUSet(ch))
                return CastN(wchar_t, ch - 1);
        }
        return ch;
    }

    template <typename TYPE>
    static TYPE ToUpper(TYPE ch);
    template <typename TYPE>
    static TYPE ToLower(TYPE ch);

    /// <summary>
    /// Compare 2 characters ignoring case.
    /// </summary>
    /// <returns>0 = match.</returns>
    static constexpr COMPARE_t CmpI(char a, char b) noexcept {
        if (a == b) return 0;
        const wchar_t ch1 = ToLowerA(a);
        const wchar_t ch2 = ToLowerA(b);
        return ch1 - ch2;
    }

    /// <summary>
    /// Compare 2 UNICODE characters ignoring case.
    /// </summary>
    /// <returns>0 = match</returns>
    static constexpr COMPARE_t CmpI(wchar_t a, wchar_t b) noexcept {
        if (a == b) return 0;
        const wchar_t ch1 = ToLowerW(a);
        const wchar_t ch2 = ToLowerW(b);
        return ch1 - ch2;
    }

    /// <summary>
    /// Get decimal digit value.
    /// </summary>
    static constexpr int Dec2U(wchar_t ch) noexcept {
        // ASSERT(ch>='0'&&ch<=??);
        return ch - '0';
    }

    static bool GRAYCALL IsDigitF(wchar_t ch);
    static bool GRAYCALL IsDigitX(wchar_t ch, RADIX_t uRadix = 0x10);
    static bool GRAYCALL IsVowel(wchar_t ch);
    static char GRAYCALL U2Hex(UINT uVal);
    static int GRAYCALL Hex2U(wchar_t ch);
    static char GRAYCALL U2Radix(UINT uVal, RADIX_t uRadix = 10);
    static int GRAYCALL Radix2U(wchar_t ch, RADIX_t uRadix = 10);
};

template <>
constexpr char StrChar::ToUpper(char ch) noexcept {  // static
    return StrChar::ToUpperA(ch);
}
template <>
constexpr wchar_t StrChar::ToUpper(wchar_t ch) noexcept {  // static
    return StrChar::ToUpperW(ch);
}
template <>
constexpr char StrChar::ToLower(char ch) noexcept {  // static
    return StrChar::ToLowerA(ch);
}
template <>
constexpr wchar_t StrChar::ToLower(wchar_t ch) noexcept {  // static
    return StrChar::ToLowerW(ch);
}
}  // namespace Gray
#endif  // StrChar
