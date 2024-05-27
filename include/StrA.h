//! @file StrA.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrA_H
#define _INC_StrA_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "StrConst.h"
#include "cSpan.h"

namespace Gray {
/// <summary>
/// Functions on 8 bit char ANSI C strings. Opposite of StrU.
/// </summary>
struct GRAYCORE_LINK StrA { // : public StrT // static
    static bool GRAYCALL IsBoolTrue(const char* pszStr, bool bHead = false);
    static bool GRAYCALL IsBoolFalse(const char* pszStr, bool bHead = false);

    static const char* GRAYCALL GetArticleAndSpace(const char* pszWords);

    /// <summary>
    /// is the word already plural?
    /// but will typically appear as a single object. use with StrA::GetArticleAndSpace
    /// TODO THIS NEEDS WORK
    /// Similar to M$ System.Data.Entity.Design.PluralizationServices
    /// </summary>
    static bool GRAYCALL IsPlural(const char* pszWord);

    /// <summary>
    /// Is this a valid formatted SymName?
    /// Is this a simple 'c' style identifier/symbolic string? starts with a char and can have numbers.
    /// @note JSON allows '.' as part of normal names ??
    /// </summary>
    /// <param name="pszTag">the identifier (valid char set).</param>
    /// <param name="bAllowDots"></param>
    /// <returns>length else -lt- 0 = HRESULT error.</returns>
    static HRESULT GRAYCALL CheckSymName(const ATOMCHAR_t* pszTag, bool bAllowDots = false);

    /// <summary>
    /// Get a fixed point number from a string. (n nPlaces). Used for money CY ?
    /// @note Why not use atof/strtod() ? rounding NOT ALL INTS ARE WELL STORED as FLOAT !
    /// </summary>
    /// <param name="rpszExp">get the number text from here.</param>
    /// <param name="nPlaces">how many decimal places to store</param>
    /// <returns>the new value from the text.</returns>
    static INT_PTR GRAYCALL GetFixedIntRef(const char*& rpszExp, int nPlaces);

    //*****************************************************************************
    // String Modifying
    static StrLen_t GRAYCALL MakeNamedBitmask(cSpanX<char> ret, UINT dwFlags, const char** ppszNames, COUNT_t iMaxNames, char chSep = '\0');
};
}  // namespace Gray
#endif
