//
//! @file StrT.inl
//! included by "StrT.h" or "StrT.cpp" to implement the functions. (perhaps inline, perhaps not)
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_StrUtil_INL
#define _INC_StrUtil_INL
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrConst.h"
#include "StrA.h"
#include "cUnitTestDecl.h"

namespace Gray
{
	//***************************************************************************
	// Compares and searches

	template< typename TYPE>
	COMPARE_t GRAYCALL StrT::Cmp(const TYPE* pszStr1, const TYPE* pszStr2) NOEXCEPT
	{
		//! replace _strcmp()
		//! How does pszStr1 compare to pszStr2
		if (pszStr1 == pszStr2)
			return COMPARE_Equal;
		if (pszStr1 == nullptr)
			return COMPARE_Less;
		if (pszStr2 == nullptr)
			return COMPARE_Greater;
		for (StrLen_t i = 0;; i++)
		{
			DEBUG_CHECK(i < StrT::k_LEN_MAX);
			TYPE ch1 = pszStr1[i];
			TYPE ch2 = pszStr2[i];
			if (ch1 == '\0' || ch1 != ch2)
				return ch1 - ch2 ;
		}
	}
	template< typename TYPE>
	COMPARE_t GRAYCALL StrT::CmpN(const TYPE* pszStr1, const TYPE* pszStr2, StrLen_t iLenMaxChars) NOEXCEPT
	{
		//! How does pszStr1 compare to pszStr2
		//! replace strncmp()
		if (pszStr1 == pszStr2)
			return COMPARE_Equal;
		if (pszStr1 == nullptr)
			return COMPARE_Less;
		if (pszStr2 == nullptr)
			return COMPARE_Greater;
		if (iLenMaxChars < 0)
			return COMPARE_Less;
		DEBUG_CHECK(iLenMaxChars <= StrT::k_LEN_MAX);
		for (StrLen_t i = 0; i < iLenMaxChars; i++)
		{
			TYPE ch1 = pszStr1[i];
			TYPE ch2 = pszStr2[i];
			if (ch1 == '\0' || ch1 != ch2)
				return ch1 - ch2 ;
		}
		return COMPARE_Equal;
	}

	template< typename TYPE>
	COMPARE_t GRAYCALL StrT::CmpI(const TYPE* pszStr1, const TYPE* pszStr2) NOEXCEPT
	{
		//! @note for some reason the M$ version fails in static initializers in release mode !?
		//! replace _strcmpi strcmpi _stricmp
		//! "#define _stricmp	strcasecmp"
		if (pszStr1 == pszStr2)
			return COMPARE_Equal;
		if (pszStr1 == nullptr)
			return COMPARE_Less;
		if (pszStr2 == nullptr)
			return COMPARE_Greater;
		for (StrLen_t i = 0;; i++)
		{
			DEBUG_CHECK(i < StrT::k_LEN_MAX);
			COMPARE_t iDiff = StrChar::CmpI(pszStr1[i], pszStr2[i]);
			if (pszStr1[i] == '\0' || iDiff != 0)
				return iDiff;
		}
	}

	template< typename TYPE>
	COMPARE_t GRAYCALL StrT::CmpIN(const TYPE* pszStr1, const TYPE* pszStr2, StrLen_t iLenMaxChars) NOEXCEPT
	{
		//! Find matching string up to length iLenMaxChars. (unless '/0' is found)
		//! replaces _strnicmp strnicmp
		//! "#define _strnicmp	strncasecmp"
		if (pszStr1 == pszStr2)
			return COMPARE_Equal;
		if (pszStr1 == nullptr)
			return COMPARE_Less;
		if (pszStr2 == nullptr)
			return COMPARE_Greater;
		if (iLenMaxChars < 0)
			return COMPARE_Less;
		DEBUG_CHECK(iLenMaxChars <= StrT::k_LEN_MAX);
		for (StrLen_t i = 0; i < iLenMaxChars; i++)
		{
			COMPARE_t iDiff = StrChar::CmpI(pszStr1[i], pszStr2[i]);
			if (pszStr1[i] == '\0' || iDiff != 0)
				return iDiff;
		}
		return COMPARE_Equal;
	}

	template< typename TYPE>
	bool StrT::StartsWithI(const TYPE* pszStr1, const TYPE* pszPrefix)
	{
		//! Compare pszStr1 up to the length of pszPrefix
		//! Similar to .NET StartsWith() https://msdn.microsoft.com/en-us/library/system.string.startswith%28v=vs.110%29.aspx?f=255&MSPPError=-2147217396
		//! Look for a prefix. Similar to CmpHeadI

		if (pszStr1 == nullptr)
			pszStr1 = cStrConst::k_Empty;
		if (pszPrefix == nullptr)
			pszPrefix = cStrConst::k_Empty;

		for (StrLen_t i = 0;; i++)
		{
			ASSERT(i < StrT::k_LEN_MAX);
			TYPE ch = pszPrefix[i];
			if (ch == '\0')
				return true;	// consider this equal.
			COMPARE_t iDiff = StrChar::CmpI(ch, pszStr1[i]);
			if (iDiff != COMPARE_Equal)
				return false;
		}
	}

	template< typename TYPE>
	bool GRAYCALL StrT::EndsWithI(const TYPE* pszStr1, const TYPE* pszPostfix, StrLen_t nLenStr)
	{
		//! Compare the end of pszStr1 with pszPostfix
		//! Similar to .NET EndsWith() 
		//! Look for a pszPostfix ignoring case.

		if (pszStr1 == nullptr)
			pszStr1 = cStrConst::k_Empty;
		if (pszPostfix == nullptr)
			pszPostfix = cStrConst::k_Empty;
		if (nLenStr <= k_StrLen_UNK)
		{
			nLenStr = Len(pszStr1);
		}

		ASSERT(nLenStr >= 0 && nLenStr < StrT::k_LEN_MAX);
		StrLen_t nLenPost = Len(pszPostfix, nLenStr + 1);	// Assume this is short.
		ASSERT(nLenPost < StrT::k_LEN_MAX);
		if (nLenPost > nLenStr)
			return false;

		return StrT::CmpI(pszStr1 + (nLenStr - nLenPost), pszPostfix) == COMPARE_Equal;
	}

	template< typename TYPE>
	COMPARE_t GRAYCALL StrT::CmpHeadI(const TYPE* pszFindHead, const TYPE* pszTableElem)
	{
		//! Does pszTabkleElem start with the prefix pszFindHead?
		//! Compare only up to the length of pszTableElem.
		//! If pszFindHead has more chars but are separated by non alnum() then ignore.
		//! Follows the rules for symbolic names.
		//! similar to StartsWithI ?
		//! @note
		//!  we may want to allow /- in names for HTTP?
		//! @return
		//!  0 = match.

		if (pszTableElem == nullptr)
			return COMPARE_Less;
		if (pszFindHead == nullptr)
			return COMPARE_Less;
		for (StrLen_t i = 0;; i++)
		{
			ASSERT(i < StrT::k_LEN_MAX);
			TYPE ch1 = pszFindHead[i];
			TYPE ch2 = pszTableElem[i];
			if (!StrChar::IsCSym(ch1))
				ch1 = '\0';	// force end
			if (!StrChar::IsCSym(ch2))
				ch2 = '\0';	// force end
			COMPARE_t iDiff = StrChar::CmpI(ch1, ch2);
			if (iDiff != 0 || ch1 == '\0')
			{
				return iDiff;
			}
		}
	}

	template< typename TYPE>
	HASHCODE32_t GRAYCALL StrT::GetHashCode32(const TYPE* pszStr, StrLen_t nLen, HASHCODE32_t nHash) NOEXCEPT
	{
		//! Get a HASHCODE32_t for the string. Ignore case.
		//! based on http://www.azillionmonkeys.com/qed/hash.html super fast hash.
		//! Need not be truly unique. Just most likely unique. Low chance of collision in random set.
		//! Even distribution preferred. Simple CRC32 does not produce good distribution?
		//! Never return 0 except for empty string. not k_HASHCODE_CLEAR
		//! Others: http://sites.google.com/site/murmurhash/, boost string_hash()
		//! @note equivalent UNICODE and char strings should return the same HASHCODE32_t.

		if (pszStr == nullptr)
			return k_HASHCODE_CLEAR;
		if (nLen <= k_StrLen_UNK)
			nLen = StrT::Len(pszStr, k_LEN_MAX);
		if (nLen <= 0)
			return k_HASHCODE_CLEAR;

		for (StrLen_t nLen2 = nLen / 2; nLen2 > 0; nLen2--)
		{
			nHash += StrChar::ToUpperA(pszStr[0]);
			HASHCODE32_t tmp = (StrChar::ToUpperA(pszStr[1]) << 11) ^ nHash;
			nHash = (nHash << 16) ^ tmp;
			pszStr += 2;
			nHash += nHash >> 11;
		}

		if (nLen & 1)		// Handle end/odd case.
		{
			nHash += StrChar::ToUpperA(pszStr[0]);
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
		if (nHash == k_HASHCODE_CLEAR)
			return 1;	// NOT k_HASHCODE_CLEAR
		return nHash;
	}

	//***********************************************************

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::FindCharN(const TYPE* pszStr, TYPE chFind) NOEXCEPT
	{
		//! Find index of the first occurrence of a single char in a string.
		//! @return -1 = k_StrLen_UNK = not found.
		if (pszStr != nullptr)
		{
			for (int i = 0;; i++)
			{
				TYPE chN = pszStr[i];
				if (chN == chFind)
					return i;
				if (chN == '\0')	// found the end.
					break;
			}
		}
		return k_StrLen_UNK;
	}

	template< typename TYPE>
	TYPE* GRAYCALL StrT::FindChar(const TYPE* pszStr, TYPE chFind, StrLen_t iLenMax) NOEXCEPT
	{
		//! Find first occurrence of a single char in a string.
		//! replace strchr(), and memchr()
		//! @return nullptr = not found.
		if (pszStr != nullptr)
		{
			for (; iLenMax > 0; iLenMax--)
			{
				TYPE chN = *pszStr;
				if (chN == chFind)
					return const_cast<TYPE*>(pszStr);
				if (chN == '\0')	// found the end.
					return nullptr;
				pszStr++;
			}
		}
		return nullptr;
	}

	template< typename TYPE>
	TYPE* GRAYCALL StrT::FindCharRev(const TYPE* pszStr, TYPE chFind, StrLen_t iLenMax)
	{
		//! Find last occurrence of a single char in a string.
		//! replace strrchr() or _tcsrchr(). find TYPE from end.
		if (pszStr == nullptr)
			return nullptr;
		if (iLenMax <= k_StrLen_UNK)
		{
			iLenMax = StrT::Len(pszStr);
		}
		while (--iLenMax > 0)
		{
			if (pszStr[iLenMax] == chFind)
				return const_cast<TYPE*>(pszStr + iLenMax);
		}
		return nullptr;
	}

	template< typename TYPE>
	TYPE* GRAYCALL StrT::FindTokens(const TYPE* pszStr, const TYPE* pszTokens, StrLen_t iLenMaxChars)
	{
		//! Find one of the char pszTokens in pszStr.
		//! @return
		//!  nullptr = none found.
		if (pszStr == nullptr || pszTokens == nullptr)
			return nullptr;
		for (StrLen_t i = 0;; i++, pszStr++)
		{
			if (i >= iLenMaxChars)
				return nullptr;
			TYPE ch = *pszStr;
			if (ch == '\0')
				return nullptr;
			if (StrT::HasChar(pszTokens, ch))
				return const_cast<TYPE*>(pszStr);
		}
	}

	template< typename TYPE>
	TYPE* GRAYCALL StrT::FindStr(const TYPE* pszText, const TYPE* pszSubStr, StrLen_t iLenMaxChars)
	{
		//! Find pszSubStr inside pszText. (exact case match of all chars in pszSubStr)
		//! replaces strstr(), or .NET Contains()

		if (pszText == nullptr || pszSubStr == nullptr)
			return nullptr;

		StrLen_t i = 0;
		for (StrLen_t iMatch = 0; i < iLenMaxChars; i++)
		{
			TYPE ch = pszText[i];
			if (ch == '\0')
				break;
			if (ch == pszSubStr[iMatch])
			{
				iMatch++;	// look for next char.
				if (pszSubStr[iMatch] == '\0')
				{
					return const_cast <TYPE*>((pszText + i - iMatch) + 1);	// found match!
				}
			}
			else if (iMatch != 0)
			{
				// must revert back to start of non match.
				i -= iMatch;
				iMatch = 0;
			}
		}
		return nullptr;
	}

	template< typename TYPE>
	TYPE* GRAYCALL StrT::FindStrI(const TYPE* pszText, const TYPE* pszSubStr, StrLen_t iLenMaxChars)
	{
		//! Find pszSubStr in pszText. (ignores case)
		//! like strstr() but ignores case like stristr()
		//! @return nullptr = can't find it.

		if (pszText == nullptr || pszSubStr == nullptr)
			return nullptr;

		StrLen_t i = 0;
		for (StrLen_t iMatch = 0; i < iLenMaxChars; i++)
		{
			TYPE ch = pszText[i];
			if (ch == '\0')
				break;
			if (StrChar::CmpI(ch, pszSubStr[iMatch]) == 0)	// ignore case.
			{
				iMatch++;	// look for next char.
				if (pszSubStr[iMatch] == '\0')
				{
					return const_cast <TYPE*>((pszText + i - iMatch) + 1);	// found match!
				}
			}
			else if (iMatch != 0)
			{
				// must revert back to start of non match.
				i -= iMatch;
				iMatch = 0;
			}
		}
		return nullptr;
	}

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::FindWord(const TYPE* pszText, const TYPE* pszKeyWord, StrLen_t iLenMaxChars)
	{
		//! Find the pszKeyWord in the pszText string. Ignore Case.
		//! like FindStrI() but looks for starts of words. not match mid word.
		//! @return index of the END of the word match. <=0 = not found.

		if (pszText == nullptr)
			return k_StrLen_UNK;
		if (pszKeyWord == nullptr)
			return k_StrLen_UNK;

		StrLen_t j = 0;
		StrLen_t i = 0;
		for (; i < iLenMaxChars; i++)
		{
			TYPE ch = pszText[i];
			if (pszKeyWord[j] == '\0')
			{
				if (StrChar::IsAlNum(ch))	// partials don't count.
					break;
				return i;	// found it.
			}
			if (ch == '\0')
				return(0);
			if (!j && i)
			{
				if (StrChar::IsAlpha(pszText[i - 1]))	// not start of word ?
					continue;
			}
			if (StrChar::ToUpperA(ch) == StrChar::ToUpperA(pszKeyWord[j]))
				j++;
			else
				j = 0;
		}
		return(0);	// NOT Found
	}

	//*************************************************************

	template< typename TYPE>
	bool GRAYCALL StrT::IsWhitespace(const TYPE* pStr, StrLen_t iLenMaxChars) NOEXCEPT
	{
		//! Is the whole string whitespace, empty or nullptr?
		//! Like .NET String.IsNullOrWhiteSpace()
		//! @arg iLenMaxChars = don't bother checking more than this.

		if (pStr == nullptr)
			return true;
		if (iLenMaxChars > StrT::k_LEN_MAX)
			iLenMaxChars = StrT::k_LEN_MAX;
		for (; iLenMaxChars > 0 && pStr[0] != '\0'; iLenMaxChars--, pStr++)
		{
			if (!StrChar::IsSpaceX(pStr[0]))
				return false;
		}
		return true;
	}

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::GetWhitespaceEnd(const TYPE* pStr, StrLen_t iLenChars) // static
	{
		//! Walk backwards from the end of the string.
		//! @return
		//!  get Length of the string minus ending whitespace. (strips new lines)
		//!  0 = it was all spaces.

		if (pStr == nullptr)
			return 0;
		if (iLenChars <= k_StrLen_UNK)
		{
			iLenChars = Len(pStr);
		}

		while (iLenChars > 0 && StrChar::IsSpaceX(pStr[iLenChars - 1]))
		{
			iLenChars--;
		}

		return iLenChars;
	}

	template< typename TYPE>
	bool GRAYCALL StrT::IsPrintable(const TYPE* pStr, StrLen_t iLenChars) NOEXCEPT
	{
		//! Is this a normally printable string?
		if (pStr == nullptr)
			return false;
		for (StrLen_t i = 0; i < iLenChars; i++)
		{
			TYPE ch = pStr[i];
			if (ch == '\0')	// Is this an abnormal termination ?
				break;
			if (StrChar::IsSpaceX(ch))
				continue;
			if (StrChar::IsPrint(ch))
				continue;
			return false;
		}
		return true;
	}

	//***********************************************************

	template< typename TYPE >
	const TYPE* GRAYCALL StrX<TYPE>::GetTableElem(ITERATE_t iEnumVal, const void* ppszTableInit, ITERATE_t iCountMax, size_t nElemSize)
	{
		//! Get a string for an enum value.
		//! iCountMax = I know the max.
		if (IS_INDEX_BAD(iEnumVal, iCountMax))
		{
			return StrT::Cast<TYPE>(CSTRCONST("?"));
		}
		return GetTableElemU(ppszTableInit, iEnumVal, nElemSize);
	}

	template< typename TYPE>
	ITERATE_t GRAYCALL StrX<TYPE>::GetTableCount(const void* ppszTableInit, size_t nElemSize)
	{
		//! Count the size of a null terminated table of strings.
		//! Don't assume sorted.
		if (ppszTableInit == nullptr)
			return 0;
		const TYPE* const* ppszTable = (const TYPE* const*)ppszTableInit;
		for (ITERATE_t i = 0;; i++)
		{
			const TYPE* pszName = GetTableElemU(ppszTable, i, nElemSize);
			if (pszName == nullptr)
				return i;
		}
		UNREACHABLE_CODE(__noop);
	}

	template< typename TYPE>
	ITERATE_t GRAYCALL StrX<TYPE>::GetTableCountSorted(const void* ppszTableInit, size_t nElemSize)
	{
		//! make sure the table actually IS sorted !
		//! ignores case

		if (ppszTableInit == nullptr)
			return 0;
		const TYPE* const* ppszTable = (const TYPE* const*)ppszTableInit;
		for (ITERATE_t i = 0;; i++)
		{
			const TYPE* pszName1 = GetTableElemU(ppszTable, i, nElemSize);
			if (pszName1 == nullptr)
				return 0;
			const TYPE* pszName2 = GetTableElemU(ppszTable, i + 1, nElemSize);
			if (pszName2 == nullptr)
				return i + 1;
			COMPARE_t iCompare = StrT::CmpI(pszName1, pszName2);
			if (iCompare >= COMPARE_Equal) // no dupes!
			{
				ASSERT(iCompare < 0);
				return k_ITERATE_BAD;
			}
		}
	}

	//***********************************************************

	template< typename TYPE>
	ITERATE_t GRAYCALL StrT::TableFindHead(const TYPE* pszFindHead, const void* ppszTableInit, size_t nElemSize)
	{
		//! Find a string in a table.
		//! Ignores case. unsorted table.
		//! Use rules for StrT::CmpHeadI for compare. compare only up to the CSYM values.
		//! @return
		//!  -1 = not found. k_ITERATE_BAD
		if (StrT::IsNullOrEmpty(pszFindHead))
			return k_ITERATE_BAD;
		if (ppszTableInit == nullptr)
			return k_ITERATE_BAD;

		const TYPE* const* ppszTable = (const TYPE* const*)ppszTableInit;
		for (ITERATE_t i = 0;; i++)
		{
			const TYPE* pszName = StrX<TYPE>::GetTableElemU(ppszTable, i, nElemSize);
			if (pszName == nullptr)
				break;
			if (!StrT::CmpHeadI(pszFindHead, pszName))
				return(i);
		}
		return k_ITERATE_BAD;
	}

	template< typename TYPE>
	ITERATE_t GRAYCALL StrT::TableFindHeadSorted(const TYPE* pszFindHead, const void* ppszTableInit, ITERATE_t iCountMax, size_t nElemSize)
	{
		//! Find a string in a table.
		//! Do a binary search (un-cased) on a sorted table.
		//! Use rules for StrT::CmpHeadI for compare. compare only up to the CSYM values.
		//! @return -1 = not found k_ITERATE_BAD

		if (StrT::IsNullOrEmpty(pszFindHead))
			return k_ITERATE_BAD;
		if (ppszTableInit == nullptr)
			return k_ITERATE_BAD;
#ifdef _DEBUG
		ASSERT(StrX<TYPE>::GetTableCountSorted(ppszTableInit, nElemSize) == iCountMax);
#endif

		const TYPE* const* ppszTable = (const TYPE* const*)ppszTableInit;
		ITERATE_t iHigh = iCountMax - 1;
		if (iHigh < 0)
		{
			return k_ITERATE_BAD;
		}
		ITERATE_t iLow = 0;
		while (iLow <= iHigh)
		{
			ITERATE_t i = (iHigh + iLow) / 2;
			COMPARE_t iCompare = StrT::CmpHeadI(pszFindHead, StrX<TYPE>::GetTableElemU(ppszTable, i, nElemSize));
			if (iCompare == COMPARE_Equal)
				return i;
			if (iCompare > 0)
			{
				iLow = i + 1;
			}
			else
			{
				iHigh = i - 1;
			}
		}
		return k_ITERATE_BAD;
	}

	template< typename TYPE>
	ITERATE_t GRAYCALL StrT::TableFind(const TYPE* pszFindThis, const void* ppszTableInit, size_t nElemSize)
	{
		//! Find a string in a non-sorted table. Ignores case
		//! Used with STR_TABLEFIND_N
		//! @return -1 = no match. k_ITERATE_BAD

		if (ppszTableInit == nullptr || StrT::IsNullOrEmpty(pszFindThis))
			return k_ITERATE_BAD;
		const TYPE* const* ppszTable = (const TYPE* const*)ppszTableInit;
		for (ITERATE_t i = 0;; i++)
		{
			const TYPE* pszName = StrX<TYPE>::GetTableElemU(ppszTable, i, nElemSize);
			if (pszName == nullptr)
				break;
			if (!StrT::CmpI(pszFindThis, pszName))
				return i;
		}
		return k_ITERATE_BAD;
	}

	template< typename TYPE>
	ITERATE_t GRAYCALL StrT::TableFindSorted(const TYPE* pszFindThis, const void* ppszTableInit, ITERATE_t iCountMax, size_t nElemSize)
	{
		//! Find a string in a table.
		//! Do a binary search (un-cased) on a sorted table.
		//! @return -1 = not found k_ITERATE_BAD

		if (pszFindThis == nullptr)
			return k_ITERATE_BAD;
		if (ppszTableInit == nullptr)
			return k_ITERATE_BAD;
#ifdef _DEBUG
		ASSERT(StrX<TYPE>::GetTableCountSorted(ppszTableInit, nElemSize) == iCountMax);
#endif

		const TYPE* const* ppszTable = (const TYPE* const*)ppszTableInit;
		ITERATE_t iHigh = iCountMax - 1;
		if (iHigh < 0)
		{
			return k_ITERATE_BAD;
		}
		ITERATE_t iLow = 0;
		while (iLow <= iHigh)
		{
			ITERATE_t i = (iHigh + iLow) / 2;
			const TYPE* pszName = StrX<TYPE>::GetTableElemU(ppszTable, i, nElemSize);
			COMPARE_t iCompare = StrT::CmpI(pszFindThis, pszName);
			if (iCompare == COMPARE_Equal)
				return(i);
			if (iCompare > 0)
			{
				iLow = i + 1;
			}
			else
			{
				iHigh = i - 1;
			}
		}
		return k_ITERATE_BAD;
	}

	//*************************************************************
	// String Modifiers

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::CopyLen(TYPE* pDst, const TYPE* pSrc, StrLen_t iLenMaxChars) NOEXCEPT
	{
		//! Copy a string. replaces strncpy (sort of)
		//! @arg iLenMaxChars = _countof(Dst) = includes room for '\0'. (just like memcpy)
		//! so iLenMaxChars=_countof(Dst) is OK !
		//! @note
		//!  DO NOT assume pSrc is null terminated. tho it might be. just use iLenMaxChars
		//! @note
		//!  This will ALWAYS terminate the string (unlike strncpy)
		//! @return
		//!  Length of pDst in chars. (Not including '\0')

		if (pDst == nullptr || iLenMaxChars <= 0)
			return 0;
		StrLen_t i = 0;
		if (pSrc != nullptr)
		{
			iLenMaxChars--;	// save room for '\0'
			if (pDst >= pSrc && pDst <= (pSrc + iLenMaxChars))
			{
				i = StrT::Len(pSrc, iLenMaxChars);
				if (pDst != pSrc) // same string.
				{
					// Must do backwards copy like cMem::CopyOverlap().
					int j = i;
					for (; --j >= 0; )
						pDst[j] = pSrc[j];
				}
			}
			else
			{
				// Just copy to '\0'
				for (; pSrc[i] != '\0' && i < iLenMaxChars; i++)
					pDst[i] = pSrc[i];
			}
		}

		pDst[i] = '\0';	// always terminate.
		return i;
	}

	//***************************************************************************************

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::TrimWhitespaceEnd(TYPE* pStr, StrLen_t iLenChars)
	{
		//! Trim any whitespace off the end of the string.
		//! @return
		//!  new length of the line. (without whitespace and comments)
		if (pStr == nullptr)
			return 0;
		if (iLenChars == k_StrLen_UNK)
		{
			iLenChars = StrT::Len(pStr);
		}
		iLenChars = StrT::GetWhitespaceEnd(pStr, iLenChars);
		if (pStr[iLenChars] != '\0')
		{
			pStr[iLenChars] = '\0';
		}
		return iLenChars;
	}

	template< typename TYPE>
	TYPE* GRAYCALL StrT::TrimWhitespace(TYPE* pStr, StrLen_t iLenMax)
	{
		//! Trim starting AND ending whitespace
		TYPE* pStrStart = pStr;
		pStr = GetNonWhitespace(pStr, iLenMax);
		iLenMax -= Diff(pStr, pStrStart);
		StrT::TrimWhitespaceEnd(pStr, StrT::Len(pStr, iLenMax));
		return pStr;
	}

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::ReplaceX(TYPE* pDst, StrLen_t iDstLenMax, StrLen_t iDstIdx, StrLen_t iDstSegLen, const TYPE* pSrc, StrLen_t iSrcLen)
	{
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

		if (iSrcLen <= k_StrLen_UNK)
		{
			iSrcLen = StrT::Len(pSrc);
		}
		TYPE* pDst2 = pDst + iDstIdx;
		StrLen_t iLenRest = StrT::Len(pDst2 + iDstSegLen);
		if (iDstIdx + iSrcLen + iLenRest >= iDstLenMax)	// not big enough!
			return 0;
		cMem::CopyOverlap(pDst2 + iSrcLen, pDst2 + iDstSegLen, (iLenRest + 1) * sizeof(TYPE));	// make room.
		cMem::Copy(pDst2, pSrc, iSrcLen * sizeof(TYPE));
		return iSrcLen;
	}

	//******************************************************************************

	template< typename TYPE>
	TYPE* GRAYCALL StrT::FindBlockEnd(STR_BLOCK_TYPE eBlockType, const TYPE* pszLine, StrLen_t iLenMaxChars)
	{
		//! Find the end of an STR_BLOCK_TYPE sequence. (quote,brace,bracket,parenthesis)
		//! skip nested blocks. deal with quoted and escaped strings.
		//! @arg
		//!  eBlockType = what type of block is this? 0 = STR_BLOCK_QUOTE.
		//!    STR_BLOCK_NONE = just look for nested blocks.
		//!  pszLine = the TYPE char AFTER the opening quote/brace/etc STR_BLOCK_TYPE char.
		//!  iLenMaxChars = StrT::k_LEN_MAX
		//! @return
		//!  Pointer to the end of the block sequence. Perhaps caller will replace it with '\0' ?
		//!  nullptr = FAIL, same pointer = FAIL.

		if (pszLine == nullptr)
			return nullptr;
		StrLen_t i = 0;
		for (; i < iLenMaxChars; i++)
		{
			TYPE ch = pszLine[i];
			if (ch == '\0')	// nothing closing.
			{
				break;
			}

			if (eBlockType != STR_BLOCK_NONE && ch == (TYPE)(k_szBlockEnd[eBlockType]))
			{
				// Found the end. OK.
				return(const_cast<TYPE*>(pszLine + i));
			}

			// NO Such thing as nested blocks inside " quote. only look for quote end.
			if (eBlockType == STR_BLOCK_QUOTE)	// so don't bother looking.
			{
				// skip \escape sequences inside quotes.
				if (ch == '\\' && pszLine[i + 1] > ' ')
				{
					i++;
				}
				continue;
			}

			// Unmatched block ends inside? BAD. Terminate.
			if (StrT::HasChar(k_szBlockEnd + STR_BLOCK_QUOTE + 1, (char)ch))
			{
				if (eBlockType != STR_BLOCK_NONE)
				{
#ifdef _INC_cLogMgr_H
					DEBUG_ERR(("Unmatched internal %c mark!", ch));
#endif
				}
				break;
			}

			// Allow nested blocks of some STR_BLOCK_TYPE.
			STR_BLOCK_TYPE eBlockType2 = (STR_BLOCK_TYPE)StrT::FindCharN<char>(k_szBlockStart, (char)ch);	// cast away wchar_t
			if (eBlockType2 >= 0)
			{
				const TYPE* pszBlockEnd = StrT::FindBlockEnd(eBlockType2, pszLine + i + 1);
				if (pszBlockEnd != nullptr && (TYPE)k_szBlockEnd[eBlockType2] == pszBlockEnd[0])
				{
					i = StrT::Diff(pszBlockEnd, pszLine);
				}
				else
				{
					// Failed!
				}
			}
		}

		if (eBlockType == STR_BLOCK_NONE)
		{
			// i was just looking for nesting errors.
			return(const_cast<TYPE*>(pszLine + i));
		}

		// Failed to find closing character.
#ifdef _INC_cLogMgr_H
		DEBUG_ERR(("Unmatched ending %c mark!", k_szBlockStart[eBlockType]));
#endif
		return const_cast<TYPE*>(pszLine);
	}

	template< typename TYPE>
	TYPE* GRAYCALL StrT::StripBlock(TYPE* pszText)
	{
		//! strip block based on the very first character of the string.
		//! If the string is encased in "" or () then remove them.

		STR_BLOCK_TYPE eBlockType = (STR_BLOCK_TYPE)StrT::FindCharN<char>(k_szBlockStart, (char)pszText[0]);	// start block.
		if (eBlockType < 0)	// not blocked, at least not a type I recognize.
			return pszText;
		TYPE* pszBlockEnd = StrT::FindBlockEnd(eBlockType, pszText + 1); // const_cast
		if (pszBlockEnd == nullptr || pszBlockEnd[0] != (TYPE)k_szBlockEnd[eBlockType])	// failed to close !
			return pszText;
		*pszBlockEnd = '\0';
		return pszText + 1;
	}

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::EscSeqRemove(TYPE* pStrOut, const TYPE* pStrIn, StrLen_t iLenOutMax, StrLen_t iLenInMax)
	{
		//! Filter/decode out the 'C like' embedded escape sequences. "\n\r" etc
		//! This string will shrink.
		//! Assumed to be inside quotes ending at iLenInMax.
		//! opposite of StrT::EscSeqAdd()
		//! @note
		//!  Since we are removing, allow pStrOut = pStrIn.
		//! @arg
		//!  iLenMax = the string length NOT including null.
		//! @return new length of the string. same or less than input
		//! @return pStrOut
		//! @note should we check for internal chars < 32 NOT encoded ? THIS would be an error !!!

		ASSERT_N(pStrOut != nullptr);
		if (pStrIn == nullptr)
			return 0;

		StrLen_t j = 0;
		for (StrLen_t i = 0; i < iLenInMax && j < iLenOutMax; i++, j++)
		{
			TYPE ch = pStrIn[i];
			if (ch == '\0')
				break;
			if (ch == '\\')
			{
				ch = pStrIn[++i];
				int iEsc = StrT::FindCharN<char>(k_szEscEncode, (char)ch);
				if (iEsc >= 0 && StrChar::IsAscii(ch))
				{
					ch = k_szEscDecode[iEsc];
				}
				else if (ch == 'x')
				{
					// MUST be followed by at least 1 hex char. but can have 2
					i++;
					const TYPE* pszEndN;
					ch = (TYPE)StrT::toU(pStrIn + i, &pszEndN, 16);
					i += Diff(pszEndN, pStrIn + i) - 1;
				}
				else if (ch >= '0'&&ch <= '7')	// If it's a digit then it's octal.
				{
					// MUST be followed by at least 1 octal chars. but can have 3
					const TYPE* pszEndN;
					ch = (TYPE)StrT::toU(pStrIn + i, &pszEndN, 8);
					i += Diff(pszEndN, pStrIn + i) - 1;
				}
#if 0
				else if (ch == 'u')
				{
					// @todo u## = UNICODE encoded ! can produce multiple character UTF8 sequences.
					const TYPE* pszEndN;
					int iChar = StrT::toU(pStrIn + i, &pszEndN, 10);
				}
#endif
				else
				{
					// JUNK ? bad encoding.
					pStrOut[j++] = '\\';	// ignore it i guess.
				}
			}
			pStrOut[j] = ch;
		}
		pStrOut[j] = '\0';
		return j;
	}

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::EscSeqRemoveQ(TYPE* pStrOut, const TYPE* pStrIn, StrLen_t iLenOutMax, StrLen_t iLenInMax)
	{
		//! Remove the opening and closing quotes. Put enclosed string (decoded) into pStrOut.
		//! @return the consumed length of pStrIn. NOT the length of pStrOut.

		ASSERT_N(pStrOut != nullptr);
		if (pStrIn == nullptr)
			return 0;

		if (pStrIn[0] != '"')
		{
			// Just copy the string untranslated.
			return StrT::CopyLen<TYPE>(pStrOut, pStrIn, MIN(iLenOutMax, iLenInMax));
		}

		TYPE* pszBlockEnd = StrT::FindBlockEnd<TYPE>(STR_BLOCK_QUOTE, pStrIn + 1, iLenInMax - 1);
		if (pszBlockEnd == nullptr || pszBlockEnd[0] != '\"') 	// not sure what this is. not closed string.
		{
			return k_StrLen_UNK;	// BAD. NOT Closed.
		}

		StrLen_t iLenIn = StrT::Diff<TYPE>(pszBlockEnd, pStrIn);

		// remove escape chars.
		StrT::EscSeqRemove<TYPE>(pStrOut, pStrIn + 1, iLenOutMax, iLenIn - 1);

		// ignore junk after end quote. Thats my callers problem.
		return iLenIn + 1;
	}

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::EscSeqAdd(TYPE* pStrOut, const TYPE* pStrIn, StrLen_t iLenOutMax)
	{
		//! Encode/Replace odd chars with escape sequences. e.g. "\n"
		//! This makes the string larger!
		//! opposite of StrT::EscSeqRemove()
		//! @arg pStrOut = nullptr = test output. 0 = no quotes needed.
		//! @return
		//!   pStrOut has chars ONLY in the range of 32 to 127
		//!   new length of the string. same or more than input
		//! @note Only double quotes and "\\\b\f\n\r\t" are needed for JSON strings.

		ASSERT_N(pStrIn != nullptr);
		ASSERT_N(pStrIn != pStrOut);

		StrLen_t iOut = 0;
		for (StrLen_t iIn = 0; iOut < iLenOutMax; iIn++, iOut++)
		{
			TYPE ch = pStrIn[iIn];
			if (ch == '\0')
				break;
			// Why encode a question mark ?
			int iEsc = StrT::FindCharN<char>(k_szEscDecode, (char)ch);
			if (iEsc >= 0 && StrChar::IsAscii(ch))
			{
				if (pStrOut == nullptr)
					return 1;
				pStrOut[iOut++] = '\\';
				pStrOut[iOut] = k_szEscEncode[iEsc];
			}
			else if (!StrChar::IsPrint(ch))
			{
				// Use hex escape seq like 	\xAA
				if (pStrOut == nullptr)
					return 1;
				pStrOut[iOut++] = '\\';
				pStrOut[iOut++] = 'x';
				iOut += StrT::UtoA(ch, pStrOut + iOut, iLenOutMax - iOut, 16) - 1;
			}
			else
			{
				if (pStrOut == nullptr)
					continue;
				pStrOut[iOut] = ch;	// Not encoded.
			}
		}

		if (pStrOut == nullptr)
			return 0;
		pStrOut[iOut] = '\0';
		return iOut;
	}

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::EscSeqAddQ(TYPE* pStrOut, const TYPE* pStrIn, StrLen_t iLenOutMax)
	{
		//! Encode the string and add quotes.
		if (iLenOutMax <= 2)
		{
			return 0;
		}
		pStrOut[0] = '\"';
		StrLen_t iLen = StrT::EscSeqAdd(pStrOut + 1, pStrIn, iLenOutMax - 3);
		pStrOut[iLen + 1] = '\"';
		pStrOut[iLen + 2] = '\0';
		return iLen + 2;
	}

	//******************************************************************************

	template< typename TYPE>
	ITERATE_t GRAYCALL StrT::ParseCmds(TYPE* pszCmdLine, StrLen_t nCmdLenMax, TYPE** ppCmd, ITERATE_t iCmdQtyMax, const TYPE* pszSep, STRP_MASK_t uFlags)
	{
		//! Parse a separated list of tokens/arguments to a function/command
		//! @arg
		//!  pszCmdLine = NOTE: this is modified / chopped up buffer.
		//!  ppCmd = an array of pointers inside pszCmdLine
		//!  iCmdQtyMax = _countof(ppCmd);
		//!  pszSep = what are the valid separators.
		//! @arg uFlags = STRP_MASK_t = STRP_TYPE_. deal with quoted and escaped strings.
		//! @return
		//!  Number of args. in ppCmd
		//!  0 = empty string/array.
		//! EXAMPLES:
		//!  "tag=word&tag2=word" for HTML pages.
		//!  "arg \t arg" preserve 1 space.

#if 0 //def _DEBUG
		if (!StrT::Cmp(pszCmdLine, CSTRCONST("0,0,0,0\"")))
		{
			DEBUG_MSG(("Str::Cmp"));
		}
#endif

		ASSERT(iCmdQtyMax >= 1);	// else why bother?
		ASSERT_N(ppCmd != nullptr);
		ITERATE_t iQty = 0;
		if (pszCmdLine == nullptr)
		{
		do_cleanup:
			// nullptr terminate list if possible.
			ITERATE_t iQtyLeft = (iCmdQtyMax - iQty);
			if (iQtyLeft > 0)
			{
				if (iQtyLeft > 8)
					iQtyLeft = 8;	// keep this from ridiculous.
				cMem::Zero(&ppCmd[iQty], iQtyLeft * sizeof(TYPE*));
			}
			return iQty;
		}

		if (pszSep == nullptr)
		{
			pszSep = CSTRCONST(",");
		}
		if (nCmdLenMax <= k_StrLen_UNK || nCmdLenMax > StrT::k_LEN_MAX)
		{
			nCmdLenMax = StrT::k_LEN_MAX;
		}

		bool bUsedNonWhitespaceSep = false;
		ppCmd[0] = pszCmdLine;	// iQty=0
		StrLen_t i = 0;
		for (; i < nCmdLenMax; i++)
		{
			TYPE ch = pszCmdLine[i];
			if (ch == '\0')	// no more args i guess.
				break;
			bool bIsWhitespace = StrChar::IsSpaceX(ch);	// include newlines here.
			if (bIsWhitespace)
			{
				if ((uFlags & STRP_START_WHITE) && ch != '\r' && ch != '\n')
				{
					// trim starting whitespace.
					if (ppCmd[iQty] == (pszCmdLine + i))
					{
						ppCmd[iQty] = pszCmdLine + i + 1;
						continue;
					}
				}
				if (uFlags & STRP_MERGE_CRNL)
				{
					// merge CR and LF/NL
					if (ch == '\r' && pszCmdLine[i + 1] == '\n')	// the normal order is \r\n (13,10)
					{
						continue;
					}
				}
			}
			if (StrT::HasChar(pszSep, ch)) // found a separator.
			{
				if (iCmdQtyMax == 1)
					break;

				if (uFlags & STRP_SPACE_SEP)
				{
					// allow whitespace separators only if no other separator used.
					if (!bIsWhitespace)
					{
						bUsedNonWhitespaceSep = true;
						uFlags &= ~STRP_EMPTY_SKIP;	// empty is intentional.
					}
					else if (bUsedNonWhitespaceSep)
					{
						continue;	// ignore whitespace now.
					}
				}

				TYPE* pszCmdStart = ppCmd[iQty];
				if (uFlags & STRP_END_WHITE)
				{
					//  trim whitespace from the end of the token
					ASSERT(pszCmdLine + i >= pszCmdStart);
					StrT::TrimWhitespaceEnd(pszCmdStart, Diff(pszCmdLine + i, pszCmdStart));
				}
				else if (pszCmdLine[i] != '\0')
				{
					pszCmdLine[i] = '\0';
				}
				if (*pszCmdStart == '\0')
				{
					// Empty token
					if (uFlags & STRP_EMPTY_SKIP)
					{
						// merge/skip empty tokens
						iQty--;
					}
					else if (uFlags & STRP_EMPTY_STOP)
					{
						// Just stop on empty token.
						ppCmd[++iQty] = pszCmdStart;	// include the empty token as the terminator.
						break;
					}
				}

				// Start of the next token.
				ASSERT(iQty < iCmdQtyMax - 1);
				ppCmd[++iQty] = pszCmdLine + i + 1;

				if (iQty >= iCmdQtyMax - 1)	// this is just the last anyhow. so we are done. only get here to allow skip of whitespace.
				{
					// The last entry we have room for. so just take the rest of the args.
					i++;
					if (uFlags & STRP_START_WHITE)
					{
						i += StrT::GetNonWhitespaceI(pszCmdLine + i);
					}
					i += StrT::Len(pszCmdLine + i);
					i = MIN(i, nCmdLenMax);
					break;
				}
				continue;
			}

			if ((uFlags & STRP_CHECK_BLOCKS ) && StrChar::IsAscii(ch))
			{
				// Special block char ?
				// has some other block ending ? Thats weird.
				if (StrT::HasChar(k_szBlockEnd + STR_BLOCK_QUOTE + 1, (char)ch))
				{
					break;	// Not sure what i should do about this ?
				}

				STR_BLOCK_TYPE eBlockType = (STR_BLOCK_TYPE)StrT::FindCharN<char>(k_szBlockStart, (char)ch);	// start block.
				if (eBlockType >= 0)
				{
					const TYPE* pszBlockEnd = StrT::FindBlockEnd(eBlockType, pszCmdLine + i + 1); // const_cast
					if (pszBlockEnd == nullptr || pszBlockEnd[0] != (TYPE)k_szBlockEnd[eBlockType])	// failed to close ?
						break;	// FAIL
					i = StrT::Diff(pszBlockEnd, pszCmdLine);
				}
				continue;
			}

			// just part of the token i guess.
		}

		// clean up last entry
		TYPE* pszCmdStart = ppCmd[iQty];
		if (uFlags & STRP_END_WHITE)
		{
			ASSERT((pszCmdLine + i) >= pszCmdStart);
			StrT::TrimWhitespaceEnd(pszCmdStart, Diff(pszCmdLine + i, pszCmdStart));
		}
		else if (pszCmdLine[i] != '\0')
		{
			pszCmdLine[i] = '\0';
		}
		if (pszCmdStart[0] != '\0')	// this counts as an arg.
		{
			iQty++;
		}

		goto do_cleanup;
	}

	template< typename TYPE>
	ITERATE_t GRAYCALL StrT::ParseCmdsTmp(TYPE* pszTmp, StrLen_t iTmpSizeMax, const TYPE* pszCmdLine, TYPE** ppCmd, ITERATE_t iCmdQtyMax, const TYPE* pszSep, STRP_MASK_t uFlags)
	{
		//! Make a temporary copy of the string for parsing. 
		//! @arg iTmpSizeMax = StrT::k_LEN_MAX
		//! @arg uFlags = deal with quoted and escaped strings.
		//! @return Quantity of arguments
		StrLen_t iLenChars = StrT::CopyLen<TYPE>(pszTmp, pszCmdLine, iTmpSizeMax);
		UNREFERENCED_PARAMETER(iLenChars);
		return ParseCmds(pszTmp, iTmpSizeMax, ppCmd, iCmdQtyMax, pszSep, uFlags);
	}

	//***********************************************************

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::MatchRegEx(const TYPE* pText, const TYPE* pPattern, bool bIgnoreCase, StrLen_t nTextMax)	// static
	{
		//! Recursive very simple regex pattern match with backtracking.
		//! Will cope with match( "a.b.c", "*.c" )
		//! regex like. Case sensitive.
		//! @arg nTextMax > 0 = allow partial matches. effectively adds a * to the end.
		//! @return length of the match in pText. if < Len(pText) then its only a partial match. pText has extra stuff.

		ASSERT_N(pText != nullptr);
		ASSERT_N(pPattern != nullptr);

		StrLen_t i = 0;
		for (;; i++, pPattern++)
		{
			TYPE chP = *pPattern;
			TYPE chT = pText[i];

			if (chP == '\0')	// pattern complete. matched?
			{
				if (chT == '\0')	// Full match
					return i;
				if (nTextMax > 0)	// didnt match all the text. but its a full match of the pattern.
					return i;
				// More text = no match.
				return 0;
			}
			if (nTextMax > 0 && i >= nTextMax)
			{
				return -i;  // i didn't finish the pattern but i didn't break it either. partial match.
			}
			if (chT == '\0')		// end of text to compare. no match.
			{
				return 0;
			}
			if (chP == '*')
			{
				// * = Ignore a sequence of chars and maybe skip to the next thing?
				// NOTE: Avoid massive recursion.

				if (pPattern[1] == '\0')
				{
					if (nTextMax > 0)
					{
						return nTextMax;
					}
					return i + StrT::Len(pText + i);	// This is a Full match to the end.
				}

				// Looking for something after the *. Next matching pattern defines the end of *.
				for (int j = i; ; j++)
				{
					if (pText[j] == '\0')
					{
						return 0;	// no match. DON't allow bPartialMatch to end on a *.
					}

					StrLen_t nMatch2 = StrT::MatchRegEx(pText + j, pPattern + 1, bIgnoreCase, (nTextMax>0) ? (nTextMax - j) : nTextMax);
					if (nMatch2 != 0)
					{
						if (nMatch2 < 0)	// Run out of text but not out of pattern.
						{
							ASSERT(nTextMax > 0);
							ASSERT(j - nMatch2 >= nTextMax);
							return nMatch2 - j;
						}
						return j + nMatch2;
					}
				}
			}

			if (chP != '?')	// ? = Ignore a single char
			{
				if (bIgnoreCase)
				{
					if (StrChar::CmpI(chT, chP) != COMPARE_Equal)	// compare char.
						break;
				}
				else
				{
					if (chT != chP)	// if literal char doesn't match. Case sensitive.
						break;
				}
			}
		}

		// No match.
		return 0;
	}

	//*******************************************************************************
	// Numerics 

	template< typename TYPE>
	StrLen_t GRAYCALL StrT::ULtoAK(UINT64 uVal, OUT TYPE* pszOut, StrLen_t iStrMax, UINT nKUnit, bool bSpace)
	{
		//! Make string describing a value in K/M/G/T/P/E/Z/Y units. (Kilo,Mega,Giga,Tera,Peta,Exa,Zetta,Yotta)
		//! @arg nKUnit = 1024 default
		//! @note
		//!  Kilo=10^3,2^10, Exa=10^18,2^60, Zetta=10^21,2^70, (http://searchstorage.techtarget.com/sDefinition/0,,sid5_gci499008,00.html)
		//! @return
		//!  length of string created.

		static const char k_chKUnits[10] = "\0KMGTPEZY";
		if (nKUnit <= 0)	// default.
			nKUnit = 1024;

		double dVal = (double)uVal;
		size_t i = 0;
		for (; i < (_countof(k_chKUnits) - 2) && dVal >= nKUnit; i++)
		{
			dVal /= nKUnit;
		}

		StrLen_t iLenUse;
		if (i == 0)
		{
			iLenUse = StrT::ULtoA(uVal, pszOut, iStrMax); // _CVTBUFSIZE
		}
		else
		{
			// limit to 2 decimal places ?
			iLenUse = StrT::DtoA(dVal, pszOut, iStrMax, 2, '\0');	// NOT 'inf' or NaN has no units.
			if (iLenUse > 0 && iLenUse <= iStrMax - 3 && !StrChar::IsAlpha(pszOut[0]))
			{
				if (bSpace)	// insert a space or not?
				{
					pszOut[iLenUse++] = ' ';
				}
				pszOut[iLenUse++] = (TYPE)k_chKUnits[i];
				pszOut[iLenUse] = '\0';
			}
		}
		return iLenUse;
	}
}

#endif // _INC_StrUtil_INL
