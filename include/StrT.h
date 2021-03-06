//
//! @file StrT.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_StrT_H
#define _INC_StrT_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"
#include "StrNum.h"
#include "StrU.h"
#include "cValArray.h"
#include "cHeap.h"
#include "cDebugAssert.h"	// ASSERT

namespace Gray
{
	class cLogProcessor;

	enum STR_BLOCK_TYPE	//!< quotes/brackets and parenthesis must be matched.
	{
		//! @enum Gray::STR_BLOCK_TYPE
		//! string block types. see k_szBlockStart[] and k_szBlockEnd[]
		//! ignore single quotes (may be unmatched apostrophe), commas ?

		STR_BLOCK_NONE = -1,

		// Symmetric
		STR_BLOCK_QUOTE = 0,	//!< "X" double quotes
		// DO %X% percent pairs. ala DOS batch scripts for environment variables.

		// Asymmetric
		STR_BLOCK_CURLY,		//!< {X} curly braces
		STR_BLOCK_SQUARE,		//!< [X] brackets
		STR_BLOCK_PAREN,		//!< (X) parenthesis

		// DO <?X?> XML/HTML type blocks ? use StrT::ReplaceX()
		STR_BLOCK_QTY,
	};
	typedef UINT32 STR_BLOCKS_t;	//!< bit mask of STR_BLOCK_TYPE

	enum STRP_TYPE_
	{
		//! @enum Gray::STRP_TYPE_
		//! string token/separator parsing options.
		STRP_0,
		STRP_START_WHITE = 0x01,		//!< remove start whitespace from each token
		STRP_SPACE_SEP = 0x02,		//!< allow space separator only if non space not already used.
		STRP_END_WHITE = 0x04,		//!< trim end whitespace off token.
		STRP_CHECK_BLOCKS = 0x08,		//!< check for special nested block sequences. "{[("
		STRP_DEF = 0x0F,			//!< default parsing for a line with , separators.

		STRP_MERGE_CRNL = 0x10,		//!< merge "\r\n" (may also be separators.) (newline = linefeed)
		STRP_EMPTY_SKIP = 0x20,		//!< merge/skip empty tokens
		STRP_EMPTY_STOP = 0x40,		//!< Stop when we hit an empty token.
		STRP_ALL = 0x7F,		//!< all parsing options on.
	};
	typedef UINT32 STRP_MASK_t;	//!< bit mask of STRP_TYPE_

	//*****************************************************************************

	struct GRAYCORE_LINK StrT // static //!< namespace for string templates for UTF8 and UNICODE
	{
		//! @struct Gray::StrT
		//! A bunch of common string functions that adapt regardless of UNICODE or UTF8
		//! @note This works similar to MFC StringTraits, StrTraitMFC<>?
		//! similar to char_traits<TYPE> for STL

		static const StrLen_t k_LEN_MAX = 15000;		//!< arbitrary max size for Format() etc. NOTE: _MSC_VER says stack frame should be at least 16384
		static const StrLen_t k_LEN_MAX_KEY = 128;		//!< arbitrary max size of (Symbolic Identifier) keys.

		static const char k_szBlockStart[STR_BLOCK_QTY + 1]; //!< array of STR_BLOCK_TYPE	"\"{[("
		static const char k_szBlockEnd[STR_BLOCK_QTY + 1];

		static const char k_szEscEncode[12];	//!< The encoded version of escape chars. quotes, etc.
		static const char k_szEscDecode[12];

		// NON modifying methods first.

		template< typename TYPE >
		static StrLen_t Len(const TYPE* pszStr) NOEXCEPT;

		template< typename TYPE >
		static inline const TYPE* Cast(const TYPE* pszStr)
		{
			//! a template based caster is useful for templates. rather than (const TYPE*)
			//! because it isn't really a cast. (so is safer) Its just a rule for type conversion and will fail if type is not provided.
			//! Can use with cStrConst
			return pszStr;
		}

		template< typename TYPE >
		static inline bool IsNullOrEmpty(const TYPE* pszStr) NOEXCEPT
		{
			//! Like .NET String.IsNullOrEmpty. Similar to IsWhitespace().
			if (pszStr == nullptr)
				return true;
			if (pszStr[0] == '\0')
				return true;
			return false;
		}

		template< typename TYPE >
		static inline const TYPE* CheckEmpty(const TYPE* pszStr) NOEXCEPT
		{
			//! If this is an empty string then make it nullptr.
			if (pszStr == nullptr)
				return nullptr;
			if (pszStr[0] == '\0')
				return nullptr;
			return pszStr;
		}

		template< typename TYPE >
		static constexpr StrLen_t Len(const TYPE* pszStr, StrLen_t iLenMax) NOEXCEPT
		{
			//! Get length (up to iLenMax <= k_LEN_MAX ) avoiding read errors for un-terminated sources. like strlen() but with limit. AKA strnlen().
			//! @return the length of the string up to (including) iLenMax
			if (pszStr == nullptr)
				return 0;
			StrLen_t i = 0;
			for (; i < iLenMax && pszStr[i] != '\0'; i++)
			{
			}
			return i;
		}

		template< typename TYPE >
		static StrLen_t inline Diff(const TYPE* pszEnd, const TYPE* pszStart)
		{
			//! Difference between 2 pointers in chars (not bytes). Check for 64 bit overflow. Safer.
			ASSERT(pszEnd != nullptr);
			ASSERT(pszStart != nullptr);
			INT_PTR i = pszEnd - pszStart;	// like ptrdiff_t cMem::Diff() but in chars not bytes. 
			ASSERT(i > -(INT_PTR)(cHeap::k_ALLOC_MAX / sizeof(TYPE)) && i < (INT_PTR)(cHeap::k_ALLOC_MAX / sizeof(TYPE)));	// k_ALLOC_MAX as TYPE
			return (StrLen_t)i;
		}

		template< typename TYPE >
		GRAYCORE_LINK static COMPARE_t GRAYCALL Cmp(const TYPE* pszStr1, const TYPE* pszStr2) NOEXCEPT;
		template< typename TYPE >
		GRAYCORE_LINK static COMPARE_t GRAYCALL CmpN(const TYPE* pszStr1, const TYPE* pszStr2, StrLen_t iLenMaxChars) NOEXCEPT;
		template< typename TYPE >
		GRAYCORE_LINK static COMPARE_t GRAYCALL CmpI(const TYPE* pszStr1, const TYPE* pszStr2) NOEXCEPT;
		template< typename TYPE >
		GRAYCORE_LINK static COMPARE_t GRAYCALL CmpIN(const TYPE* pszStr1, const TYPE* pszStr2, StrLen_t iLenMaxChars) NOEXCEPT;

		template< typename TYPE >
		GRAYCORE_LINK static COMPARE_t GRAYCALL CmpHeadI(const TYPE* pszFind, const TYPE* pszTableElem);
		template< typename TYPE >
		GRAYCORE_LINK static bool GRAYCALL StartsWithI(const TYPE* pszStr2, const TYPE* pszPrefix);
		template< typename TYPE >
		GRAYCORE_LINK static bool GRAYCALL EndsWithI(const TYPE* pszStr2, const TYPE* pszPostfix, StrLen_t nLenStr = k_StrLen_UNK);

		// TODO constexpr ?
		template< typename TYPE >
		GRAYCORE_LINK static HASHCODE32_t GRAYCALL GetHashCode32(const TYPE* pszStr, StrLen_t nLen = k_StrLen_UNK, HASHCODE32_t nHash = k_HASHCODE_CLEAR) NOEXCEPT;

		template< typename TYPE >
		GRAYCORE_LINK static TYPE* GRAYCALL FindChar(const TYPE* pszStr, TYPE ch, StrLen_t iLen = StrT::k_LEN_MAX) NOEXCEPT;
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL FindCharN(const TYPE* pszStr, TYPE ch) NOEXCEPT;
		template< typename TYPE >
		static bool HasChar(const TYPE* pszStr, TYPE ch) NOEXCEPT
		{
			return FindCharN(pszStr, ch) >= 0;
		}
		template< typename TYPE >
		GRAYCORE_LINK static TYPE* GRAYCALL FindCharRev(const TYPE* pszStr, TYPE ch, StrLen_t iLen = k_StrLen_UNK);
		template< typename TYPE >
		GRAYCORE_LINK static TYPE* GRAYCALL FindTokens(const TYPE* pszStr, const TYPE* pszTokens, StrLen_t iLenMax = StrT::k_LEN_MAX);

		template< typename TYPE >
		GRAYCORE_LINK static TYPE* GRAYCALL FindStr(const TYPE* pszStr, const TYPE* pszFind, StrLen_t iLenMax = StrT::k_LEN_MAX);
		template< typename TYPE >
		GRAYCORE_LINK static TYPE* GRAYCALL FindStrI(const TYPE* pszStr, const TYPE* pszFind, StrLen_t iLenMax = StrT::k_LEN_MAX);
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL FindWord(const TYPE* pTextSearch, const TYPE* pszKeyWord, StrLen_t iLenMax = StrT::k_LEN_MAX);

		template< typename TYPE>
		static constexpr StrLen_t GetNonWhitespaceI(const TYPE* pStr, StrLen_t iLenMax = StrT::k_LEN_MAX) NOEXCEPT
		{
			//! Skip tabs and spaces but NOT new lines. NOT '\0' either.
			if (pStr == nullptr)
				return 0;
			StrLen_t i = 0;
			while (i < iLenMax && StrChar::IsSpace(pStr[i]))
				i++;
			return i;
		}
		template< typename TYPE>
		static const TYPE* GetNonWhitespace(const TYPE* pStr, StrLen_t iLenMax = StrT::k_LEN_MAX) NOEXCEPT
		{
			// never return nullptr unless pStr = nullptr
			return pStr + GetNonWhitespaceI(pStr, iLenMax) ;
		}
		template< typename TYPE>
		static TYPE* GetNonWhitespace(TYPE* pStr, StrLen_t iLenMax = StrT::k_LEN_MAX) NOEXCEPT
		{
			return pStr + GetNonWhitespaceI(pStr, iLenMax) ;
		}
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL GetWhitespaceEnd(const TYPE* pStr, StrLen_t iLenChars = k_StrLen_UNK);
		template< typename TYPE >
		GRAYCORE_LINK static bool GRAYCALL IsWhitespace(const TYPE* pStr, StrLen_t iLenChars = StrT::k_LEN_MAX) NOEXCEPT;

		template< typename TYPE >
		GRAYCORE_LINK static bool GRAYCALL IsPrintable(const TYPE* pStr, StrLen_t iLenChars = StrT::k_LEN_MAX) NOEXCEPT;

		// String searching. const
		template< typename TYPE >
		GRAYCORE_LINK static ITERATE_t GRAYCALL TableFind(const TYPE* pszFindThis, const void* ppszTableInit, size_t nElemSize = sizeof(const TYPE*));
		template< typename TYPE >
		GRAYCORE_LINK static ITERATE_t GRAYCALL TableFindHead(const TYPE* pszFindHead, const void* ppszTableInit, size_t nElemSize = sizeof(const TYPE*));
		template< typename TYPE >
		GRAYCORE_LINK static ITERATE_t GRAYCALL TableFindSorted(const TYPE* pszFindThis, const void* ppszTableInit, ITERATE_t iCountMax, size_t nElemSize = sizeof(const TYPE*));
		template< typename TYPE >
		GRAYCORE_LINK static ITERATE_t GRAYCALL TableFindHeadSorted(const TYPE* pszFindHead, const void* ppszTableInit, ITERATE_t iCountMax, size_t nElemSize = sizeof(const TYPE*));

#define STR_TABLEFIND_N(k,t)	StrT::TableFind( k, t, sizeof(t[0]))
#define STR_TABLEFIND_NH(k,t)	StrT::TableFindHead( k, t, sizeof(t[0]))
#define STR_TABLEFIND_S(k,t)	StrT::TableFindSorted( k, t, _countof(t)-1, sizeof(t[0]))
#define STR_TABLEFIND_SH(k,t)	StrT::TableFindHeadSorted( k, t, _countof(t)-1, sizeof(t[0]))

		// simple string regular expression pattern matching. i.e. *? Wildcards.
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL MatchRegEx(const TYPE* pText, const TYPE* pRegExPattern, bool bIgnoreCase, StrLen_t nTextMax = k_StrLen_UNK);

		//**********************************************************************
		// String modifying.

		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL CopyLen(TYPE* pszDst, const TYPE* pSrc, StrLen_t iLenCharsMax) NOEXCEPT; // iLenCharsMax includes room for '\0'

		template< typename TYPE >
		static void MakeUpperCase(TYPE* pszDst, StrLen_t iLenCharsMax) NOEXCEPT
		{
			//! replaces _strupr
			//! No portable __linux__ equiv to _strupr()?
			if (pszDst == nullptr)
				return;
			StrLen_t i = 0;
			for (; pszDst[i] != '\0' && i < iLenCharsMax; i++)
			{
				pszDst[i] = (TYPE)StrChar::ToUpperA(pszDst[i]);
			}
		}
		template< typename TYPE >
		static void MakeLowerCase(TYPE* pszDst, StrLen_t iLenCharsMax) NOEXCEPT
		{
			//! replaces strlwr()
			//! No portable __linux__ equiv to strlwr()?
			if (pszDst == nullptr)
				return;
			StrLen_t i = 0;
			for (; pszDst[i] != '\0' && i < iLenCharsMax; i++)
			{
				pszDst[i] = (TYPE)StrChar::ToLowerA(pszDst[i]);
			}
		}

		// like _vsntprintf
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL vsprintfN(OUT TYPE* pszOut, StrLen_t iLenOutMax, const TYPE* pszFormat, va_list vlist);

		template< typename TYPE >
		static StrLen_t _cdecl sprintfN(OUT TYPE* pszOut, StrLen_t iLenOutMax, const TYPE* pszFormat, ...)
		{
			//! Format a string with variadic arguments. Truncate at iLenOutMax if necessary.
			//! @arg
			//!  iLenOutMax = max output size in characters. (Not Bytes) Must allow space for '\0'
			//! @return
			//!  size in characters. -1 = too small. (NOT CONSISTENT WITH LINUX!)
			va_list vargs;
			va_start(vargs, pszFormat);
			const StrLen_t nLenRet = StrT::vsprintfN(pszOut, iLenOutMax, pszFormat, vargs);
			va_end(vargs);
			return nLenRet;
		}

		template< typename TYPE >
		GRAYCORE_LINK static TYPE* GRAYCALL FindBlockEnd(STR_BLOCK_TYPE eBlockType, const TYPE* pLine, StrLen_t iLenMax = StrT::k_LEN_MAX);
		template< typename TYPE>
		GRAYCORE_LINK static TYPE* GRAYCALL StripBlock(TYPE* pszText);

		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqRemove(TYPE* pStrOut, const TYPE* pStrIn, StrLen_t iLenOutMax = StrT::k_LEN_MAX, StrLen_t iLenInMax = StrT::k_LEN_MAX);
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqRemoveQ(TYPE* pStrOut, const TYPE* pStrIn, StrLen_t iLenOutMax = StrT::k_LEN_MAX, StrLen_t iLenInMax = StrT::k_LEN_MAX);

		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqAdd(TYPE* pStrOut, const TYPE* pStrIn, StrLen_t iLenOutMax = StrT::k_LEN_MAX);
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL EscSeqAddQ(TYPE* pStrOut, const TYPE* pStrIn, StrLen_t iLenOutMax = StrT::k_LEN_MAX);

		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL TrimWhitespaceEnd(TYPE* pStr, StrLen_t iLenChars = k_StrLen_UNK);
		template< typename TYPE >
		GRAYCORE_LINK static TYPE* GRAYCALL TrimWhitespace(TYPE* pStr, StrLen_t iLenMax = StrT::k_LEN_MAX);

		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL ReplaceX(TYPE* pDst, StrLen_t iDstLenMax, StrLen_t nDstIdx, StrLen_t iDstSegLen, const TYPE* pSrc, StrLen_t iSrcLen = k_StrLen_UNK);

		template< typename TYPE >
		GRAYCORE_LINK static ITERATE_t GRAYCALL ParseArray(TYPE* pszCmdLine, StrLen_t nCmdLenMax, TYPE** ppCmds, ITERATE_t iCmdQtyMax, const TYPE* pszSep = nullptr, STRP_MASK_t uFlags = STRP_DEF);
		template< typename TYPE >
		GRAYCORE_LINK static ITERATE_t GRAYCALL ParseArrayTmp(TYPE* pszTmp, StrLen_t iTmpSizeMax, const TYPE* pszCmdLine, TYPE** ppCmds, ITERATE_t iCmdQtyMax, const TYPE* pszSep = nullptr, STRP_MASK_t uFlags = STRP_DEF);

		//**********************************************************************
		// Numerics - All numerics can be done in Latin. So just convert to use StrNum

		// string to numeric. similar to strtoul()
		template< typename TYPE >
		GRAYCORE_LINK static UINT64 GRAYCALL toUL(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/nullptr, RADIX_t nBaseRadix = 0) NOEXCEPT;
		template< typename TYPE >
		GRAYCORE_LINK static INT64 GRAYCALL toIL(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/nullptr, RADIX_t nBaseRadix = 10) NOEXCEPT;

		template< typename TYPE >
		static UINT toU(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/nullptr, RADIX_t nBaseRadix = 0) NOEXCEPT
		{
			//! Just cast down from 64.
			return (UINT)toUL(pszStr, ppszStrEnd, nBaseRadix);
		}
		template< typename TYPE >
		static int toI(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/nullptr, RADIX_t nBaseRadix = 10) NOEXCEPT
		{
			//! atoi()
			//! Just cast down from 64.
			return (int)toIL(pszStr, ppszStrEnd, nBaseRadix);
		}

		template< typename TYPE >
		static UINT_PTR toUP(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/nullptr, RADIX_t nBaseRadix = 0) NOEXCEPT
		{
			// UINT_PTR as 64 bit or 32 bit as needed.
			return (UINT_PTR)toUL(pszStr, ppszStrEnd, nBaseRadix);
		}
		template< typename TYPE >
		static INT_PTR toIP(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/nullptr, RADIX_t nBaseRadix = 10) NOEXCEPT
		{
			// INT_PTR as 64 bit or 32 bit as needed.
			return (INT_PTR)toIL(pszStr, ppszStrEnd, nBaseRadix);
		}

		// numeric to string
		template< typename TYPE>
		GRAYCORE_LINK static TYPE* GRAYCALL ULtoA2(UINT64 uVal, TYPE* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix = 10, char chRadixA = 'A');
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL ULtoA(UINT64 nVal, TYPE* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix = 10);
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL ILtoA(INT64 nVal, OUT TYPE* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix = 10);

		template< typename TYPE>
		GRAYCORE_LINK static StrLen_t GRAYCALL ULtoAK(UINT64 uVal, OUT TYPE* pszStr, StrLen_t iStrMax, UINT nKUnit = 1024, bool bSpace = true);

		template< typename TYPE >
		static StrLen_t UtoA(UINT32 nVal, OUT TYPE* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10)
		{
			//! Convert an UINT/UINT32/DWORD to a string. like sprintf("%u")
			//! Assume auto convert BYTE, WORD to UINT/UINT32/DWORD.
			//! Just cast up to 64.
			return ULtoA(nVal, pszOut, iStrMax, nBaseRadix);	// convert 32 bit up to 64 bit.
		}
		template< typename TYPE >
		static StrLen_t ItoA(INT32 nVal, OUT TYPE* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10)
		{
			//! Convert an int/INT32 to a string.
			//! Assume auto convert char, short to INT32.
			//! like ltoa(); or ToString(), or like sprintf("%d")
			//! Just cast up to 64.
			return ILtoA(nVal, pszOut, iStrMax, nBaseRadix);	// convert 32 bit up to 64 bit.
		}

		// Floats/Doubles
		template< typename TYPE>
		GRAYCORE_LINK static double GRAYCALL toDouble(const TYPE* pszStr, const TYPE** ppszStrEnd = /*(const TYPE**)*/ nullptr);
		template< typename TYPE >
		GRAYCORE_LINK static StrLen_t GRAYCALL DtoA(double nVal, OUT TYPE* pszOut, StrLen_t iStrMax, int iDecPlaces = -1, char chE = -'e');
	};

	template< typename TYPE = char >
	struct GRAYCORE_LINK StrX : public StrT	// static
	{
		//! @struct Gray::StrX
		//! Type cannot be derived from arguments. we must declare char type specifically.

		static const TYPE* GRAYCALL GetBoolStr(bool bVal) NOEXCEPT;

		// String searching. const
		static inline const TYPE* GetTableElemU(const void* ppszTableInit, ITERATE_t i, size_t nSizeElem)
		{
			//! U = I dont know the max yet.
			//! @arg ppszTableInit = array of structures containing pointers to strings.
			ASSERT(ppszTableInit != nullptr);
			return *((const TYPE* const*)(((const BYTE*)ppszTableInit) + (i * nSizeElem)));
		}

		static const TYPE* GRAYCALL GetTableElem(ITERATE_t iEnumVal, const void* ppszTableInit, ITERATE_t iCountMax, size_t nElemSize = sizeof(const TYPE*));

		static ITERATE_t GRAYCALL GetTableCount(const void* ppszTableInit, size_t nElemSize);
		static ITERATE_t GRAYCALL GetTableCountSorted(const void* ppszTableInit, size_t nElemSize);
	};

	// Override implementations

	template<> StrLen_t inline StrT::Len<char>(const char* pszStr) NOEXCEPT	// count of chars NOT same as bytes (size_t)
	{
		//! Get length of string not including '\0'. Like strlen()
		//! Danger. ASSUME sane iLenMax <= k_LEN_MAX ??  Dont use this function. use length limited version.
		//! nullptr is NOT allowed by ::strlen(). ASSERT?
#if USE_CRT
		if (pszStr == nullptr)
			return 0;
		return (StrLen_t) ::strlen(pszStr);
#elif defined(__GNUC__)
		return (StrLen_t) ::__builtin_strlen(p1, p2, nSizeBlock);
#else
		return Len(pszStr, k_LEN_MAX);
#endif
	}
	template<> StrLen_t inline StrT::Len<wchar_t>(const wchar_t* pszStr) NOEXCEPT
	{
		//! Get length of string not including '\0'
		//! Danger. ASSUME sane iLenMax <= k_LEN_MAX ??  Dont use this function. use length limited version.
		//! nullptr is NOT allowed by wcslen. ASSERT?
#if USE_CRT
		if (pszStr == nullptr)
			return 0;
		return (StrLen_t) ::wcslen(pszStr);
#else
		return Len(pszStr, k_LEN_MAX);
#endif
	}

	template<> inline UINT64 StrT::toUL<char>(const char* pszStr, const char** ppszStrEnd, RADIX_t nBaseRadix) NOEXCEPT
	{
		// Direct to Latin.
		return StrNum::toUL(pszStr, ppszStrEnd, nBaseRadix);
	}
	template<> inline UINT64 StrT::toUL<wchar_t>(const wchar_t* pszStr, const wchar_t** ppszStrEnd, RADIX_t nBaseRadix) NOEXCEPT
	{
		// Convert to Latin.
		char szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 4];
		StrNum::GetNumberString(szTmp, pszStr, StrNum::k_LEN_MAX_DIGITS_INT);
		const char* ppszStrEndA = nullptr;
		const UINT64 nVal = StrNum::toUL(szTmp, &ppszStrEndA, nBaseRadix);
		if (ppszStrEnd != nullptr)
		{
			*ppszStrEnd = pszStr + StrT::Diff(ppszStrEndA, szTmp);
		}
		return nVal;
	}

	template<> inline INT64 StrT::toIL<char>(const char* pszStr, const char** ppszStrEnd, RADIX_t nBaseRadix)
	{
		return StrNum::toIL(pszStr, ppszStrEnd, nBaseRadix);
	}
	template<> inline INT64 StrT::toIL<wchar_t>(const wchar_t* pszStr, const wchar_t** ppszStrEnd, RADIX_t nBaseRadix)
	{
		char szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 4];
		StrNum::GetNumberString(szTmp, pszStr, StrNum::k_LEN_MAX_DIGITS_INT);
		const char* ppszStrEndA;
		const INT64 nVal = StrNum::toIL(szTmp, &ppszStrEndA, nBaseRadix);
		if (ppszStrEnd != nullptr)
		{
			*ppszStrEnd = pszStr + StrT::Diff(ppszStrEndA, szTmp);
		}
		return nVal;
	}

	template<> inline double StrT::toDouble<char>(const char* pszStr, const char** ppszStrEnd) // static
	{
#if USE_CRT
		if (pszStr == nullptr)
			return 0;
		return ::strtod(pszStr, (char**)ppszStrEnd);	// const_cast
#else
		return StrNum::toDouble(pszStr, ppszStrEnd);
#endif
	}
	template<> inline double StrT::toDouble<wchar_t>(const wchar_t* pszStr, const wchar_t** ppszStrEnd) // static
	{
#if USE_CRT
		if (pszStr == nullptr)
			return 0;
		return ::wcstod(pszStr, (wchar_t**)ppszStrEnd);	// const_cast
#else
		// Convert to char string first.
		char szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
		StrNum::GetNumberString(szTmp, pszStr, StrNum::k_LEN_MAX_DIGITS);
		const char* ppszStrEndA;
		const double nVal = StrNum::toDouble(szTmp, &ppszStrEndA);
		if (ppszStrEnd != nullptr)
		{
			*ppszStrEnd = pszStr + StrT::Diff(ppszStrEndA, szTmp);
		}
		return nVal;
#endif
	}

	template<> inline char* StrT::ULtoA2<char>(UINT64 uVal, char* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix, char chRadixA) // static
	{
		return StrNum::ULtoA2(uVal, pszOut, iOutMax, nBaseRadix, chRadixA);
	}
	template<> inline wchar_t* StrT::ULtoA2<wchar_t>(UINT64 uVal, wchar_t* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix, char chRadixA) // static
	{
		char szTmp[StrNum::k_LEN_MAX_DIGITS_INT];
		char* pszRetA = StrNum::ULtoA2(uVal, szTmp, STRMAX(szTmp), nBaseRadix, chRadixA);
		StrLen_t iLenInc = StrT::Diff(szTmp + STRMAX(szTmp), pszRetA);
		if (iLenInc > iOutMax)
			iLenInc = iOutMax;
		pszOut += (iOutMax - iLenInc);
		StrU::UTF8toUNICODE(pszOut, iLenInc, pszRetA, iLenInc);
		return pszOut;
	}

	template<> inline StrLen_t StrT::ULtoA<char>(UINT64 uVal, char* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix) // static
	{
		return StrNum::ULtoA(uVal, pszOut, iOutMax, nBaseRadix);
	}
	template<> inline StrLen_t StrT::ULtoA<wchar_t>(UINT64 uVal, wchar_t* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix) // static
	{
		char szTmp[StrNum::k_LEN_MAX_DIGITS_INT];
		const StrLen_t iStrLen = StrNum::ULtoA(uVal, szTmp, _countof(szTmp), nBaseRadix);
		return StrU::UTF8toUNICODE(pszOut, iOutMax, szTmp, iStrLen);
	}
	template<> inline StrLen_t StrT::ILtoA<char>(INT64 uVal, char* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix) // static
	{
		return StrNum::ILtoA(uVal, pszOut, iOutMax, nBaseRadix);
	}
	template<> inline StrLen_t StrT::ILtoA<wchar_t>(INT64 uVal, wchar_t* pszOut, StrLen_t iOutMax, RADIX_t nBaseRadix) // static
	{
		char szTmp[StrNum::k_LEN_MAX_DIGITS_INT];
		const StrLen_t iStrLen = StrNum::ILtoA(uVal, szTmp, _countof(szTmp), nBaseRadix);
		return StrU::UTF8toUNICODE(pszOut, iOutMax, szTmp, iStrLen);
	}

	template<> inline StrLen_t StrT::DtoA<char>(double nVal, OUT char* pszOut, StrLen_t iStrMax, int iDecPlaces, char chE) // static
	{
#if 0
		return StrNum::DToATestLegacy(nVal, pszOut, iStrMax, iDecPlaces);
#else
		return StrNum::DtoAG(nVal, pszOut, iStrMax, iDecPlaces, chE);
#endif
	}

	template<> inline StrLen_t StrT::DtoA<wchar_t>(double nVal, OUT wchar_t* pszOut, StrLen_t iOutMax, int iDecPlaces, char chE) // static
	{
		char szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
#if 0
		const StrLen_t iStrLen = StrNum::DToATestLegacy(nVal, szTmp, _countof(szTmp), iDecPlaces);
#else
		const StrLen_t iStrLen = StrNum::DtoAG2(nVal, szTmp, iDecPlaces, chE);
#endif
		return StrU::UTF8toUNICODE(pszOut, iOutMax, szTmp, iStrLen);
	}


	template<> inline StrLen_t StrT::ULtoAK<char>(UINT64 uVal, OUT char* pszOut, StrLen_t iStrMax, UINT nKUnit, bool bSpace) // static
	{
		return StrNum::ULtoAK(uVal, pszOut, iStrMax, nKUnit, bSpace);
	}

	template<> inline StrLen_t StrT::ULtoAK<wchar_t>(UINT64 uVal, OUT wchar_t* pszOut, StrLen_t iStrMax, UINT nKUnit, bool bSpace) // static
	{
		char szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
		const StrLen_t iStrLen = StrNum::ULtoAK(uVal, szTmp, _countof(szTmp), nKUnit, bSpace);
		return StrU::UTF8toUNICODE(pszOut, iStrMax, szTmp, iStrLen);
	}

	// Override implementations

	template<> inline const char* StrX<char>::GetBoolStr(bool bVal) NOEXCEPT // static
	{
		// Simpler than using "true" : "false"
		return bVal ? "1" : "0";
	}
	template<> inline const wchar_t* StrX<wchar_t>::GetBoolStr(bool bVal) NOEXCEPT // static
	{
		return bVal ? L"1" : L"0";
	}

	// #include "StrT.inl"
	// Force instantiation of stuff for char and wchar_t for linking.
}
#endif // _INC_StrT_H
