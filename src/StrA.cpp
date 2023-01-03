//
//! @file StrA.cpp
//! String global search functions. Const strings
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrA.h"
#include "StrChar.h"
#include "cLogMgr.h"
#include "cHeap.h"
#include "cBits.h"

namespace Gray
{
	bool GRAYCALL StrA::IsBoolTrue(const char* pszStr, bool bHead) // static
	{
		//! convert string to boolean value.
		static const char* const k_pszBoolTrue[] = // static
		{
			"1",
			"On",
			"T",
			"true",		// JSON = "true", C#,.NET = "True"
			"Y",
			"Yes",
			nullptr,
		};

		if (pszStr == nullptr)
			return false;
		ITERATE_t i;
		if (bHead)
		{
			i = StrT::TableFindHeadSorted(pszStr, k_pszBoolTrue, _countof(k_pszBoolTrue) - 1);
		}
		else
		{
			i = StrT::TableFindSorted(pszStr, k_pszBoolTrue, _countof(k_pszBoolTrue) - 1);
		}
		return i >= 0;
	}

	bool GRAYCALL StrA::IsBoolFalse(const char* pszStr, bool bHead)
	{
		//! convert string to boolean value.
		static const char* const k_pszBoolFalse[] = // static
		{
			"0",
			"F",
			"false",
			"N",
			"No",
			"Off",
			nullptr,
		};
		if (pszStr == nullptr)
			return false;
		ITERATE_t i;
		if (bHead)
		{
			i = StrT::TableFindHeadSorted(pszStr, k_pszBoolFalse, _countof(k_pszBoolFalse) - 1);
		}
		else
		{
			i = StrT::TableFindSorted(pszStr, k_pszBoolFalse, _countof(k_pszBoolFalse) - 1);
		}
		return i >= 0;
	}

	bool GRAYCALL StrA::IsPlural(const char* pszWord)
	{
		//! is the word already plural?
		//! but will typically appear as a single object. use with StrA::GetArticleAndSpace
		//! TODO THIS NEEDS WORK
		//! Similar to M$ System.Data.Entity.Design.PluralizationServices

		ASSERT(pszWord != nullptr);

		static const char* const k_Plurals[] = // These are already plural so don't need another s.
		{
			"boots",
			"cards",
			"eyes",
			"feet",
			"fish",
			"gloves",
			"hair",
			"leggings",
			"pants",
			"shoes",
			"sleeves",
			nullptr,
			// sheep,  barracks, teeth, children, people, women, men, mice, geese
		};

		ITERATE_t iRet = STR_TABLEFIND_SH(pszWord, k_Plurals);
		if (iRet >= 0)
			return true;

		const char* pStr = StrT::FindCharRev(pszWord, ' ');
		if (pStr != nullptr)
		{
			iRet = STR_TABLEFIND_SH(pStr + 1, k_Plurals);
			if (iRet >= 0)
				return true;
		}

		// end with an s or es ? StrChar::IsVowel
		return false;
	}

	const char* GRAYCALL StrA::GetArticleAndSpace(const char* pszWord)
	{
		//! What indefinite article should be used with this word? "a ", "an ", ""
		//! @note This is wrong many times.
		//!  i.e. some words need no article (plurals) : boots.

		ASSERT(pszWord != nullptr);
		if (StrA::IsPlural(pszWord))
		{
			return "";
		}
		if (StrChar::IsVowel(pszWord[0]))
		{
			return "an ";
		}
		return "a ";
	}

	INT_PTR GRAYCALL StrA::GetFixedIntRef(const char*& rpszExp, int nPlaces) // static
	{
		//!  Get a fixed point number from a string. (n nPlaces).
		//!  Used for money CY ?
		//! @arg
		//!  rpszExp = get the number text from here.
		//!  nPlaces = how many decimal places to store.
		//! @return
		//!  the new value from the text.
		//! @note
		//!  Why not use atof/strtod() ? rounding NOT ALL INTS ARE WELL STORED as FLOAT !

		if (rpszExp == nullptr)
			return 0;

		INT_PTR lVal = StrT::toIP(rpszExp, &rpszExp);
		int  iDecimalFound = 0;

		while (!iDecimalFound || nPlaces>0)
		{
			char ch = *rpszExp;

			if (iDecimalFound == 0 && ch == '.')
			{
				iDecimalFound = 1;
				rpszExp++; // AnsiNext( rpszExp );      // Get the next.
				continue;
			}

			lVal *= 10;	// move up for the decimal place.

			if (!StrChar::IsDigit(ch))         // end found.
			{
				iDecimalFound = 2;
				if (!nPlaces)
					break;          // May have no places.
			}
			else if (iDecimalFound < 2)
			{
				lVal += StrChar::Dec2U(ch);
				rpszExp++; // AnsiNext( rpszExp );      // Get the next.
			}

			if (iDecimalFound>0)
				nPlaces--;
		}

		return lVal;
	}

	//***********************************************************

	StrLen_t GRAYCALL StrA::MakeNamedBitmask(char* pszOut, StrLen_t iLenOutMax, UINT dwFlags, const char** ppszNames, ITERATE_t iMaxNames, char chSep)
	{
		//! For each bit set in dwFlags, copy a ppszNames[bit number] string to the pszOut. separated by chSep (|)
		//! @return string length

		bool bMath = (chSep == '\0');
		if (bMath)
			chSep = '|';

		ITERATE_t iCount = 0;
		StrLen_t iLen = 0;
		for (ITERATE_t i = 0; dwFlags != 0 && iLen < iLenOutMax; i++, dwFlags >>= 1)	// walk the bits.
		{
			if (!(dwFlags & 1))
				continue;
			if (iCount>0)
			{
				pszOut[iLen++] = chSep;
			}
			if (i >= iMaxNames)
			{
				// No more names so just use numbers. Ideally this would not be called.
				iLen += StrT::UtoA((UINT)_1BITMASK(i), pszOut + iLen, iLenOutMax - iLen, 0x10);
			}
			else
			{
				iLen += StrT::CopyLen(pszOut + iLen, ppszNames[i], iLenOutMax - iLen);
			}
			iCount++;
		}

		if (iCount == 0 && bMath)
		{
			// No bits were set.
			iLen = StrT::CopyLen(pszOut, "0", iLenOutMax);
		}

		return iLen;
	}
}
