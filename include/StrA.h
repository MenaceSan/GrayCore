//
//! @file StrA.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrA_H
#define _INC_StrA_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "StrConst.h"

namespace Gray {
/// <summary>
/// Functions on 8 bit char ANSI C strings. Opposite of StrU.
/// </summary>
struct GRAYCORE_LINK StrA { // : public StrT // static
    static bool GRAYCALL IsBoolTrue(const char* pszStr, bool bHead = false);
    static bool GRAYCALL IsBoolFalse(const char* pszStr, bool bHead = false);

    static const char* GRAYCALL GetArticleAndSpace(const char* pszWords);
    static bool GRAYCALL IsPlural(const char* pszWord);

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
    static StrLen_t GRAYCALL MakeNamedBitmask(char* pszOut, StrLen_t iOutSizeMax, UINT dwFlags, const char** ppszNames, COUNT_t iMaxNames, char chSep = '\0');
};
}  // namespace Gray
#endif
