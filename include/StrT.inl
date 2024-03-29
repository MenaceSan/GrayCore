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
bool GRAYCALL StrT::EndsWithI(const TYPE* pszStr1, const TYPE* pszPostfix, StrLen_t nLenStr) {
    //! Compare the end of pszStr1 with pszPostfix
    //! Similar to .NET EndsWith()
    //! Look for a pszPostfix ignoring case.

    if (pszStr1 == nullptr) pszStr1 = cStrConst::k_Empty;
    if (pszPostfix == nullptr) pszPostfix = cStrConst::k_Empty;
    if (nLenStr <= k_StrLen_UNK) nLenStr = Len(pszStr1);

    ASSERT(nLenStr >= 0 && nLenStr < cStrConst::k_LEN_MAX);
    const StrLen_t nLenPost = Len(pszPostfix, nLenStr + 1);  // Assume this is short.
    ASSERT(nLenPost < cStrConst::k_LEN_MAX);
    if (nLenPost > nLenStr) return false;

    return StrT::CmpI(pszStr1 + (nLenStr - nLenPost), pszPostfix) == COMPARE_Equal;
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
HASHCODE32_t GRAYCALL StrT::GetHashCode32(const TYPE* pszStr, StrLen_t nLen, HASHCODE32_t nHash) noexcept {
    if (pszStr == nullptr) return k_HASHCODE_CLEAR;
    if (nLen <= k_StrLen_UNK) nLen = StrT::Len(pszStr, cStrConst::k_LEN_MAX);  // nLen was not supplied. I must figure it out.        
    if (nLen <= 0) return k_HASHCODE_CLEAR;

    for (StrLen_t nLen2 = nLen / 2; nLen2 > 0; nLen2--) {
        nHash += StrChar::ToUpper(pszStr[0]);
        HASHCODE32_t tmp = (StrChar::ToUpper(pszStr[1]) << 11) ^ nHash;
        nHash = (nHash << 16) ^ tmp;
        pszStr += 2;
        nHash += nHash >> 11;
    }

    if (nLen & 1) {  // Handle end/odd case.
        nHash += StrChar::ToUpper(pszStr[0]);
        nHash ^= nHash << 11;
        nHash += nHash >> 17;
    }

    // Force "avalanching" of final 127 bits.
    nHash ^= nHash << 3;
    nHash += nHash >> 5;
    nHash ^= nHash << 2;
    nHash += nHash >> 15;
    nHash ^= nHash << 10;

    // This avoids ever returning a nHash code of 0, since that is used to signal "hash not computed yet"
    if (nHash == k_HASHCODE_CLEAR) return 1;  // NOT k_HASHCODE_CLEAR
    return nHash;
}

//***********************************************************

template <typename TYPE>
StrLen_t GRAYCALL StrT::FindCharN(const TYPE* pszStr, TYPE chFind) noexcept {
    //! Find index of the first occurrence of a single char in a string.
    //! @return -1 = k_StrLen_UNK = not found.
    if (pszStr != nullptr) {
        for (int i = 0;; i++) {
            TYPE chN = pszStr[i];
            if (chN == chFind) return i;
            if (chN == '\0') break;  // found the end.
        }
    }
    return k_StrLen_UNK;
}

template <typename TYPE>
TYPE* GRAYCALL StrT::FindChar(const TYPE* pszStr, TYPE chFind, StrLen_t iLenMax) noexcept {
    //! Find first occurrence of a single char in a string.
    //! replace strchr(), and memchr()
    //! @return nullptr = not found.
    if (pszStr != nullptr) {
        for (; iLenMax > 0; iLenMax--) {
            TYPE chN = *pszStr;
            if (chN == chFind) return const_cast<TYPE*>(pszStr);
            if (chN == '\0') return nullptr;  // found the end.
            pszStr++;
        }
    }
    return nullptr;
}

template <typename TYPE>
TYPE* GRAYCALL StrT::FindCharRev(const TYPE* pszStr, TYPE chFind, StrLen_t iLenMax) {
    //! Find last occurrence of a single char in a string.
    //! replace strrchr() or _tcsrchr(). find TYPE from end.
    if (pszStr == nullptr) return nullptr;
    if (iLenMax <= k_StrLen_UNK) {
        iLenMax = StrT::Len(pszStr);
    }
    while (--iLenMax > 0) {
        if (pszStr[iLenMax] == chFind) return const_cast<TYPE*>(pszStr + iLenMax);
    }
    return nullptr;
}

template <typename TYPE>
TYPE* GRAYCALL StrT::FindTokens(const TYPE* pszStr, const TYPE* pszTokens, StrLen_t iLenMaxChars) {
    //! Find one of the char pszTokens in pszStr.
    //! @return
    //!  nullptr = none found.
    if (pszStr == nullptr || pszTokens == nullptr) return nullptr;
    for (StrLen_t i = 0;; i++, pszStr++) {
        if (i >= iLenMaxChars) return nullptr;
        TYPE ch = *pszStr;
        if (ch == '\0') return nullptr;
        if (StrT::HasChar(pszTokens, ch)) return const_cast<TYPE*>(pszStr);
    }
}

template <typename TYPE>
TYPE* GRAYCALL StrT::FindStr(const TYPE* pszText, const TYPE* pszSubStr, StrLen_t iLenMaxChars) {
    //! Find pszSubStr inside pszText. (exact case match of all chars in pszSubStr)
    //! replaces strstr(), or .NET Contains()

    if (pszText == nullptr || pszSubStr == nullptr) return nullptr;

    StrLen_t i = 0;
    for (StrLen_t iMatch = 0; i < iLenMaxChars; i++) {
        TYPE ch = pszText[i];
        if (ch == '\0') break;
        if (ch == pszSubStr[iMatch]) {
            iMatch++;  // look for next char.
            if (pszSubStr[iMatch] == '\0') {
                return const_cast<TYPE*>((pszText + i - iMatch) + 1);  // found match!
            }
        } else if (iMatch != 0) {
            // must revert back to start of non match.
            i -= iMatch;
            iMatch = 0;
        }
    }
    return nullptr;
}

template <typename TYPE>
TYPE* GRAYCALL StrT::FindStrI(const TYPE* pszText, const TYPE* pszSubStr, StrLen_t iLenMaxChars) {
    if (pszText == nullptr || pszSubStr == nullptr) return nullptr;

    StrLen_t i = 0;
    for (StrLen_t iMatch = 0; i < iLenMaxChars; i++) {
        TYPE ch = pszText[i];
        if (ch == '\0') break;
        if (StrChar::CmpI(ch, pszSubStr[iMatch]) == 0) {  // ignore case.
            iMatch++;                                     // look for next char.
            if (pszSubStr[iMatch] == '\0') {
                return const_cast<TYPE*>((pszText + i - iMatch) + 1);  // found match!
            }
        } else if (iMatch != 0) {
            // must revert back to start of non match.
            i -= iMatch;
            iMatch = 0;
        }
    }
    return nullptr;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::FindWord(const TYPE* pszText, const TYPE* pszKeyWord, StrLen_t iLenMaxChars) {
    //! Find the pszKeyWord in the pszText string. Ignore Case.
    //! like FindStrI() but looks for starts of words. not match mid word.
    //! @return index of the END of the word match. <=0 = not found.

    if (pszText == nullptr) return k_StrLen_UNK;
    if (pszKeyWord == nullptr) return k_StrLen_UNK;

    StrLen_t j = 0;
    StrLen_t i = 0;
    for (; i < iLenMaxChars; i++) {
        TYPE ch = pszText[i];
        if (pszKeyWord[j] == '\0') {
            if (StrChar::IsAlNum(ch)) break;  // partials don't count.
            return i;                         // found it.
        }
        if (ch == '\0') return 0;
        if (!j && i) {
            if (StrChar::IsAlpha(pszText[i - 1]))  // not start of word ?
                continue;
        }
        if (StrChar::ToUpper(ch) == StrChar::ToUpper(pszKeyWord[j]))
            j++;
        else
            j = 0;
    }
    return 0;  // NOT Found
}

//*************************************************************

template <typename TYPE>
bool GRAYCALL StrT::IsWhitespace(const TYPE* pStr, StrLen_t iLenMaxChars) noexcept {
    //! Is the whole string whitespace, empty or nullptr?
    //! Like .NET String.IsNullOrWhiteSpace()
    //! @arg iLenMaxChars = don't bother checking more than this.

    if (pStr == nullptr) return true;
    if (iLenMaxChars > cStrConst::k_LEN_MAX) iLenMaxChars = cStrConst::k_LEN_MAX;
    for (; iLenMaxChars > 0 && pStr[0] != '\0'; iLenMaxChars--, pStr++) {
        if (!StrChar::IsSpaceX(pStr[0])) return false;
    }
    return true;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::GetWhitespaceEnd(const TYPE* pStr, StrLen_t iLenChars) {  // static
    if (pStr == nullptr) return 0;
    if (iLenChars <= k_StrLen_UNK) iLenChars = Len(pStr);
    while (iLenChars > 0 && StrChar::IsSpaceX(pStr[iLenChars - 1])) iLenChars--;
    return iLenChars;
}

template <typename TYPE>
bool GRAYCALL StrT::IsPrintable(const TYPE* pStr, StrLen_t iLenChars) noexcept {
    //! Is this a normally printable string?
    if (pStr == nullptr) return false;
    for (StrLen_t i = 0; i < iLenChars; i++) {
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
    //! DEBUG . make sure the table actually IS sorted !
    //! ignores case

    if (t.isNull()) return true;

    ITERATE_t i = 0;
    for (;; i++) {
        if (i >= t.GetSize() - 1) return true;
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

    if (StrT::IsNullOrEmpty(pszFindHead)) return k_ITERATE_BAD;
    if (t.isNull()) return k_ITERATE_BAD;
#ifdef _DEBUG
    ASSERT(StrX<TYPE>::IsTableSorted(t));
#endif

    ITERATE_t iHigh = t.GetSize() - 1;
    if (iHigh < 0) return k_ITERATE_BAD;

    ITERATE_t iLow = 0;
    while (iLow <= iHigh) {
        ITERATE_t i = (iHigh + iLow) / 2;
        const TYPE* pszName = *t.GetElemT<TYPE*>(i);
        COMPARE_t iCompare = StrT::CmpHeadI(pszFindHead, pszName);
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

    if (pszFindThis == nullptr) return k_ITERATE_BAD;
    if (t.isNull()) return k_ITERATE_BAD;
#ifdef _DEBUG
    ASSERT(StrX<TYPE>::IsTableSorted(t));
#endif

    ITERATE_t iHigh = t.GetSize() - 1;
    if (iHigh < 0) return k_ITERATE_BAD;

    ITERATE_t iLow = 0;
    while (iLow <= iHigh) {
        ITERATE_t i = (iHigh + iLow) / 2;
        const TYPE* pszName = *t.GetElemT<TYPE*>(i);
        COMPARE_t iCompare = StrT::CmpI(pszFindThis, pszName);
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
            i = StrT::Len(pSrc, iLenMaxChars);
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
StrLen_t GRAYCALL StrT::TrimWhitespaceEnd(TYPE* pStr, StrLen_t iLenChars) {
    //! Trim any whitespace off the end of the string.
    //! @return
    //!  new length of the line. (without whitespace and comments)
    if (pStr == nullptr) return 0;
    if (iLenChars == k_StrLen_UNK) {
        iLenChars = StrT::Len(pStr);
    }
    iLenChars = StrT::GetWhitespaceEnd(pStr, iLenChars);
    if (pStr[iLenChars] != '\0') {
        pStr[iLenChars] = '\0';
    }
    return iLenChars;
}

template <typename TYPE>
TYPE* GRAYCALL StrT::TrimWhitespace(TYPE* pStr, StrLen_t iLenMax) {
    //! Trim starting AND ending whitespace
    TYPE* pStrStart = pStr;
    pStr = GetNonWhitespace(pStr, iLenMax);
    iLenMax -= Diff(pStr, pStrStart);
    StrT::TrimWhitespaceEnd(pStr, StrT::Len(pStr, iLenMax));
    return pStr;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::ReplaceX(cSpanX<TYPE>& dst, StrLen_t iDstIdx, StrLen_t iDstSegLen, const TYPE* pSrc, StrLen_t iSrcLen) {
    //! Replace a segment of a string with pSrc, Maybe change length!
    //! @arg
    //!  iDstLenMax = don't let the pDst get bigger than this.
    //!  iDstSegLen = Replace old segment length
    //! @return
    //!  length of the resulting string.
    //! e.g.
    //!  pStr = "this are a string";
    //!  StrT::ReplaceX( pStr, _MAX_PATH, 5, 3, "is", -1 );
    //!  pStr = "this is a string";

    if (iSrcLen <= k_StrLen_UNK) iSrcLen = StrT::Len(pSrc);
    TYPE* pDst2 = dst.get_DataWork() + iDstIdx;
    const StrLen_t iLenRest = StrT::Len(pDst2 + iDstSegLen);
    if (iDstIdx + iSrcLen + iLenRest >= dst.get_MaxLen())  // not big enough!
        return 0;
    cMem::CopyOverlap(pDst2 + iSrcLen, pDst2 + iDstSegLen, (iLenRest + 1) * sizeof(TYPE));  // make room.
    cMem::Copy(pDst2, pSrc, iSrcLen * sizeof(TYPE));
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
                i = StrT::Diff(pszBlockEnd, pszLine);
            } else {
                // Failed!
            }
        }
    }

    if (eBlockType == STR_BLOCK_t::_NONE) {
        // i was just looking for nesting errors.
        return const_cast<TYPE*>(pszLine + i);
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

    STR_BLOCK_t eBlockType = (STR_BLOCK_t)StrT::FindCharN<char>(k_szBlockStart, (char)pszText[0]);  // start block.
    if (eBlockType <= STR_BLOCK_t::_NONE) return pszText;                                           // not blocked, at least not a type I recognize.

    TYPE* pszBlockEnd = StrT::FindBlockEnd(eBlockType, pszText + 1);                       // const_cast
    if (pszBlockEnd == nullptr || pszBlockEnd[0] != CastN(TYPE, GetBlockEnd(eBlockType)))  // failed to close !
        return pszText;
    *pszBlockEnd = '\0';
    return pszText + 1;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::EscSeqDecode1(OUT TYPE& ch, const TYPE* pStrIn) {
    TYPE ch2 = *pStrIn;
    int iEsc = StrT::FindCharN<char>(k_szEscEncode, (char)ch2);
    if (iEsc >= 0 && StrChar::IsAscii(ch)) {
        ch = k_szEscDecode[iEsc];
        return 1;
    }
    if (ch2 == 'x') {
        // MUST be followed by at least 1 hex char. but can have 2
        const TYPE* pszEndN;
        ch = CastN(TYPE, StrT::toU(pStrIn + 1, &pszEndN, 16));
        return Diff(pszEndN, pStrIn);
    }
    if (ch2 >= '0' && ch2 <= '7') {  // If it's a digit then it's octal.
        // MUST be followed by at least 1 octal chars. but can have 3
        const TYPE* pszEndN;
        ch = CastN(TYPE, StrT::toU(pStrIn, &pszEndN, 8));
        return Diff(pszEndN, pStrIn);
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
StrLen_t GRAYCALL StrT::EscSeqDecode(cSpanX<TYPE>& ret, const TYPE* pStrIn, StrLen_t iLenInMax) {
    if (pStrIn == nullptr || ret.isEmpty()) return 0;
    StrLen_t j = 0;
    StrLen_t i = 0;
    for (; i < iLenInMax && j < ret.GetSize() - 1; i++, j++) {
        TYPE ch = pStrIn[i];
        if (ch == '\0') break;
        if (ch == '\\') i += EscSeqDecode1(ch, pStrIn + i + 1);
        ret.get_DataWork()[j] = ch;
    }
    ret.get_DataWork()[j] = '\0';
    return i;
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::EscSeqDecodeQ(cSpanX<TYPE>& ret, const TYPE* pStrIn, StrLen_t iLenInMax) {
    //! Remove the opening and closing quotes. Put enclosed string (decoded) into ret.
    //! @return the consumed length of pStrIn. NOT the length of ret.

    if (pStrIn == nullptr || ret.isEmpty()) return 0;

    const StrLen_t iLenMax = cValT::Min(ret.GetSize(), iLenInMax);

    if (pStrIn[0] != '"') {
        // Just copy the string untranslated.
        return StrT::CopyLen<TYPE>(ret.get_DataWork(), pStrIn, iLenMax);
    }

    TYPE* pszBlockEnd = StrT::FindBlockEnd<TYPE>(STR_BLOCK_t::_QUOTE, pStrIn + 1, iLenMax - 1);
    if (pszBlockEnd == nullptr || pszBlockEnd[0] != '\"') {  // not sure what this is. not closed string.
        return k_StrLen_UNK;                                 // BAD. NOT Closed.
    }

    const StrLen_t iLenIn = StrT::Diff<TYPE>(pszBlockEnd, pStrIn);

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
        int iEsc = StrChar::IsAscii(ch) ? StrT::FindCharN<char>(k_szEscDecode, (char)ch) : k_StrLen_UNK;
        if (iEsc >= 0) return true;
        if (StrChar::IsPrintA(ch)) continue;  // just testing.
        return true;                          // Use hex escape seq like 	\xAA
    }
}

template <typename TYPE>
StrLen_t GRAYCALL StrT::EscSeqAdd(cSpanX<TYPE>& ret, const TYPE* pStrIn) {
    //! Encode/Replace odd chars with escape sequences. e.g. "\n"
    //! This makes the string larger!
    //! opposite of StrT::EscSeqDecode()
    //! @return new length of the string. same or more than input.

    ASSERT_NN(pStrIn);
    TYPE* pStrOut = ret.get_DataWork();
    if (pStrOut == nullptr) return 0;
    ASSERT(pStrIn != pStrOut);

    StrLen_t iOut = 0;
    for (StrLen_t iIn = 0; iOut < ret.GetSize(); iIn++, iOut++) {
        TYPE ch = pStrIn[iIn];
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
StrLen_t GRAYCALL StrT::EscSeqAddQ(cSpanX<TYPE>& ret, const TYPE* pStrIn) {
    //! Encode the string and add quotes.
    if (ret.GetSize() < 3) return 0;
    TYPE* pStrOut = ret.get_DataWork();
    pStrOut[0] = '\"';
    const StrLen_t iLen = StrT::EscSeqAdd(ToSpan(pStrOut + 1, ret.GetSize() - 3), pStrIn);
    pStrOut[iLen + 1] = '\"';
    pStrOut[iLen + 2] = '\0';
    return iLen + 2;
}

//******************************************************************************

template <typename TYPE>
ITERATE_t GRAYCALL StrT::ParseArray(cSpanX<TYPE>& cmdLine, cSpanX<TYPE*>& cmds, const TYPE* pszSep, STRP_MASK_t uFlags) {

#if 0  // def _DEBUG
	if (!StrT::Cmp(cmdLine, CSTRCONST("0,0,0,0\""))) {
		DEBUG_MSG(("Str::Cmp"));
	}
#endif

    ASSERT(cmds.get_Count() >= 1);  // else why bother?

    ITERATE_t iQty = 0;
    if (cmdLine.isNull()) {
    do_cleanup:
        // nullptr terminate list if possible.
        const ITERATE_t iQtyLeft = cValT::Min<ITERATE_t>(cmds.GetSize() - iQty, 8);
        if (iQtyLeft > 0) {
            cMem::Zero(cmds.get_DataWork() + iQty, iQtyLeft * sizeof(TYPE*));
        }
        return iQty;
    }

    if (pszSep == nullptr) pszSep = CSTRCONST(",");

    TYPE* pszCmdLine = cmdLine.get_DataWork();
    TYPE** ppCmd = cmds.get_DataWork();

    bool bUsedNonWhitespaceSep = false;
    ppCmd[0] = pszCmdLine;  // iQty=0
    StrLen_t i = 0;
    for (; i < cmdLine.get_MaxLen(); i++) {
        TYPE ch = pszCmdLine[i];
        if (ch == '\0') break;                       // no more args i guess.
        bool bIsWhitespace = StrChar::IsSpaceX(ch);  // include newlines here.
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

            TYPE* pszCmdStart = ppCmd[iQty];
            if (uFlags & STRP_END_WHITE) {
                //  trim whitespace from the end of the token
                ASSERT(pszCmdLine + i >= pszCmdStart);
                StrT::TrimWhitespaceEnd(pszCmdStart, Diff(pszCmdLine + i, pszCmdStart));
            } else if (pszCmdLine[i] != '\0') {
                pszCmdLine[i] = '\0';
            }
            if (*pszCmdStart == '\0') {
                // Empty token
                if (uFlags & STRP_EMPTY_SKIP) {
                    // merge/skip empty tokens
                    iQty--;
                } else if (uFlags & STRP_EMPTY_STOP) {
                    // Just stop on empty token.
                    ppCmd[++iQty] = pszCmdStart;  // include the empty token as the terminator.
                    break;
                }
            }

            // Start of the next token.
            ASSERT(iQty < cmds.GetSize() - 1);
            ppCmd[++iQty] = pszCmdLine + i + 1;

            if (iQty >= cmds.GetSize() - 1) {  // this is just the last anyhow. so we are done. only get here to allow skip of whitespace.
                // The last entry we have room for. so just take the rest of the args.
                i++;
                if (uFlags & STRP_START_WHITE) {
                    i += StrT::GetNonWhitespaceI(pszCmdLine + i);
                }
                i += StrT::Len(pszCmdLine + i);
                i = cValT::Min(i, cmdLine.get_MaxLen());
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
                i = StrT::Diff(pszBlockEnd, pszCmdLine);
            }
            continue;
        }

        // just part of the token i guess.
    }

    // clean up last entry
    TYPE* pszCmdStart = ppCmd[iQty];
    if (uFlags & STRP_END_WHITE) {
        ASSERT((pszCmdLine + i) >= pszCmdStart);
        StrT::TrimWhitespaceEnd(pszCmdStart, Diff(pszCmdLine + i, pszCmdStart));
    } else if (pszCmdLine[i] != '\0') {
        pszCmdLine[i] = '\0';
    }
    if (pszCmdStart[0] != '\0') {  // this counts as an arg.
        iQty++;
    }

    goto do_cleanup;
}

template <typename TYPE>
ITERATE_t GRAYCALL StrT::ParseArrayTmp(cSpanX<TYPE>& tmp, const TYPE* pszCmdLine, cSpanX<TYPE*> cmds, const TYPE* pszSep, STRP_MASK_t uFlags) {
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
        TYPE chP = *pPattern;
        TYPE chT = pText[i];

        if (chP == '\0') {    // pattern complete. matched?
            if (chT == '\0')  // Full match
                return i;
            if (nTextMax > 0)  // didnt match all the text. but its a full match of the pattern.
                return i;
            // More text = no match.
            return 0;
        }
        if (nTextMax > 0 && i >= nTextMax) {
            return -i;  // i didn't finish the pattern but i didn't break it either. partial match.
        }
        if (chT == '\0') {  // end of text to compare. no match.
            return 0;       // no match.
        }
        if (chP == '*') {
            // * = Ignore a sequence of chars and maybe skip to the next thing?
            // NOTE: Avoid massive recursion.

            if (pPattern[1] == '\0') {
                if (nTextMax > 0) {
                    return nTextMax;
                }
                return i + StrT::Len(pText + i);  // This is a Full match to the end.
            }

            // Looking for something after the *. Next matching pattern defines the end of *.
            for (int j = i;; j++) {
                if (pText[j] == '\0') {
                    return 0;  // no match. DON't allow bPartialMatch to end on a *.
                }

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
                if (StrChar::CmpI(chT, chP) != COMPARE_Equal)  // compare char.
                    break;
            } else {
                if (chT != chP)  // if literal char doesn't match. Case sensitive.
                    break;
            }
        }
    }

    return 0;  // no match.
}
}  // namespace Gray
#endif  // _INC_StrT_INL
