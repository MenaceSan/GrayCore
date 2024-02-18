//! @file StrT.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrT_H
#define _INC_StrT_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

// #include "StrArg.h"
#include "StrChar.h"
#include "StrConst.h"
#include "StrNum.h"
#include "StrU.h"
#include "cDebugAssert.h"  // ASSERT
#include "cHeap.h"
#include "cSpan.h"

namespace Gray {
/// <summary>
/// quotes/brackets and parenthesis must be matched.
/// string block types. see k_szBlockStart[] and k_szBlockEnd[]
/// ignore single quotes (may be unmatched apostrophe), commas ?
/// </summary>
enum class STR_BLOCK_t {
    _NONE = -1,

    // Symmetric
    _QUOTE = 0,  /// "X" double quotes, Text
    // DO %X% percent pairs. ala DOS batch scripts for environment variables.

    // Asymmetric
    _CURLY,   /// {X} curly braces. map. k_MapOpen
    _SQUARE,  /// [X] brackets, array. k_ArrayOpen
    _PAREN,   /// (X) parenthesis. expression.

    // DO <?X?> XML/HTML type blocks ? use StrT::ReplaceX()
    _QTY,
};

/// <summary>
/// string token/separator parsing options.
/// </summary>
typedef UINT32 STRP_MASK_t;  /// bit mask of STRP_TYPE_
enum STRP_TYPE_ : STRP_MASK_t {
    STRP_0,
    STRP_START_WHITE = 0x01,   /// remove start whitespace from each token
    STRP_SPACE_SEP = 0x02,     /// allow space separator only if non space not already used.
    STRP_END_WHITE = 0x04,     /// trim end whitespace off token.
    STRP_CHECK_BLOCKS = 0x08,  /// check for special nested block sequences. "{[(". But not STR_BLOCK_t::_QUOTE
    STRP_DEF = 0x0F,           /// default parsing for a line with , separators.

    STRP_MERGE_CRNL = 0x10,  /// merge "\r\n" (may also be separators.) (newline = linefeed)
    STRP_EMPTY_SKIP = 0x20,  /// merge/skip empty tokens
    STRP_EMPTY_STOP = 0x40,  /// Stop when we hit an empty token.
    STRP_ALL = 0x7F,         /// all parsing options on.
};

//*****************************************************************************

/// <summary>
/// A bunch of common string functions that adapt regardless of UNICODE or UTF8
/// @note This works similar to MFC StringTraits, StrTraitMFC ?
/// similar to char_traits &lt;TYPE&gt; for STL
/// </summary>
struct GRAYCORE_LINK StrT {                      // static /// namespace for string templates for UTF8 and UNICODE
    static const StrLen_t k_LEN_Default = 8096;  /// default size for allocation of buffers.

    static const char k_szBlockStart[static_cast<int>(STR_BLOCK_t::_QTY) + 1];  /// array of STR_BLOCK_t	"\"{[("
    static const char k_szBlockEnd[static_cast<int>(STR_BLOCK_t::_QTY) + 1];

    static const char k_szEscEncode[12];  /// The encoded version of escape chars. quotes, etc.
    static const char k_szEscDecode[12];

    // NON modifying methods first.
    static inline char GetBlockStart(STR_BLOCK_t b) {
        ASSERT(IS_INDEX_GOOD_ARRAY(b, k_szBlockStart));
        return k_szBlockStart[static_cast<int>(b)];
    }
    static inline char GetBlockEnd(STR_BLOCK_t b) {
        ASSERT(IS_INDEX_GOOD_ARRAY(b, k_szBlockEnd));
        return k_szBlockEnd[static_cast<int>(b)];
    }

    /// <summary>
    /// Get length of string not including '\0'. Like strlen()
    /// Danger. ASSUME sane iLenMax <= k_LEN_MAX ??  Dont use this function. use length limited version!
    /// nullptr is NOT allowed by ::strlen() or wcslen. ASSERT?
    /// </summary>
    /// <param name="pszStr"></param>
    /// <returns>Count of Chars (not bytes)</returns>
    template <typename TYPE>
    static StrLen_t Len(const TYPE* pszStr) noexcept;

    /// <summary>
    /// Get read only span. Not including '\0'.
    /// </summary>
    template <typename TYPE>
    static cSpan<TYPE> ToSpanStr(const TYPE* pszStr) noexcept {
        return ToSpan(pszStr, Len(pszStr));
    }
    template <typename TYPE>
    static cSpanX<TYPE> ToSpanWrite(TYPE* pszStr) noexcept {
        return ToSpan(pszStr, Len(pszStr) + 1);
    }

    /// <summary>
    /// a template based caster is useful for templates. rather than (const TYPE*)
    /// because it isn't really a cast. (so is safer) Its just a rule for type conversion and will fail if type is not provided.
    /// Can use with cStrConst
    /// </summary>
    /// <typeparam name="TYPE">char or wchar_t</typeparam>
    /// <param name="pszStr"></param>
    /// <returns></returns>
    template <typename TYPE>
    static inline const TYPE* Cast(const TYPE* pszStr) {
        return pszStr;
    }

    /// <summary>
    /// Like .NET String.IsNullOrEmpty. Similar to IsWhitespace().
    /// </summary>
    template <typename TYPE>
    static inline bool IsNullOrEmpty(const TYPE* pszStr) noexcept {
        if (pszStr == nullptr) return true;
        if (pszStr[0] == '\0') return true;
        return false;
    }

    /// <summary>
    /// If this is an empty string then make it nullptr.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszStr"></param>
    /// <returns></returns>
    template <typename TYPE>
    static inline const TYPE* CheckEmpty(const TYPE* pszStr) noexcept {
        if (pszStr == nullptr) return nullptr;
        if (pszStr[0] == '\0') return nullptr;
        return pszStr;
    }

    /// <summary>
    /// Get length (up to iLenMax <= k_LEN_MAX ) avoiding read errors for un-terminated sources. like strlen() but with limit. AKA strnlen().
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszStr"></param>
    /// <param name="iLenMax"></param>
    /// <returns>the length of the string up to (including) iLenMax</returns>
    template <typename TYPE>
    static constexpr StrLen_t Len(const TYPE* pszStr, StrLen_t iLenMax) noexcept {
        if (pszStr == nullptr) return 0;
        StrLen_t i = 0;
        for (; i < iLenMax && pszStr[i] != '\0'; i++) {
        }
        return i;
    }

    /// <summary>
    /// Difference between 2 pointers in chars (not bytes). Check for 64 bit overflow. Safer.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszEnd"></param>
    /// <param name="pszStart"></param>
    /// <returns></returns>
    template <typename TYPE>
    static StrLen_t inline Diff(const TYPE* pszEnd, const TYPE* pszStart) {
        ASSERT_NN(pszEnd);
        ASSERT_NN(pszStart);
        const INT_PTR i = pszEnd - pszStart;                                                                            // like ptrdiff_t cMem::Diff() but in chars not bytes.
        ASSERT(i > -(INT_PTR)(cHeap::k_ALLOC_MAX / sizeof(TYPE)) && i < (INT_PTR)(cHeap::k_ALLOC_MAX / sizeof(TYPE)));  // k_ALLOC_MAX as TYPE
        return CastN(StrLen_t, i);
    }

    /// <summary>
    /// How does pszStr1 compare to pszStr2. replaces _strcmp(), strcmp()
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszStr1"></param>
    /// <param name="pszStr2"></param>
    /// <returns>0 = equivalent values.</returns>
    template <typename TYPE>
    GRAYCORE_LINK static COMPARE_t GRAYCALL Cmp(const TYPE* pszStr1, const TYPE* pszStr2) noexcept;
    /// <summary>
    /// How does pszStr1 compare to pszStr2. replace strncmp()
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszStr1"></param>
    /// <param name="pszStr2"></param>
    /// <param name="iLenMaxChars"></param>
    /// <returns></returns>
    template <typename TYPE>
    GRAYCORE_LINK static COMPARE_t GRAYCALL CmpN(const TYPE* pszStr1, const TYPE* pszStr2, StrLen_t iLenMaxChars) noexcept;
    template <typename TYPE>
    GRAYCORE_LINK static COMPARE_t GRAYCALL CmpI(const TYPE* pszStr1, const TYPE* pszStr2) noexcept;

    /// <summary>
    /// Find matching string up to length iLenMaxChars. (unless '/0' is found).
    /// replaces _strnicmp strnicmp. "#define _strnicmp	strncasecmp"
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszStr1"></param>
    /// <param name="pszStr2"></param>
    /// <param name="iLenMaxChars"></param>
    /// <returns>COMPARE_t</returns>
    template <typename TYPE>
    GRAYCORE_LINK static COMPARE_t GRAYCALL CmpIN(const TYPE* pszStr1, const TYPE* pszStr2, StrLen_t iLenMaxChars) noexcept;

    /// <summary>
    /// Does pszTableElem start with the prefix pszFindHead?
    /// Compare only up to the length of pszTableElem.
    /// If pszFindHead has more chars but are separated by non alnum() then ignore.
    /// Follows the rules for symbolic names.
    /// @note we may want to allow / -in names for HTTP ?s
    /// similar to StartsWithI() ?
    /// </summary>
    /// <typeparam name="TYPE">char or wchar_t</typeparam>
    /// <param name="pszFindHead"></param>
    /// <param name="pszTableElem"></param>
    /// <returns>0 = match.</returns>
    template <typename TYPE>
    GRAYCORE_LINK static COMPARE_t GRAYCALL CmpHeadI(const TYPE* pszFindHead, const TYPE* pszTableElem);
    template <typename TYPE>
    GRAYCORE_LINK static bool GRAYCALL StartsWithI(const TYPE* pszStr1, const TYPE* pszPrefix);
    template <typename TYPE>
    GRAYCORE_LINK static bool GRAYCALL EndsWithI(const TYPE* pszStr1, const TYPE* pszPostfix, StrLen_t nLenStr = k_StrLen_UNK);

    /// <summary>
    /// Get a HASHCODE32_t for the string. Ignore case.
    /// based on http://www.azillionmonkeys.com/qed/hash.html super fast hash.
    /// Need not be truly unique. Just most likely unique. Low chance of collision in random set.
    /// Even distribution preferred. Simple CRC32 does not produce good distribution?
    /// Others: http://sites.google.com/site/murmurhash/, boost string_hash(), Knuth
    /// @note equivalent UNICODE and char strings should return the same HASHCODE32_t.
    /// TODO constexpr ?
    /// </summary>
    /// <return>HASHCODE32_t. Never return 0 except for empty string. k_HASHCODE_CLEAR</return>
    template <typename TYPE>
    GRAYCORE_LINK static HASHCODE32_t GRAYCALL GetHashCode32(const TYPE* pszStr, StrLen_t nLen = k_StrLen_UNK, HASHCODE32_t nHash = k_HASHCODE_CLEAR) noexcept;

    template <typename TYPE>
    GRAYCORE_LINK static TYPE* GRAYCALL FindChar(const TYPE* pszStr, TYPE ch, StrLen_t iLen = cStrConst::k_LEN_MAX) noexcept;
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL FindCharN(const TYPE* pszStr, TYPE ch) noexcept;
    template <typename TYPE>
    static bool HasChar(const TYPE* pszStr, TYPE ch) noexcept {
        return FindCharN(pszStr, ch) >= 0;
    }
    template <typename TYPE>
    GRAYCORE_LINK static TYPE* GRAYCALL FindCharRev(const TYPE* pszStr, TYPE ch, StrLen_t iLen = k_StrLen_UNK);
    template <typename TYPE>
    GRAYCORE_LINK static TYPE* GRAYCALL FindTokens(const TYPE* pszStr, const TYPE* pszTokens, StrLen_t iLenMax = cStrConst::k_LEN_MAX);

    template <typename TYPE>
    GRAYCORE_LINK static TYPE* GRAYCALL FindStr(const TYPE* pszStr, const TYPE* pszFind, StrLen_t iLenMax = cStrConst::k_LEN_MAX);

    /// <summary>
    /// Find pszSubStr in pszText. (ignores case). like strstr() but ignores case like stristr().
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszStr"></param>
    /// <param name="pszFind"></param>
    /// <param name="iLenMax"></param>
    /// <returns>nullptr = can't find it.</returns>
    template <typename TYPE>
    GRAYCORE_LINK static TYPE* GRAYCALL FindStrI(const TYPE* pszStr, const TYPE* pszFind, StrLen_t iLenMax = cStrConst::k_LEN_MAX);

    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL FindWord(const TYPE* pTextSearch, const TYPE* pszKeyWord, StrLen_t iLenMax = cStrConst::k_LEN_MAX);

    /// <summary>
    /// Skip tabs and spaces but NOT new lines. NOT '\0' either.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pStr"></param>
    /// <param name="iLenMax"></param>
    /// <returns>index of first non whitespace character</returns>
    template <typename TYPE>
    static constexpr StrLen_t GetNonWhitespaceI(const TYPE* pStr, StrLen_t iLenMax = cStrConst::k_LEN_MAX) noexcept {
        if (pStr == nullptr) return 0;
        StrLen_t i = 0;
        while (i < iLenMax && StrChar::IsSpace(pStr[i])) i++;
        return i;
    }
    template <typename TYPE>
    static const TYPE* GetNonWhitespace(const TYPE* pStr, StrLen_t iLenMax = cStrConst::k_LEN_MAX) noexcept {
        // never return nullptr unless pStr = nullptr
        return pStr + GetNonWhitespaceI(pStr, iLenMax);
    }
    template <typename TYPE>
    static TYPE* GetNonWhitespace(TYPE* pStr, StrLen_t iLenMax = cStrConst::k_LEN_MAX) noexcept {
        return pStr + GetNonWhitespaceI(pStr, iLenMax);
    }

    /// <summary>
    /// Walk backwards from the end of the string.
    /// </summary>
    /// <returns>get Length of the string minus ending whitespace. (strips new lines). 0 = it was all spaces.</returns>
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL GetWhitespaceEnd(const TYPE* pStr, StrLen_t iLenChars = k_StrLen_UNK);

    template <typename TYPE>
    GRAYCORE_LINK static bool GRAYCALL IsWhitespace(const TYPE* pStr, StrLen_t iLenChars = cStrConst::k_LEN_MAX) noexcept;

    template <typename TYPE>
    GRAYCORE_LINK static bool GRAYCALL IsPrintable(const TYPE* pStr, StrLen_t iLenChars = cStrConst::k_LEN_MAX) noexcept;

    // String searching. const

    /// <summary>
    /// Find a string in a table (of elements of arbitrary size) where the first element is a pointer to a string. Ignores case.
    /// </summary>
    /// <returns>-1 = no match. k_ITERATE_BAD</returns>
    template <typename TYPE>
    GRAYCORE_LINK static ITERATE_t GRAYCALL TableFind(const TYPE* pszFindThis, const cSpanUnk& t);
    template <typename TYPE>
    GRAYCORE_LINK static ITERATE_t GRAYCALL TableFindHead(const TYPE* pszFindHead, const cSpanUnk& t);
    template <typename TYPE>
    GRAYCORE_LINK static ITERATE_t GRAYCALL TableFindSorted(const TYPE* pszFindThis, const cSpanUnk& t);
    template <typename TYPE>
    GRAYCORE_LINK static ITERATE_t GRAYCALL TableFindHeadSorted(const TYPE* pszFindHead, const cSpanUnk& t);

    template <typename TYPE_CH, class TYPE_ELEM>
    static ITERATE_t SpanFind(const TYPE_CH* k, const cSpan<TYPE_ELEM>& t) {
        return TableFind(k, t.get_SpanUnk());
    }
    template <typename TYPE_CH, class TYPE_ELEM>
    static ITERATE_t SpanFindHead(const TYPE_CH* k, const cSpan<TYPE_ELEM>& t) {
        return TableFindHead(k, t.get_SpanUnk());
    }
    template <typename TYPE_CH, class TYPE_ELEM>
    static ITERATE_t SpanFindSorted(const TYPE_CH* k, const cSpan<TYPE_ELEM>& t) {
        return TableFindSorted(k, t.get_SpanUnk());
    }
    template <typename TYPE_CH, class TYPE_ELEM>
    static ITERATE_t SpanFindHeadSorted(const TYPE_CH* k, const cSpan<TYPE_ELEM>& t) {
        return TableFindHeadSorted(k, t.get_SpanUnk());
    }

    /// <summary>
    /// simple string regular expression (regex) pattern matching. i.e. *? Wildcards.
    /// regex like. Case sensitive. Recursive very simple regex pattern match with backtracking.
    /// Will cope with match( "a.b.c", "*.c" )		///
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pText"></param>
    /// <param name="pRegExPattern"></param>
    /// <param name="bIgnoreCase"></param>
    /// <param name="nTextMax">nTextMax > 0 = allow partial matches. effectively adds a * to the end.</param>
    /// <returns>length of the match in pText. if -lt- Len(pText) then its only a partial match. pText has extra stuff.</returns>
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL MatchRegEx(const TYPE* pText, const TYPE* pRegExPattern, bool bIgnoreCase, StrLen_t nTextMax = k_StrLen_UNK);

    //**********************************************************************
    // String modifying.

    /// <summary>
    /// Copy a string. replaces strncpy (sort of)
    /// @note This will ALWAYS terminate the string (unlike strncpy)
    /// @note DO NOT assume pSrc is null terminated. tho it might be. just use iLenMaxChars
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pDst"></param>
    /// <param name="pSrc"></param>
    /// <param name="iLenMaxChars">_countof(Dst) = includes room for '\0'. (just like memcpy). iLenMaxChars=_countof(Dst) is OK !</param>
    /// <returns>Length of pDst in chars. (Not including '\0')</returns>
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL CopyLen(TYPE* pszDst, const TYPE* pSrc, StrLen_t iLenCharsMax) noexcept;  // iLenCharsMax includes room for '\0'

    template <typename TYPE>
    inline static StrLen_t GRAYCALL Copy(cSpanX<TYPE>& dst, const TYPE* pSrc) noexcept {  // cSpanX includes room for '\0'
        return CopyLen(dst.get_DataWork(), pSrc, dst.GetSize());
    }

    /// <summary>
    /// replaces _strupr(). No portable __linux__ equiv to _strupr()?
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszDst"></param>
    /// <param name="iLenCharsMax"></param>
    /// <returns></returns>
    template <typename TYPE>
    static void MakeUpperCase(TYPE* pszDst, StrLen_t iLenCharsMax) noexcept {
        if (pszDst == nullptr) return;
        for (StrLen_t i = 0; i < iLenCharsMax; i++) {
            const TYPE ch = pszDst[i];
            if (ch == '\0') return;
            pszDst[i] = StrChar::ToUpper(ch);
        }
    }

    template <typename TYPE>
    static void MakeLowerCase(TYPE* pszDst, StrLen_t iLenCharsMax) noexcept {
        //! replaces strlwr()
        //! No portable __linux__ equiv to strlwr()?
        if (pszDst == nullptr) return;
        for (StrLen_t i = 0; i < iLenCharsMax; i++) {
            const TYPE ch = pszDst[i];
            if (ch == '\0') return;
            pszDst[i] = StrChar::ToLower(ch);
        }
    }

    // like _vsntprintf
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL vsprintfN(cSpanX<TYPE>& ret, const TYPE* pszFormat, va_list vlist);

    /// <summary>
    /// Format a string with variadic arguments. Truncate at iLenOutMax if necessary.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszOut"></param>
    /// <param name="iLenOutMax">max output size in characters. (Not Bytes) Must allow space for '\0'</param>
    /// <param name="pszFormat"></param>
    /// <param name=""></param>
    /// <returns>size in characters. -1 = too small. (NOT CONSISTENT WITH LINUX!)</returns>
    template <typename TYPE>
    static StrLen_t _cdecl sprintfN(OUT cSpanX<TYPE>& ret, const TYPE* pszFormat, ...) {
        va_list vargs;
        va_start(vargs, pszFormat);
        const StrLen_t nLenRet = StrT::vsprintfN(ret, pszFormat, vargs);
        va_end(vargs);
        return nLenRet;
    }

    template <typename TYPE>
    GRAYCORE_LINK static TYPE* GRAYCALL FindBlockEnd(STR_BLOCK_t eBlockType, const TYPE* pLine, StrLen_t iLenMax = cStrConst::k_LEN_MAX);
    template <typename TYPE>
    GRAYCORE_LINK static TYPE* GRAYCALL StripBlock(TYPE* pszText);

    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqDecode1(OUT TYPE& ch, const TYPE* pStrIn);

    /// <summary>
    /// Filter/decode out the 'C like' embedded escape sequences. "\n\r" etc
    /// ret string will shrink. Assumed to be inside quotes ending at iLenInMax.
    /// opposite of StrT::EscSeqAdd().
    /// @note Since we are removing, allow ret = pStrIn.
    /// @note should we check for internal chars < 32 NOT encoded ? THIS would be an error !
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="ret"></param>
    /// <param name="pStrIn"></param>
    /// <param name="iLenInMax"></param>
    /// <returns>the consumed length of pStrIn. NOT the length of ret.</returns>
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqDecode(cSpanX<TYPE>& ret, const TYPE* pStrIn, StrLen_t iLenInMax = cStrConst::k_LEN_MAX);
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqDecodeQ(cSpanX<TYPE>& ret, const TYPE* pStrIn, StrLen_t iLenInMax = cStrConst::k_LEN_MAX);

    template <typename TYPE>
    GRAYCORE_LINK static bool GRAYCALL EscSeqTest(const TYPE* pStrIn);
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqAdd(cSpanX<TYPE>& ret, const TYPE* pStrIn);
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqAddQ(cSpanX<TYPE>& ret, const TYPE* pStrIn);

    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL TrimWhitespaceEnd(TYPE* pStr, StrLen_t iLenChars = k_StrLen_UNK);
    template <typename TYPE>
    GRAYCORE_LINK static TYPE* GRAYCALL TrimWhitespace(TYPE* pStr, StrLen_t iLenMax = cStrConst::k_LEN_MAX);

    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL ReplaceX(cSpanX<TYPE>& dst, StrLen_t nDstIdx, StrLen_t iDstSegLen, const TYPE* pSrc, StrLen_t iSrcLen = k_StrLen_UNK);

    /// <summary>
    /// Parse a separated list of tokens/arguments to a function/command. inline.
    /// EXAMPLES:<code>
    ///  "tag=word&tag2=word" for HTML pages.
    ///  "arg \t arg" preserve 1 space.</code>
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="cmdLine">NOTE: this is modified / chopped up buffer.</param>
    /// <param name="cmds">an array of pointers inside pszCmdLine</param>
    /// <param name="pszSep">what are the valid separators.</param>
    /// <param name="uFlags">STRP_MASK_t = STRP_TYPE_. deal with quoted and escaped strings.</param>
    /// <returns>Number of args. in ppCmd. 0 = empty string/array.</returns>
    template <typename TYPE>
    GRAYCORE_LINK static ITERATE_t GRAYCALL ParseArray(cSpanX<TYPE>& cmdLine, cSpanX<TYPE*>& cmds, const TYPE* pszSep = nullptr, STRP_MASK_t uFlags = STRP_DEF);
    template <typename TYPE>
    GRAYCORE_LINK static ITERATE_t GRAYCALL ParseArrayTmp(cSpanX<TYPE>& tmp, const TYPE* pszCmdLine, cSpanX<TYPE*> cmds, const TYPE* pszSep = nullptr, STRP_MASK_t uFlags = STRP_DEF);

    //**********************************************************************
    // Numerics - All numerics can be done in Latin. So just convert to use StrNum

    // string to numeric. similar to strtoul()
    template <typename TYPE>
    GRAYCORE_LINK static UINT64 GRAYCALL toUL(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/ nullptr, RADIX_t nBaseRadix = 0) noexcept;
    template <typename TYPE>
    GRAYCORE_LINK static INT64 GRAYCALL toIL(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/ nullptr, RADIX_t nBaseRadix = 10) noexcept;

    template <typename TYPE>
    static UINT toU(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/ nullptr, RADIX_t nBaseRadix = 0) noexcept {
        //! Just cast down from 64.
        return (UINT)toUL(pszStr, ppszStrEnd, nBaseRadix);
    }
    template <typename TYPE>
    static int toI(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/ nullptr, RADIX_t nBaseRadix = 10) noexcept {
        //! atoi()
        //! Just cast down from 64.
        return (int)toIL(pszStr, ppszStrEnd, nBaseRadix);
    }

    template <typename TYPE>
    static UINT_PTR toUP(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/ nullptr, RADIX_t nBaseRadix = 0) noexcept {
        // UINT_PTR as 64 bit or 32 bit as needed.
        return (UINT_PTR)toUL(pszStr, ppszStrEnd, nBaseRadix);
    }
    template <typename TYPE>
    static INT_PTR toIP(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/ nullptr, RADIX_t nBaseRadix = 10) noexcept {
        // INT_PTR as 64 bit or 32 bit as needed.
        return (INT_PTR)toIL(pszStr, ppszStrEnd, nBaseRadix);
    }

    // numeric to string
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL ULtoA(UINT64 nVal, cSpanX<TYPE>& ret, RADIX_t nBaseRadix = 10);
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL ILtoA(INT64 nVal, cSpanX<TYPE>& ret, RADIX_t nBaseRadix = 10);

    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL ULtoAK(UINT64 uVal, cSpanX<TYPE>& ret, UINT nKUnit = 1024, bool bSpace = true);

    /// <summary>
    /// Convert an UINT/UINT32/DWORD to a string. like sprintf("%u"). Use k_LEN_MAX_DIGITS_INT
    /// Assume auto convert BYTE, WORD to UINT/UINT32/DWORD.
    /// Just cast up to 64 bit.
    /// </summary>
    /// <returns>length</returns>
    template <typename TYPE>
    static StrLen_t UtoA(UINT32 nVal, cSpanX<TYPE>& ret, RADIX_t nBaseRadix = 10) {
        return ULtoA(nVal, ret, nBaseRadix);  // convert 32 bit up to 64 bit.
    }

    /// <summary>
    /// Convert an int/INT32 to a string. Use k_LEN_MAX_DIGITS_INT
    /// Assume auto convert char, short to INT32.
    /// like ltoa(); or ToString(), or like sprintf("%d"). Just cast up to 64.
    /// </summary>
    template <typename TYPE>
    static StrLen_t ItoA(INT32 nVal, cSpanX<TYPE>& ret, RADIX_t nBaseRadix = 10) {
        return ILtoA(nVal, ret, nBaseRadix);  // convert 32 bit up to 64 bit.
    }

    // String to/from Floats/Doubles
    template <typename TYPE>
    GRAYCORE_LINK static double GRAYCALL toDouble(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/ nullptr);
    template <typename TYPE>
    GRAYCORE_LINK static StrLen_t GRAYCALL DtoA(double nVal, cSpanX<TYPE>& ret, int iDecPlaces = -1, char chE = -'e');

    // read/write a string of comma separated numbers.
    template <typename TYPE>
    static StrLen_t GRAYCALL ConvertToCSV(cSpanX<TYPE>& ret, const cMemSpan& src);
};

/// <summary>
/// Type cannot be derived from arguments. we must declare char type explicit.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <typename TYPE = char>
struct GRAYCORE_LINK StrX : public StrT {  // static
    static const TYPE* GRAYCALL GetBoolStr(bool bVal) noexcept;
    static bool GRAYCALL IsTableSorted(const cSpanUnk& t);
};

// Override implementations

template <>
StrLen_t inline StrT::Len<char>(const char* pszStr) noexcept {  // count of chars NOT same as bytes (size_t)
#if USE_CRT
    if (pszStr == nullptr) return 0;
    return CastN(StrLen_t, ::strlen(pszStr));
#elif defined(__GNUC__)
    return (StrLen_t)::__builtin_strlen(p1, p2, nSizeBlock);
#else
    return Len(pszStr, k_LEN_MAX);
#endif
}
template <>
StrLen_t inline StrT::Len<wchar_t>(const wchar_t* pszStr) noexcept {
#if USE_CRT
    if (pszStr == nullptr) return 0;
    return CastN(StrLen_t, ::wcslen(pszStr));
#else
    return Len(pszStr, k_LEN_MAX);
#endif
}

template <>
inline UINT64 StrT::toUL<char>(const char* pszStr, const char** ppszStrEnd, RADIX_t nBaseRadix) noexcept {
    // Direct to Latin.
    return StrNum::toUL(pszStr, ppszStrEnd, nBaseRadix);
}
template <>
inline UINT64 StrT::toUL<wchar_t>(const wchar_t* pszStr, const wchar_t** ppszStrEnd, RADIX_t nBaseRadix) noexcept {
    // Convert to Latin.
    char szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 4];
    StrNum::GetNumberString(szTmp, pszStr, StrNum::k_LEN_MAX_DIGITS_INT);
    const char* ppszStrEndA = nullptr;
    const UINT64 nVal = StrNum::toUL(szTmp, &ppszStrEndA, nBaseRadix);
    if (ppszStrEnd != nullptr) {
        *ppszStrEnd = pszStr + StrT::Diff(ppszStrEndA, szTmp);
    }
    return nVal;
}

template <>
inline INT64 StrT::toIL<char>(const char* pszStr, const char** ppszStrEnd, RADIX_t nBaseRadix) {
    return StrNum::toIL(pszStr, ppszStrEnd, nBaseRadix);
}
template <>
inline INT64 StrT::toIL<wchar_t>(const wchar_t* pszStr, const wchar_t** ppszStrEnd, RADIX_t nBaseRadix) {
    char szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 4];
    StrNum::GetNumberString(szTmp, pszStr, StrNum::k_LEN_MAX_DIGITS_INT);
    const char* ppszStrEndA;
    const INT64 nVal = StrNum::toIL(szTmp, &ppszStrEndA, nBaseRadix);
    if (ppszStrEnd != nullptr) {
        *ppszStrEnd = pszStr + StrT::Diff(ppszStrEndA, szTmp);
    }
    return nVal;
}

template <>
inline double StrT::toDouble<char>(const char* pszStr, const char** ppszStrEnd) {  // static
#if USE_CRT
    if (pszStr == nullptr) return 0;
    return ::strtod(pszStr, (char**)ppszStrEnd);  // const_cast
#else
    return StrNum::toDouble(pszStr, ppszStrEnd);
#endif
}
template <>
inline double StrT::toDouble<wchar_t>(const wchar_t* pszStr, const wchar_t** ppszStrEnd) {  // static
#if USE_CRT
    if (pszStr == nullptr) return 0;
    return ::wcstod(pszStr, (wchar_t**)ppszStrEnd);  // const_cast
#else
    // Convert to char string first.
    char szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
    StrNum::GetNumberString(szTmp, pszStr, StrNum::k_LEN_MAX_DIGITS);
    const char* ppszStrEndA;
    const double nVal = StrNum::toDouble(szTmp, &ppszStrEndA);
    if (ppszStrEnd != nullptr) {
        *ppszStrEnd = pszStr + StrT::Diff(ppszStrEndA, szTmp);
    }
    return nVal;
#endif
}

template <>
inline StrLen_t StrT::ULtoA<char>(UINT64 uVal, cSpanX<char>& ret, RADIX_t nBaseRadix) {  // static
    return StrNum::ULtoA(uVal, ret, nBaseRadix);
}
template <>
inline StrLen_t StrT::ULtoA<wchar_t>(UINT64 uVal, cSpanX<wchar_t>& ret, RADIX_t nBaseRadix) {  // static
    char szTmp[StrNum::k_LEN_MAX_DIGITS_INT];
    const StrLen_t iStrLen = StrNum::ULtoA(uVal, TOSPAN(szTmp), nBaseRadix);
    return StrU::UTF8toUNICODE(ret, ToSpan(szTmp, iStrLen));
}
template <>
inline StrLen_t StrT::ILtoA<char>(INT64 uVal, cSpanX<char>& ret, RADIX_t nBaseRadix) {  // static
    return StrNum::ILtoA(uVal, ret, nBaseRadix);
}
template <>
inline StrLen_t StrT::ILtoA<wchar_t>(INT64 uVal, cSpanX<wchar_t>& ret, RADIX_t nBaseRadix) {  // static
    char szTmp[StrNum::k_LEN_MAX_DIGITS_INT];
    const StrLen_t iStrLen = StrNum::ILtoA(uVal, TOSPAN(szTmp), nBaseRadix); 
    return StrU::UTF8toUNICODE(ret, ToSpan(szTmp, iStrLen));
}

template <>
inline StrLen_t StrT::DtoA<char>(double nVal, cSpanX<char>& ret, int iDecPlaces, char chE) {  // static
    return StrNum::DtoAG(nVal, ret, iDecPlaces, chE);
}

template <>
inline StrLen_t StrT::DtoA<wchar_t>(double nVal, cSpanX<wchar_t>& ret, int iDecPlaces, char chE) {  // static
    char szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
    const StrLen_t iStrLen = StrNum::DtoAG2(nVal, szTmp, iDecPlaces, chE);
    return StrU::UTF8toUNICODE(ret, ToSpan(szTmp, iStrLen));
}

template <>
inline StrLen_t StrT::ULtoAK<char>(UINT64 uVal, cSpanX<char>& ret, UINT nKUnit, bool bSpace) {  // static
    return StrNum::ULtoAK(uVal, ret, nKUnit, bSpace);
}

template <>
inline StrLen_t StrT::ULtoAK<wchar_t>(UINT64 uVal, cSpanX<wchar_t>& ret, UINT nKUnit, bool bSpace) {  // static
    char szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
    const StrLen_t iStrLen = StrNum::ULtoAK(uVal, TOSPAN(szTmp), nKUnit, bSpace);
    return StrU::UTF8toUNICODE(ret, ToSpan(szTmp, iStrLen));
}

// Override implementations

template <>
inline const char* StrX<char>::GetBoolStr(bool bVal) noexcept {  // static
    // Simpler than using "true" : "false"
    return bVal ? "1" : "0";
}
template <>
inline const wchar_t* StrX<wchar_t>::GetBoolStr(bool bVal) noexcept {  // static
    return bVal ? L"1" : L"0";
}

// #include "StrT.inl"
// Force instantiation of stuff for char and wchar_t for linking.
}  // namespace Gray
#endif  // _INC_StrT_H
