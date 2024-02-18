//! @file StrFormat.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrFormat_H
#define _INC_StrFormat_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "IUnknown.h"  // DECLARE_INTERFACE
#include "StrBuilder.h"
#include "StrChar.h"
#include "StrConst.h"
#include "cDebugAssert.h"

namespace Gray {
DECLARE_INTERFACE(IIniBaseGetter);  // cIniBase.h
typedef char IniChar_t;             /// char format even on UNICODE system! Screw M$, INI files should ALWAYS have UTF8 contents

/// <summary>
/// Hold params for a single entry in a format string.
/// common for char and wchar_t StrFormat for use with printf() type functions.
/// e.g. "%[flags][width][.precision][length]specifier"
/// http://en.cppreference.com/w/cpp/io/c/fprintf
/// </summary>
struct GRAYCORE_LINK StrFormatParams {
    static const char k_Specs[16];  /// possible specs. "EFGXcdefgiosux"  // Omit "S" "apnA"

    char m_nSpec;  /// % type. 0 = invalid. from k_Specs.

    BYTE m_nWidthMin;    /// specifies minimum field width. Total width of what we place in pszOut
    short m_nPrecision;  /// how many chars from pszParam do we take? (not including '\0') -1 = default.

    // Flags.
    BYTE m_nLong;       /// 0=int, 1=long (32 bit usually), or 2=long long (64 bit usually). 'l' or 'll'. (char or wchar_t?)
    bool m_bAlignLeft;  /// - sign indicates left align.
    bool m_bPlusSign;   /// + indicates to Show sign.
    bool m_bWidthArg;   /// Get an argument that will supply the m_nWidth.

    bool m_bAddPrefix;  /// Add a prefix 0x for hex or 0 for octal.
    bool m_bLeadZero;   /// StrNum::k_LEN_MAX_DIGITS_INT

    static inline char FindSpec(char ch) noexcept {
        //! Find a valid spec char.
        for (size_t i = 0; i < _countof(k_Specs) - 1; i++) {
            const char chCur = k_Specs[i];
            if (ch > chCur)  // keep looking.
                continue;
            if (ch < chCur)  // they are sorted.
                return '\0';
            return ch;  // got it.
        }
        return '\0';  // nothing.
    }
    void ClearParams() noexcept {
        m_nSpec = '\0';
        m_nWidthMin = 0;  // default = all. No padding.
        m_nPrecision = -1;
        m_nLong = 0;  // Account for "ll"
        m_bAlignLeft = false;
        m_bPlusSign = false;
        m_bLeadZero = false;
        m_bWidthArg = false;
        m_bAddPrefix = false;
    }
};

/// <summary>
/// A formatter for a string. Like sprintf().
/// Hold state values for a single printf type format parameter/specifier and render it.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <typename TYPE = char>
struct GRAYCORE_LINK StrFormat : public StrFormatParams {
    typedef StrLen_t(_cdecl* STRFORMAT_t)(cSpanX<TYPE>& ret, const TYPE* pszFormat, ...);  // signature for testing. like StrT::sprintfN() and StrFormat::F()

    StrFormat() {
        ClearParams();
    }

    /// <summary>
    /// Parse the mode for 1 single argument/param from a sprintf() format string.
    /// e.g. %d,%s,%u,%g,%f, etc.
    /// http://www.cplusplus.com/reference/cstdio/printf/
    /// </summary>
    /// <param name="pszFormat">"%[flags][width][.precision][length]specifier"</param>
    /// <returns>0 = not a valid mode format. just flush the format.</returns>
    StrLen_t ParseParam(const TYPE* pszFormat);

    /// <summary>
    /// copy a string. Does not terminate.
    /// </summary>
    /// <param name="out"></param>
    /// <param name="pszParam">a properly terminated string.</param>
    /// <param name="nParamLen"></param>
    /// <param name="nPrecision">how many chars from pszParam do we take? (not including '\0')</param>
    void RenderString(StrBuilder<TYPE>& out, const TYPE* pszParam, StrLen_t nParamLen, short nPrecision) const;
    void RenderUInt(StrBuilder<TYPE>& out, const TYPE* pszPrefix, RADIX_t nRadix, char chRadixA, UINT64 uVal) const;
    void RenderFloat(StrBuilder<TYPE>& out, double dVal, char chE = -'e') const;

    /// <summary>
    /// Render the current single parameter/spec.
    /// </summary>
    void RenderParam(StrBuilder<TYPE>& out, va_list* pvlist) const;

    static void GRAYCALL V(StrBuilder<TYPE>& out, const TYPE* pszFormat, va_list vlist);
    static inline StrLen_t GRAYCALL V(cSpanX<TYPE>& ret, const TYPE* pszFormat, va_list vlist);

    // variadic. sprintfN
    static inline void _cdecl F(StrBuilder<TYPE>& out, const TYPE* pszFormat, ...);
    static inline StrLen_t _cdecl F(cSpanX<TYPE>& ret, const TYPE* pszFormat, ...);  // STRFORMAT_t for testing
};

/// <summary>
/// strings may contain template blocks to be replaced. e.g. "&lt;?something?&gt;".
/// similar to expressions.
/// </summary>
struct GRAYCORE_LINK StrTemplate {
    /// <summary>
    /// Does the string include "&lt;?something?&gt;" ?
    /// </summary>
    static bool GRAYCALL HasTemplateBlock(const IniChar_t* pszInp);
    static StrLen_t GRAYCALL ReplaceTemplateBlock(StrBuilder<IniChar_t>& out, const IniChar_t* pszInp, const IIniBaseGetter* pBlockReq, bool bRecursing = false);
};

template <typename TYPE>
inline void _cdecl StrFormat<TYPE>::F(StrBuilder<TYPE>& out, const TYPE* pszFormat, ...) {  // static
    va_list vargs;
    va_start(vargs, pszFormat);
    V(out, pszFormat, vargs);
    va_end(vargs);
}

template <typename TYPE>
inline StrLen_t GRAYCALL StrFormat<TYPE>::V(cSpanX<TYPE>& ret, const TYPE* pszFormat, va_list vlist) {  // static
    StrBuilder<TYPE> out(ret);
    V(OUT out, pszFormat, vlist);
    return out.get_Length();
}
template <typename TYPE>
inline StrLen_t _cdecl StrFormat<TYPE>::F(cSpanX<TYPE>& ret, const TYPE* pszFormat, ...) {  // static
    va_list vargs;
    va_start(vargs, pszFormat);
    const StrLen_t nLenOut = V(ret, pszFormat, vargs);
    va_end(vargs);
    return nLenOut;
}
}  // namespace Gray
#endif
