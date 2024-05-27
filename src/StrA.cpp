//! @file StrA.cpp
//! String global search functions. Const strings
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrA.h"
#include "StrBuilder.h"
#include "StrChar.h"
#include "cBits.h"
#include "cLogMgr.h"

namespace Gray {
bool GRAYCALL StrA::IsBoolTrue(const char* pszStr, bool bHead) {  // static
    //! convert string to boolean value.
    static const char* const k_pszBoolTrue[] = {
        "1",    "On",  "T",
        "true",  // JSON = "true", C#,.NET = "True"
        "Y",    "Yes",
    };

    if (pszStr == nullptr) return false;
    ITERATE_t i;
    if (bHead) {
        i = StrT::SpanFindHeadSorted(pszStr, TOSPAN(k_pszBoolTrue));
    } else {
        i = StrT::SpanFindSorted(pszStr, TOSPAN(k_pszBoolTrue));
    }
    return i >= 0;
}

bool GRAYCALL StrA::IsBoolFalse(const char* pszStr, bool bHead) {
    //! convert string to boolean value.
    static const char* const k_pszBoolFalse[] = {
        // static
        "0", "F", "false", "N", "No", "Off",
    };
    if (pszStr == nullptr) return false;
    ITERATE_t i;
    if (bHead) {
        i = StrT::SpanFindHeadSorted(pszStr, TOSPAN(k_pszBoolFalse));
    } else {
        i = StrT::SpanFindSorted(pszStr, TOSPAN(k_pszBoolFalse));
    }
    return i >= 0;
}

bool GRAYCALL StrA::IsPlural(const char* pszWord) {
    ASSERT_NN(pszWord);
    static const char* const k_Plurals[] = {
        // These are already plural so don't need another s.
        "boots", "cards", "eyes", "feet", "fish", "gloves", "hair", "leggings", "pants", "shoes", "sleeves",
        // sheep,  barracks, teeth, children, people, women, men, mice, geese
    };

    ITERATE_t iRet = StrT::SpanFindHeadSorted(pszWord, TOSPAN(k_Plurals));
    if (iRet >= 0) return true;

    const char* pStr = StrT::FindCharRev(StrT::ToSpanStr(pszWord), ' ');
    if (pStr != nullptr) {
        iRet = StrT::SpanFindHeadSorted(pStr + 1, TOSPAN(k_Plurals));
        if (iRet >= 0) return true;
    }

    // end with an s or es ? StrChar::IsVowel
    return false;
}

const char* GRAYCALL StrA::GetArticleAndSpace(const char* pszWord) {
    //! What indefinite article should be used with this word? "a ", "an ", ""
    //! @note This is wrong many times.
    //!  i.e. some words need no article (plurals) : boots.
    ASSERT(pszWord != nullptr);
    if (StrA::IsPlural(pszWord)) return "";
    if (StrChar::IsVowel(pszWord[0])) return "an ";
    return "a ";
}

HRESULT GRAYCALL StrA::CheckSymName(const ATOMCHAR_t* pszTag, bool bAllowDots) {
    if (StrT::IsNullOrEmpty(pszTag)) return HRESULT_WIN32_C(ERROR_EMPTY);

    StrLen_t i = 0;
    if (!bAllowDots) {
        if (!StrChar::IsCSymF(pszTag[0])) return E_INVALIDARG; // first char of symbol is special.
        i++;
    }

    STATIC_ASSERT(k_LEN_MAX_CSYM <= StrT::k_LEN_Default, "k_LEN_MAX_CSYM");

    for (;; i++) {
        if (i >= k_LEN_MAX_CSYM) return HRESULT_WIN32_C(ERROR_BAD_LENGTH);
        const ATOMCHAR_t ch = pszTag[i];
        if (ch == '\0') break;
        if (bAllowDots && ch == '.') continue;
        if (StrChar::IsCSym(ch)) continue;
        return E_INVALIDARG;  // not a valid char!
    }

    return i;
}

INT_PTR GRAYCALL StrA::GetFixedIntRef(const char*& rpszExp, int nPlaces) {  // static
    if (rpszExp == nullptr) return 0;

    INT_PTR lVal = StrT::toIP(rpszExp, &rpszExp);
    int iDecimalFound = 0;

    while (!iDecimalFound || nPlaces > 0) {
        char ch = *rpszExp;

        if (iDecimalFound == 0 && ch == '.') {
            iDecimalFound = 1;
            rpszExp++;  // AnsiNext( rpszExp );      // Get the next.
            continue;
        }

        lVal *= 10;  // move up for the decimal place.

        if (!StrChar::IsDigitA(ch)) {  // end found.
            iDecimalFound = 2;
            if (!nPlaces) break;  // May have no places.
        } else if (iDecimalFound < 2) {
            lVal += StrChar::Dec2U(ch);
            rpszExp++;  // AnsiNext( rpszExp );      // Get the next.
        }

        if (iDecimalFound > 0) nPlaces--;
    }

    return lVal;
}

//***********************************************************

StrLen_t GRAYCALL StrA::MakeNamedBitmask(cSpanX<char> ret, UINT dwFlags, const char** ppszNames, COUNT_t iMaxNames, char chSep) {
    //! For each bit set in dwFlags, copy a ppszNames[bit number] string to the pszOut. separated by chSep (|)
    //! @return string length

    bool bMath = (chSep == '\0');
    if (bMath) chSep = '|';

    StrBuilder<char> sb(ret);

    COUNT_t iCount = 0;
    for (BIT_ENUM_t i = 0; dwFlags != 0 && !sb.isOverflow(); i++, dwFlags >>= 1) {  // walk the bits.
        if (!(dwFlags & 1)) continue;
        if (iCount > 0) sb.AddChar(chSep);
        if (i >= iMaxNames) {
            // No more names so just label as numbers. Ideally this would not be called.
            sb.AdvanceWrite(StrT::UtoA(cBits::Mask1<UINT>(i), sb.get_SpanWrite(), 0x10));
        } else {
            sb.AddStr(ppszNames[i]);
        }
        iCount++;
    }
    if (iCount == 0 && bMath) {        
        return StrT::Copy(ret, "0"); // No bits were set.
    }
    return sb.get_Length();
}
}  // namespace Gray
