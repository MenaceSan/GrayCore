//! @file StrT.inl
//! included by "StrT.h" or "StrT.cpp" to implement the functions. (perhaps inline, perhaps not)
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrT_INL
#define _INC_StrT_INL
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrA.h"
#include "StrConst.h"
#include "cValT.h"

namespace Gray {
//***************************************************************************
// Compares and searches

template <typename TYPE>
COMPARE_t GRAYCALL StrT::Cmp(const TYPE* pszStr1, const TYPE* pszStr2) noexcept {
    if (pszStr1 == pszStr2) return COMPARE_Equal;
    if (pszStr1 == nullptr) return COMPARE_Less;
    if (pszStr2 == nullptr) return COMPARE_Greater;
    for (StrLen_t i = 0;; i++) {
        DEBUG_CHECK(i < cStrConst::k_LEN_MAX);
        const TYPE ch1 = pszStr1[i];
        const TYPE ch2 = pszStr2[i];
        if (ch1 == '\0' || ch1 != ch2) return ch1 - ch2;
    }
}
template <typename TYPE>
COMPARE_t GRAYCALL StrT::CmpN(const TYPE* pszStr1, const TYPE* pszStr2, StrLen_t iLenMaxChars) noexcept {
    if (pszStr1 == pszStr2) return COMPARE_Equal;
    if (pszStr1 == nullptr) return COMPARE_Less;
    if (pszStr2 == nullptr) return COMPARE_Greater;
    if (iLenMaxChars < 0) return COMPARE_Less;
    DEBUG_CHECK(iLenMaxChars <= cStrConst::k_LEN_MAX);
    for (StrLen_t i = 0; i < iLenMaxChars; i++) {
        const TYPE ch1 = pszStr1[i];
        const TYPE ch2 = pszStr2[i];
        if (ch1 == '\0' || ch1 != ch2) return ch1 - ch2;
    }
    return COMPARE_Equal;
}

template <typename TYPE>
COMPARE_t GRAYCALL StrT::CmpI(const TYPE* pszStr1, const TYPE* pszStr2) noexcept {
    //! @note for some reason the M$ version fails in static initializers in release mode !?
    //! replace _strcmpi strcmpi _stricmp
    //! "#define _stricmp	strcasecmp"
    if (pszStr1 == pszStr2) return COMPARE_Equal;
    if (pszStr1 == nullptr) return COMPARE_Less;
    if (pszStr2 == nullptr) return COMPARE_Greater;
    for (StrLen_t i = 0;; i++) {
        DEBUG_CHECK(i < cStrConst::k_LEN_MAX);
        const TYPE ch1 = pszStr1[i];
        const COMPARE_t iDiff = StrChar::CmpI(ch1, pszStr2[i]);
        if (ch1 == '\0' || iDiff != 0) return iDiff;
    }
}

template <typename TYPE>
COMPARE_t GRAYCALL StrT::CmpIN(const TYPE* pszStr1, const TYPE* pszStr2, StrLen_t iLenMaxChars) noexcept {
    if (pszStr1 == pszStr2) return COMPARE_Equal;
    if (pszStr1 == nullptr) return COMPARE_Less;
    if (pszStr2 == nullptr) return COMPARE_Greater;
    if (iLenMaxChars < 0) return COMPARE_Less;
    DEBUG_CHECK(iLenMaxChars <= cStrConst::k_LEN_MAX);
    for (StrLen_t i = 0; i < iLenMaxChars; i++) {
        const COMPARE_t iDiff = StrChar::CmpI(pszStr1[i], pszStr2[i]);
        if (pszStr1[i] == '\0' || iDiff != 0) return iDiff;
    }
    return COMPARE_Equal;
}

template <typename TYPE>
bool StrT::StartsWithI(const TYPE* pszStr1, const TYPE* pszPrefix) {
    //! Compare pszStr1 up to the length of pszPrefix
    //! Similar to .NET StartsWith() https://msdn.microsoft.com/en-us/library/system.string.startswith%28v=vs.110%29.aspx?f=255&MSPPError=-2147217396
    //! Look for a prefix. Similar to CmpHeadI

    if (pszStr1 == nullptr) pszStr1 = cStrConst::k_Empty;
    if (pszPrefix == nullptr) pszPrefix = cStrConst::k_Empty;

    for (StrLen_t i = 0;; i++) {
        ASSERT(i < cStrConst::k_LEN_MAX);
        const TYPE ch = pszPrefix[i];
        if (ch == '\0') return true;  // consider this equal.
        const COMPARE_t iDiff = StrChar::CmpI(ch, pszStr1[i]);
        if (iDiff != COMPARE_Equal) return false;
    }
}

template <typename TYPE>
bool GRAYCALL StrT::EndsWithI(const cSpan<TYPE>& str, const cSpan<TYPE>& postFix) noexcept {
    //! Compare the end of pszStr1 with pszPostfix
    //! Similar to .NET EndsWith()
    //! Look for a pszPostfix ignoring case.
    if (str.isNull()) return postFix.isNull();
    if (postFix.isNull()) return false;
    const StrLen_t nOffset = str.get_MaxLen() - postFix.get_MaxLen();
    if (nOffset < 0) return false;
    return StrT::CmpI<TYPE>(str.get_PtrConst() + nOffset, postFix) == COMPARE_Equal;
}

template <typename TYPE>
COMPARE_t GRAYCALL StrT::CmpHeadI(const TYPE* pszFindHead, const TYPE* pszTableElem) {
    if (pszTableElem == nullptr) return COMPARE_Less;
    if (pszFindHead == nullptr) return COMPARE_Less;
    for (StrLen_t i = 0;; i++) {
        ASSERT(i < cStrConst::k_LEN_MAX);
        TYPE ch1 = pszFindHead[i];
        TYPE ch2 = pszTableElem[i];
        if (!StrChar::IsCSym(ch1)) ch1 = '\0';  // force end
        if (!StrChar::IsCSym(ch2)) ch2 = '\0';  // force end
        const COMPARE_t iDiff = StrChar::CmpI(ch1, ch2);
        if (iDiff != 0 || ch1 == '\0') return iDiff;
    }
}

template <typename TYPE>
HASHCODE32_t GRAYCALL StrT::GetHashCode32(const cSpan<TYPE>& str) noexcept {
    if (str.isNull() || str.isEmpty()) return k_HASHCODE_CLEAR;
    return Hash32i(str);
}

//***********************************************************

template <typename TYPE>
StrLen_t GRAYCALL StrT::FindCharN(const TYPE* pszStr, TYPE chFind, StrLen_t iLenMax) noexcept {
    if (pszStr == nullptr) return k_StrLen_UNK;
    for (StrLen_t i = 0; iLenMax > 0; iLenMax--, i++) {
        const TYPE chN = pszStr[i];
        if (chN == chFind) return i;
        if (chN == '\0') break;  // found the end.
    }
    return k_StrLen_UNK;
}
template <typename TYPE>
TYPE* GRAYCALL StrT::FindChar(const TYPE* pszStr, TYPE chFind, StrLen_t iLenMax) noexcept {
    const StrLen_t i = FindCharN(pszStr, chFind, iLenMax);
    if (i < 0) return nullptr;
    return const_cast<TYPE*>(pszStr) + i;
}
template <typename TYPE>
TYPE* GRAYCALL StrT::FindCharRev(const cSpan<TYPE>& str, TYPE chFind) {
    if (str.isNull()) return nullptr;
    StrLen_t iLenMax = str.get_MaxLen();
    const TYPE* pszStr = str;
    while (--iLenMax > 0) {
        if (pszStr[iLenMax] == chFind) return const_cast<TYPE*>(pszStr + iLenMax);
    }
    return nullptr;
}
template <typename TYPE>
TYPE* GRAYCALL StrT::FindTokens(const cSpan<TYPE>& str, const TYPE* pszTokens) {
    if (str.isNull() || pszTokens == nullptr) return nullptr;
    const StrLen_t iLenMax = str.get_MaxLen();
    const TYPE* pszStr = str;
    for (StrLen_t i = 0;; i++, pszStr++) {
        if (i >= iLenMax) return nullptr;
        if (StrT::HasChar(pszTokens, *pszStr)) return const_cast<TYPE*>(pszStr);
    }
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::FindStrN(const TYPE* pszText, const TYPE* pszSubStr, StrLen_t iLenMaxChars) {
    if (pszText == nullptr || pszSubStr == nullptr) return k_StrLen_UNK;
    for (StrLen_t i = 0, iMatch = 0; i < iLenMaxChars; i++) {
        const TYPE ch = pszText[i];
        if (ch == '\0') break;
        if (ch == pszSubStr[iMatch]) {
            iMatch++;                                                // look for next char.
            if (pszSubStr[iMatch] == '\0') return (i - iMatch) + 1;  // found match!
        } else if (iMatch != 0) {
            i -= iMatch;  // non match. must revert back to start of non match. handle internal repeats.
            iMatch = 0;
        }
    }
    return k_StrLen_UNK;
}
template <typename TYPE>
TYPE* GRAYCALL StrT::FindStr(const TYPE* pszText, const TYPE* pszSubStr, StrLen_t iLenMaxChars) {
    const StrLen_t i = FindStrN(pszText, pszSubStr, iLenMaxChars);
    if (i < 0) return nullptr;
    return const_cast<TYPE*>(pszText) + i;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::FindStrNI(const TYPE* pszText, const TYPE* pszSubStr, StrLen_t iLenMaxChars) {
    if (pszText == nullptr || pszSubStr == nullptr) return k_StrLen_UNK;
    for (StrLen_t i = 0, iMatch = 0; i < iLenMaxChars; i++) {
        const TYPE ch = pszText[i];
        if (ch == '\0') break;
        if (StrChar::CmpI(ch, pszSubStr[iMatch]) == 0) {             // ignore case match.
            iMatch++;                                                // look for next char.
            if (pszSubStr[iMatch] == '\0') return (i - iMatch) + 1;  // found match!
        } else if (iMatch != 0) {
            i -= iMatch;  // non match. must revert back to start of non match. handle internal repeats.
            iMatch = 0;
        }
    }
    return k_StrLen_UNK;
}

template <typename TYPE>
TYPE* GRAYCALL StrT::FindStrI(const TYPE* pszText, const TYPE* pszSubStr, StrLen_t iLenMaxChars) {
    const StrLen_t i = FindStrNI(pszText, pszSubStr, iLenMaxChars);
    if (i < 0) return nullptr;
    return const_cast<TYPE*>(pszText) + i;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::FindWord(const TYPE* pszText, const TYPE* pszKeyWord, StrLen_t iLenMaxChars) {
    // like CmpHeadI ? NOT USED?
    if (pszText == nullptr || pszKeyWord == nullptr) return k_StrLen_UNK;
    for (StrLen_t i = 0, iMatch = 0; i < iLenMaxChars; i++) {
        const TYPE ch = pszText[i];
        if (ch == '\0') break;                                               // not found.
        if (iMatch == 0 && i && StrChar::IsAlpha(pszText[i - 1])) continue;  // not start of word. skip.
        const TYPE chM = pszKeyWord[iMatch];
        if (StrChar::CmpI(ch, chM) == COMPARE_Equal) {  // ignore case match.
            iMatch++;
            if (pszKeyWord[iMatch] != '\0') continue;
            if (!StrChar::IsAlNum(pszText[i + 1])) return (i - iMatch) + 1;  // found it. and partials don't count.
        }
        i -= iMatch;  // non match. must revert back to start of non match. handle internal repeats.
        iMatch = 0;
    }
    return k_StrLen_UNK;  // NOT Found
}

//*************************************************************

template <typename TYPE>
bool GRAYCALL StrT::IsWhitespace(const TYPE* pStr, StrLen_t iLenMaxChars) noexcept {
    if (pStr == nullptr) return true;
    for (; iLenMaxChars > 0 && pStr[0] != '\0'; iLenMaxChars--, pStr++) {
        if (!StrChar::IsSpaceX(pStr[0])) return false;
    }
    return true;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::GetWhitespaceEnd(const cSpan<TYPE>& src) {  // static
    if (src.isNull()) return 0;
    StrLen_t iLenChars = src.get_MaxLen();
    while (iLenChars > 0 && StrChar::IsSpaceX(src.GetAt(iLenChars - 1))) iLenChars--;
    return iLenChars;
}

template <typename TYPE>
bool GRAYCALL StrT::IsPrintable(const TYPE* pStr, StrLen_t iLenMaxChars) noexcept {
    //! Is this a normally printable string?
    if (pStr == nullptr) return false;
    for (StrLen_t i = 0; i < iLenMaxChars; i++) {
        const TYPE ch = pStr[i];
        if (ch == '\0') break;  // Is this an abnormal termination ?
        if (StrChar::IsSpaceX(ch)) continue;
        if (StrChar::IsPrintA(ch)) continue;
        return false;
    }
    return true;
}

//***********************************************************

template <typename TYPE>
bool GRAYCALL StrX<TYPE>::IsTableSorted(const cSpanUnk& t) {
    if (t.isEmpty()) return true;
    if (t.isNull()) return false;

    const ITERATE_t iHigh = t.GetSize() - 1;
    ITERATE_t i = 0;
    for (;; i++) {
        if (i >= iHigh) return true;
        const TYPE* pszName1 = *t.GetElemT<TYPE*>(i);
        if (pszName1 == nullptr) break;
        const TYPE* pszName2 = *t.GetElemT<TYPE*>(i + 1);
        if (pszName2 == nullptr) break;
        COMPARE_t iCompare = StrT::CmpI(pszName1, pszName2);
        if (iCompare >= COMPARE_Equal) {  // no dupes!
            ASSERT(iCompare < 0);
            break;
        }
    }
    return false;
}

//***********************************************************

template <typename TYPE>
ITERATE_t GRAYCALL StrT::TableFindHead(const TYPE* pszFindHead, const cSpanUnk& t) {
    //! Find a string in a table.
    //! Ignores case. unsorted table.
    //! Use rules for StrT::CmpHeadI for compare. compare only up to the CSYM values.
    //! @return
    //!  -1 = not found. k_ITERATE_BAD
    if (StrT::IsNullOrEmpty(pszFindHead)) return k_ITERATE_BAD;
    if (t.isNull()) return k_ITERATE_BAD;
    for (ITERATE_t i = 0; i < t.GetSize(); i++) {
        const TYPE* pszName = *t.GetElemT<TYPE*>(i);
        if (!StrT::CmpHeadI(pszFindHead, pszName)) return i;
    }
    return k_ITERATE_BAD;
}

template <typename TYPE>
ITERATE_t GRAYCALL StrT::TableFindHeadSorted(const TYPE* pszFindHead, const cSpanUnk& t) {
    //! Find a string in a table.
    //! Do a binary search (un-cased) on a sorted table.
    //! Use rules for StrT::CmpHeadI for compare. compare only up to the CSYM values.
    //! @return -1 = not found k_ITERATE_BAD

    if (StrT::IsNullOrEmpty(pszFindHead) || t.isEmpty() || t.isNull()) return k_ITERATE_BAD;

#ifdef _DEBUG
    ASSERT(StrX<TYPE>::IsTableSorted(t));
#endif

    ITERATE_t iHigh = t.GetSize() - 1;
    if (iHigh < 0) return k_ITERATE_BAD;

    ITERATE_t iLow = 0;
    while (iLow <= iHigh) {
        const ITERATE_t i = (iHigh + iLow) / 2;
        const TYPE* pszName = *t.GetElemT<TYPE*>(i);
        const COMPARE_t iCompare = StrT::CmpHeadI(pszFindHead, pszName);
        if (iCompare == COMPARE_Equal) return i;
        if (iCompare > 0) {
            iLow = i + 1;
        } else {
            iHigh = i - 1;
        }
    }
    return k_ITERATE_BAD;
}

template <typename TYPE>
ITERATE_t GRAYCALL StrT::TableFind(const TYPE* pszFindThis, const cSpanUnk& t) {
    if (StrT::IsNullOrEmpty(pszFindThis)) return k_ITERATE_BAD;
    if (t.isNull()) return k_ITERATE_BAD;
    for (ITERATE_t i = 0; i < t.GetSize(); i++) {
        const TYPE* pszName = *t.GetElemT<TYPE*>(i);
        if (!StrT::CmpI(pszFindThis, pszName)) return i;
    }
    return k_ITERATE_BAD;
}

template <typename TYPE>
ITERATE_t GRAYCALL StrT::TableFindSorted(const TYPE* pszFindThis, const cSpanUnk& t) {
    //! Find a string in a table.
    //! Do a binary search (un-cased) on a sorted table.
    //! @return -1 = not found k_ITERATE_BAD

    if (pszFindThis == nullptr || t.isEmpty() || t.isNull()) return k_ITERATE_BAD;
#ifdef _DEBUG
    ASSERT(StrX<TYPE>::IsTableSorted(t));
#endif

    ITERATE_t iHigh = t.GetSize() - 1;
    if (iHigh < 0) return k_ITERATE_BAD;

    ITERATE_t iLow = 0;
    while (iLow <= iHigh) {
        const ITERATE_t i = (iHigh + iLow) / 2;
        const TYPE* pszName = *t.GetElemT<TYPE*>(i);
        const COMPARE_t iCompare = StrT::CmpI(pszFindThis, pszName);
        if (iCompare == COMPARE_Equal) return i;
        if (iCompare > 0) {
            iLow = i + 1;
        } else {
            iHigh = i - 1;
        }
    }
    return k_ITERATE_BAD;
}

//*************************************************************
// String Modifiers

template <typename TYPE>
StrLen_t GRAYCALL StrT::CopyLen(TYPE* pDst, const TYPE* pSrc, StrLen_t iLenMaxChars) noexcept {
    if (pDst == nullptr || iLenMaxChars <= 0) return 0;
    StrLen_t i = 0;
    if (pSrc != nullptr) {
        iLenMaxChars--;  // save room for '\0'
        if (pDst >= pSrc && pDst <= (pSrc + iLenMaxChars)) {
            i = StrT::Len2(pSrc, iLenMaxChars);
            if (pDst != pSrc) {  // same string. // Must do backwards copy like cMem::CopyOverlap().
                int j = i;
                for (; --j >= 0;) pDst[j] = pSrc[j];
            }
        } else {
            // Just copy to '\0'
            for (; pSrc[i] != '\0' && i < iLenMaxChars; i++) pDst[i] = pSrc[i];
        }
    }
    pDst[i] = '\0';  // always terminate.
    return i;
}

//***************************************************************************************

template <typename TYPE>
StrLen_t GRAYCALL StrT::TrimWhitespaceEnd(cSpanX<TYPE> ret) {
    //! Trim any whitespace off the end of the string.
    //! @return
    //!  new length of the line. (without whitespace and comments)
    const StrLen_t iLenChars = StrT::GetWhitespaceEnd(ret);
    TYPE* pStr = ret.get_PtrWork();
    if (pStr != nullptr && pStr[iLenChars] != '\0') pStr[iLenChars] = '\0';
    return iLenChars;
}

template <typename TYPE>
TYPE* GRAYCALL StrT::TrimWhitespace(TYPE* pStr, StrLen_t iLenMax) {
    //! Trim starting AND ending whitespace
    TYPE* pStrStart = pStr;
    pStr = GetNonWhitespace(pStr, iLenMax);
    iLenMax -= cValSpan::Diff(pStr, pStrStart);
    StrT::TrimWhitespaceEnd(ToSpanStr(pStr, iLenMax));
    return pStr;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::ReplaceX(cSpanX<TYPE> dst, StrLen_t iDstIdx, StrLen_t iDstSegLen, const cSpan<TYPE>& src) {
    //! Replace a segment of a string with src, Maybe change length!
    //! @arg
    //!  iDstLenMax = don't let the pDst get bigger than this.
    //!  iDstSegLen = Replace old segment length
    //! @return
    //!  length of the resulting string.
    //! e.g.
    //!  pStr = "this are a string";
    //!  StrT::ReplaceX( pStr, cFilePath::k_MaxLen, 5, 3, "is", -1 );
    //!  pStr = "this is a string";

    TYPE* pDst2 = dst.get_PtrWork() + iDstIdx;
    const StrLen_t iLenRest = StrT::Len2(pDst2 + iDstSegLen, dst.get_MaxLen() - (iDstIdx + iDstSegLen));
    const StrLen_t iSrcLen = src.get_MaxLen();
    if (iDstIdx + iSrcLen + iLenRest >= dst.get_MaxLen()) return 0;                         // not big enough!
    cMem::CopyOverlap(pDst2 + iSrcLen, pDst2 + iDstSegLen, (iLenRest + 1) * sizeof(TYPE));  // make room.
    cMem::Copy(pDst2, src, iSrcLen * sizeof(TYPE));
    return iSrcLen;
}

//******************************************************************************

template <typename TYPE>
TYPE* GRAYCALL StrT::FindBlockEnd(STR_BLOCK_t eBlockType, const TYPE* pszLine, StrLen_t iLenMaxChars) {
    //! Find the end of an STR_BLOCK_t sequence. (quote,brace,bracket,parenthesis)
    //! skip nested blocks. deal with quoted and escaped strings.
    //! @arg
    //!  eBlockType = what type of block is this? 0 = STR_BLOCK_t::_QUOTE.
    //!    STR_BLOCK_t::_NONE = just look for nested blocks.
    //!  pszLine = the TYPE char AFTER the opening quote/brace/etc STR_BLOCK_t char.
    //!  iLenMaxChars = cStrConst::k_LEN_MAX
    //! @return
    //!  Pointer to the end of the block sequence. Perhaps caller will replace it with '\0' ?
    //!  nullptr = FAIL, same pointer = FAIL.

    if (pszLine == nullptr) return nullptr;
    const TYPE chEnd = (eBlockType <= STR_BLOCK_t::_NONE) ? '\0' : CastN(TYPE, GetBlockEnd(eBlockType));

    StrLen_t i = 0;
    for (; i < iLenMaxChars; i++) {
        TYPE ch = pszLine[i];
        if (ch == '\0') break;                                   // nothing closing.
        if (ch == chEnd) return const_cast<TYPE*>(pszLine + i);  // Found the end. OK.

        // NO Such thing as nested blocks inside " quote. only look for quote end.
        if (eBlockType == STR_BLOCK_t::_QUOTE) {  // so don't bother looking.
            // BUT skip \escape sequences inside quotes.
            if (ch == '\\') {
                i += EscSeqDecode1(ch, pszLine + i + 1);
            }
            continue;
        }

        // Unmatched block ends inside? BAD. Terminate. Skip STR_BLOCK_t::_QUOTE
        if (StrT::HasChar(k_szBlockEnd + 1, (char)ch)) {
            if (eBlockType != STR_BLOCK_t::_NONE) {
#ifdef _INC_cLogMgr_H
                DEBUG_ERR(("Unmatched internal %c mark!", ch));
#endif
            }
            break;
        }

        // Allow nested blocks of some STR_BLOCK_t.
        STR_BLOCK_t eBlockType2 = (STR_BLOCK_t)StrT::FindCharN<char>(k_szBlockStart, (char)ch);  // cast away wchar_t
        if (eBlockType2 > STR_BLOCK_t::_NONE) {
            const TYPE* pszBlockEnd = StrT::FindBlockEnd(eBlockType2, pszLine + i + 1);
            if (pszBlockEnd != nullptr && CastN(TYPE, GetBlockEnd(eBlockType2)) == pszBlockEnd[0]) {
                i = cValSpan::Diff(pszBlockEnd, pszLine);
            } else {
                // Failed!
            }
        }
    }

    if (eBlockType == STR_BLOCK_t::_NONE) {
        return const_cast<TYPE*>(pszLine + i);  // i was just looking for nesting errors.
    }

    // Failed to find closing character.
#ifdef _INC_cLogMgr_H
    DEBUG_ERR(("Unmatched ending %c mark!", GetBlockStart(eBlockType)));
#endif
    return const_cast<TYPE*>(pszLine);
}

template <typename TYPE>
TYPE* GRAYCALL StrT::StripBlock(TYPE* pszText) {
    //! strip block based on the very first character of the string.
    //! If the string is encased in "" or () then remove them.

    const STR_BLOCK_t eBlockType = (STR_BLOCK_t)StrT::FindCharN<char>(k_szBlockStart, (char)pszText[0]);  // start block.
    if (eBlockType <= STR_BLOCK_t::_NONE) return pszText;                                                 // not blocked, at least not a type I recognize.

    TYPE* pszBlockEnd = StrT::FindBlockEnd(eBlockType, pszText + 1);                       // const_cast
    if (pszBlockEnd == nullptr || pszBlockEnd[0] != CastN(TYPE, GetBlockEnd(eBlockType)))  // failed to close !
        return pszText;
    *pszBlockEnd = '\0';
    return pszText + 1;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::EscSeqDecode1(OUT TYPE& ch, const TYPE* pStrIn) {
    const TYPE ch2 = *pStrIn;
    const int iEsc = StrT::FindCharN<char>(k_szEscEncode, (char)ch2);
    if (iEsc >= 0 && StrChar::IsAscii(ch)) {
        ch = k_szEscDecode[iEsc];
        return 1;
    }
    if (ch2 == 'x') {
        // MUST be followed by at least 1 hex char. but can have 2
        const TYPE* pszEndN;
        ch = CastN(TYPE, StrT::toU(pStrIn + 1, &pszEndN, 16));
        return cValSpan::Diff(pszEndN, pStrIn);
    }
    if (ch2 >= '0' && ch2 <= '7') {  // If it's a digit then it's octal.
        // MUST be followed by at least 1 octal chars. but can have 3
        const TYPE* pszEndN;
        ch = CastN(TYPE, StrT::toU(pStrIn, &pszEndN, 8));
        return cValSpan::Diff(pszEndN, pStrIn);
    }
#if 0
    if (ch2 == 'u') {
	    // @todo u## = UNICODE encoded ! can produce multiple character UTF8 sequences.
		const TYPE* pszEndN;
		ch = StrT::toU(pStrIn+1, &pszEndN, 10);
    }
#endif
    // JUNK ? bad encoding.
    return 0;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::EscSeqDecode(cSpanX<TYPE> ret, const TYPE* pStrIn, StrLen_t iLenInMax) {
    if (pStrIn == nullptr || ret.isEmpty()) return 0;
    StrLen_t j = 0;
    StrLen_t i = 0;
    for (; i < iLenInMax && j < ret.GetSize() - 1; i++, j++) {
        TYPE ch = pStrIn[i];
        if (ch == '\0') break;
        if (ch == '\\') i += EscSeqDecode1(ch, pStrIn + i + 1);
        ret.get_PtrWork()[j] = ch;
    }
    ret.get_PtrWork()[j] = '\0';
    return i;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::EscSeqDecodeQ(cSpanX<TYPE> ret, const TYPE* pStrIn, StrLen_t iLenInMax) {
    //! Remove the opening and closing quotes. Put enclosed string (decoded) into ret.
    //! @return the consumed length of pStrIn. NOT the length of ret.

    if (pStrIn == nullptr || ret.isEmpty()) return 0;

    const StrLen_t iLenMax = cValT::Min(ret.GetSize(), iLenInMax);

    if (pStrIn[0] != '"') {
        // Just copy the string untranslated.
        return StrT::CopyLen<TYPE>(ret.get_PtrWork(), pStrIn, iLenMax);
    }

    TYPE* pszBlockEnd = StrT::FindBlockEnd<TYPE>(STR_BLOCK_t::_QUOTE, pStrIn + 1, iLenMax - 1);
    if (pszBlockEnd == nullptr || pszBlockEnd[0] != '\"') {  // not sure what this is. not closed string.
        return k_StrLen_UNK;                                 // BAD. NOT Closed.
    }

    const StrLen_t iLenIn = cValSpan::Diff(pszBlockEnd, pStrIn);

    // remove escape chars. ignore junk after end quote. Thats my callers problem.
    return StrT::EscSeqDecode<TYPE>(ret, pStrIn + 1, iLenIn - 1) + 2;
}

template <typename TYPE>
bool GRAYCALL StrT::EscSeqTest(const TYPE* pStrIn) {
    //! @note Only double quotes and "\\\b\f\n\r\t" are needed for JSON strings.
    ASSERT_NN(pStrIn);
    for (StrLen_t iIn = 0;; iIn++) {
        const TYPE ch = pStrIn[iIn];
        if (ch == '\0') return false;  // not escape needed chars detected.
        const int iEsc = StrChar::IsAscii(ch) ? StrT::FindCharN<char>(k_szEscDecode, (char)ch) : k_StrLen_UNK;
        if (iEsc >= 0) return true;
        if (StrChar::IsPrintA(ch)) continue;  // just testing.
        return true;                          // Use hex escape seq like 	\xAA
    }
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::EscSeqAdd(cSpanX<TYPE> ret, const TYPE* pStrIn) {
    //! Encode/Replace odd chars with escape sequences. e.g. "\n"
    //! This makes the string larger!
    //! opposite of StrT::EscSeqDecode()
    //! @return new length of the string. same or more than input.

    ASSERT_NN(pStrIn);
    TYPE* pStrOut = ret.get_PtrWork();
    if (pStrOut == nullptr) return 0;
    ASSERT(pStrIn != pStrOut);

    StrLen_t iOut = 0;
    for (StrLen_t iIn = 0; iOut < ret.GetSize(); iIn++, iOut++) {
        const TYPE ch = pStrIn[iIn];
        if (ch == '\0') break;
        int iEsc = StrChar::IsAscii(ch) ? StrT::FindCharN<char>(k_szEscDecode, (char)ch) : k_StrLen_UNK;
        if (iEsc >= 0) {
            pStrOut[iOut++] = '\\';
            pStrOut[iOut] = k_szEscEncode[iEsc];
        } else if (StrChar::IsPrintA(ch)) {
            pStrOut[iOut] = ch;  // Not encoded.
        } else {
            pStrOut[iOut++] = '\\';  // Use hex escape seq like 	\xAA
            pStrOut[iOut++] = 'x';
            iOut += StrT::UtoA(ch, ToSpan(pStrOut + iOut, ret.GetSize() - iOut), 0x10) - 1;
        }
    }
    pStrOut[iOut] = '\0';
    return iOut;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::EscSeqAddQ(cSpanX<TYPE> ret, const TYPE* pStrIn) {
    //! Encode the string and add quotes.
    if (ret.GetSize() < 3) return 0;
    TYPE* pStrOut = ret.get_PtrWork();
    pStrOut[0] = '\"';
    const StrLen_t iLen = StrT::EscSeqAdd(ToSpan(pStrOut + 1, ret.GetSize() - 3), pStrIn);
    pStrOut[iLen + 1] = '\"';
    pStrOut[iLen + 2] = '\0';
    return iLen + 2;
}

//******************************************************************************

template <typename TYPE>
ITERATE_t GRAYCALL StrT::ParseArray(cSpanX<TYPE> cmdLine, cSpanX<const TYPE*, const TYPE*> cmds, const TYPE* pszSep, STRP_MASK_t uFlags) {
#if 0  // def _DEBUG
	if (!StrT::Cmp(cmdLine, CSTRCONST("0,0,0,0\""))) {
		DEBUG_MSG(("Str::Cmp"));
	}
#endif

    ASSERT(cmds.get_Count() >= 1);  // else why bother?

    if (cmdLine.isNull()) {
        cMem::Zero(cmds.get_PtrWork(), cmds.get_SizeBytes());
        return 0;
    }

    if (pszSep == nullptr) pszSep = CSTRCONST(",");

    bool bUsedNonWhitespaceSep = false;
    ITERATE_t iQty = 0;
    TYPE* pszCmdLine = cmdLine.get_PtrWork();
    const TYPE** ppCmd = cmds.get_PtrWork();

    ppCmd[0] = pszCmdLine;  // iQty=0

    StrLen_t i = 0;
    for (; i < cmdLine.get_MaxLen(); i++) {
        const TYPE ch = pszCmdLine[i];
        if (ch == '\0') break;  // no more args i guess.

        const bool bIsWhitespace = StrChar::IsSpaceX(ch);  // include newlines here.
        if (bIsWhitespace) {
            if ((uFlags & STRP_START_WHITE) && ch != '\r' && ch != '\n') {
                // trim starting whitespace.
                if (ppCmd[iQty] == (pszCmdLine + i)) {
                    ppCmd[iQty] = pszCmdLine + i + 1;
                    continue;
                }
            }
            if (uFlags & STRP_MERGE_CRNL) {
                // merge CR and LF/NL
                if (ch == '\r' && pszCmdLine[i + 1] == '\n') {  // the normal order is \r\n (13,10)
                    continue;
                }
            }
        }
        if (StrT::HasChar(pszSep, ch)) {  // found a separator.
            if (cmds.get_Count() == 1) break;

            if (uFlags & STRP_SPACE_SEP) {
                // allow whitespace separators only if no other separator used.
                if (!bIsWhitespace) {
                    bUsedNonWhitespaceSep = true;
                    uFlags &= ~STRP_EMPTY_SKIP;  // empty is intentional.
                } else if (bUsedNonWhitespaceSep) {
                    continue;  // ignore whitespace now.
                }
            }

            TYPE* pszCmdStart = const_cast<TYPE*>(ppCmd[iQty]);
            if (uFlags & STRP_END_WHITE) {  //  trim whitespace from the end of the token
                ASSERT(pszCmdLine + i >= pszCmdStart);
                StrT::TrimWhitespaceEnd(ToSpan(pszCmdStart, cValSpan::Diff(pszCmdLine + i, pszCmdStart)));
            }
            if (pszCmdLine[i] != '\0') pszCmdLine[i] = '\0';

            if (*pszCmdStart == '\0') {  // Empty token
                if (uFlags & STRP_EMPTY_SKIP) {
                    iQty--;                             // merge/skip empty tokens
                } else if (uFlags & STRP_EMPTY_STOP) {  // Just stop on empty token.
                    ppCmd[++iQty] = pszCmdStart;        // include the empty token as the terminator.
                    break;
                }
            }

            // Start of the next token.
            ASSERT(iQty + 1 < cmds.GetSize());
            ppCmd[++iQty] = pszCmdLine + i + 1;

            if (iQty + 1 >= cmds.GetSize()) {  // this is just the last anyhow. so we are done. only get here to allow skip of whitespace.
                // The last entry we have room for. so just take the rest of the args.
                i++;
                if (uFlags & STRP_START_WHITE) {
                    i += StrT::GetNonWhitespaceN(pszCmdLine + i);
                }
                i += StrT::Len2(pszCmdLine + i, cmdLine.get_MaxLen() - i);  // the rest.
                break;
            }
            continue;
        }

        if ((uFlags & STRP_CHECK_BLOCKS) && StrChar::IsAscii(ch)) {
            // Special block char ? but not STR_BLOCK_t::_QUOTE
            // has some other block ending ? Thats weird.
            if (StrT::HasChar(k_szBlockEnd + 1, (char)ch)) break;  // Not sure what i should do about this ?

            const STR_BLOCK_t eBlockType = (STR_BLOCK_t)StrT::FindCharN<char>(k_szBlockStart, (char)ch);  // start block.
            if (eBlockType > STR_BLOCK_t::_NONE) {
                const TYPE* pszBlockEnd = StrT::FindBlockEnd(eBlockType, pszCmdLine + i + 1);          // const_cast
                if (pszBlockEnd == nullptr || pszBlockEnd[0] != CastN(TYPE, GetBlockEnd(eBlockType)))  // failed to close ?
                    break;                                                                             // FAIL
                i = cValSpan::Diff(pszBlockEnd, pszCmdLine);
            }
            continue;
        }

        // just part of the token i guess.
    }

    // clean up last entry
    TYPE* pszCmdStart = const_cast<TYPE*>(ppCmd[iQty]);
    if (uFlags & STRP_END_WHITE) {
        ASSERT((pszCmdLine + i) >= pszCmdStart);
        StrT::TrimWhitespaceEnd(ToSpan(pszCmdStart, cValSpan::Diff(pszCmdLine + i, pszCmdStart)));
    }
    if (pszCmdLine[i] != '\0') pszCmdLine[i] = '\0';
    if (pszCmdStart[0] != '\0') iQty++;  // this counts as an arg.

    // nullptr terminate list if possible.
    const ITERATE_t iQtyLeft = cValT::Min<ITERATE_t>(cmds.GetSize() - iQty, 8);
    if (iQtyLeft > 0) {
        cMem::Zero(cmds.get_PtrWork() + iQty, iQtyLeft * sizeof(TYPE*));
    }
    return iQty;
}

template <typename TYPE>
ITERATE_t GRAYCALL StrT::ParseArrayTmp(cSpanX<TYPE> tmp, const TYPE* pszCmdLine, cSpanX<const TYPE*, const TYPE*> cmds, const TYPE* pszSep, STRP_MASK_t uFlags) {
    //! Make a temporary copy of the string for parsing.
    //! @arg iTmpSizeMax = cStrConst::k_LEN_MAX
    //! @arg uFlags = deal with quoted and escaped strings.
    //! @return Quantity of arguments
    if (tmp != pszCmdLine) {
        StrT::Copy<TYPE>(tmp, pszCmdLine);
    }
    return ParseArray(tmp, cmds, pszSep, uFlags);
}

//***********************************************************

template <typename TYPE>
StrLen_t GRAYCALL StrT::MatchRegEx(const TYPE* pText, const TYPE* pPattern, bool bIgnoreCase, StrLen_t nTextMax) {  // static
    ASSERT_NN(pText);
    ASSERT_NN(pPattern);

    StrLen_t i = 0;
    for (;; i++, pPattern++) {
        const TYPE chP = *pPattern;
        const TYPE chT = pText[i];

        if (chP == '\0') {               // pattern complete. matched?
            if (chT == '\0') return i;   // Full match
            if (nTextMax > 0) return i;  // didnt match all the text. but its a full match of the pattern.
            // More text = no match.
            return 0;
        }

        if (nTextMax > 0 && i >= nTextMax) return -i;  // i didn't finish the pattern but i didn't break it either. partial match.
        if (chT == '\0') return 0;                     // end of text to compare. no match. // no match.

        if (chP == '*') {
            // * = Ignore a sequence of chars and maybe skip to the next thing?
            // NOTE: Avoid massive recursion.

            if (pPattern[1] == '\0') {
                if (nTextMax > 0) return nTextMax;
                return i + StrT::Len(pText + i);  // This is a Full match to the end.
            }

            // Looking for something after the *. Next matching pattern defines the end of *.
            for (int j = i;; j++) {
                if (pText[j] == '\0') return 0;  // no match. DON't allow bPartialMatch to end on a *.

                StrLen_t nMatch2 = StrT::MatchRegEx(pText + j, pPattern + 1, bIgnoreCase, (nTextMax > 0) ? (nTextMax - j) : nTextMax);
                if (nMatch2 != 0) {
                    if (nMatch2 < 0) {  // Run out of text but not out of pattern.
                        ASSERT(nTextMax > 0);
                        ASSERT(j - nMatch2 >= nTextMax);
                        return nMatch2 - j;
                    }
                    return j + nMatch2;
                }
            }
        }

        if (chP != '?') {  // ? = Ignore a single char
            if (bIgnoreCase) {
                if (StrChar::CmpI(chT, chP) != COMPARE_Equal) break;  // compare char.
            } else {
                if (chT != chP) break;  // if literal char doesn't match. Case sensitive.
            }
        }
    }

    return 0;  // no match.
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::ConvertToCSV(cSpanX<TYPE> ret, const cMemSpan& src) {  // static
    StrBuilder<char> bld(ret);
    for (size_t i = 0; i < src.get_SizeBytes(); i++) {
        const StrLen_t nWriteSpace = bld.get_WriteSpaceQty();
        if (nWriteSpace < 6) break;  // room to terminate < max sized number.
        if (i > 0) bld.AddChar(',');
        const StrLen_t iLenThis = UtoA(src[i], bld.GetSpanWrite(6), 10);
        if (iLenThis <= 0) break;
        bld.AdvanceWrite(iLenThis);
    }
    return bld.get_Length();
}

}  // namespace Gray
#endif  // _INC_StrT_INL
