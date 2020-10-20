//
//! @file StrA.CPP
//! String global search functions. Const strings
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrA.h"
#include "StrChar.h"
#include "CLogMgr.h"
#include "CIniBase.h"	// IIniBaseGetter
#include "CHeap.h"
#include "CBits.h"

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
		return (i >= 0);
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
		return (i >= 0);
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
			return("an ");
		}
		return("a ");
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
			return(0);

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
		//! For each bit set in dwFlags, copy a ppszNames[bit number] string to the pszOut. separated by |
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

	StrLen_t GRAYCALL StrA::BlockReplacement(char* pszOut, StrLen_t iOutLenMax, const char* pszInp, IIniBaseGetter* pBlockReq, bool bRecursing)
	{
		//! Replace strings in a marked/delimited block using results from pBlockReq
		//! Used for <?X?> replacement in scripts.
		//! e.g. SPEAK "hello there <?SRC.NAME?> my name is <?NAME?>"
		//! @arg
		//!  uSizeOut = only go this far in the pszOut buffer.
		//!  pBlockReq = submit text found in block for block replacement. nullptr = this is just a test for blocks.
		//! @note
		//!  Allowed to be recursive. ignore blocks inside quotes ?
		//!  Bad properties are just blank.
		//! @return
		//!  n = new length of pszOut. may not have been changed. or k_ITERATE_BAD

		bool bOpenBlock = false;
		StrLen_t iBeginBlock = 0;
		StrLen_t iOutLen = 0;
		StrLen_t i = 0;

		if (bRecursing)
		{
			ASSERT(pszInp[0] == '<' && pszInp[1] == '?');
		}

		ASSERT_N(pszOut != nullptr || iOutLenMax <= 0);
		ASSERT_N(pszInp != nullptr);
		for (; i < StrT::k_LEN_MAX; i++)
		{
			// const char* pszInp2 = &pszInp[i];
			char ch = pszInp[i];
			if (ch == '\0')
				break;

			// just copy the text.
			//
			if (iOutLen < iOutLenMax)
			{
				pszOut[iOutLen++] = ch;
			}

			if (!bOpenBlock)	// not in block
			{
				if (ch == '?' && i && pszInp[i - 1] == '<')	// found the start !
				{
					if (pBlockReq == nullptr)
						return i;
					iBeginBlock = iOutLen - 2; // point to opening <?
					bOpenBlock = true;
					continue;
				}
				continue;
			}

			ASSERT(bOpenBlock);
			ASSERT(i > 0);

			if (ch == '<' && pszInp[i + 1] == '?')	// found recursive start block.
			{
				iOutLen--;
				StrLen_t iLen = StrA::BlockReplacement(pszOut + iOutLen, iOutLenMax - iOutLen, pszInp + i, pBlockReq, true);
				StrLen_t iLenOutTmp = StrT::Len(pszOut + iOutLen);
				iOutLen += iLenOutTmp;
				i += iLen - 1;
				continue;
			}

			if (ch == '>' && pszInp[i - 1] == '?') // found end of block.
			{
				// NOTE: take the tag from output side in case it is the product of recursive blocks.
				bOpenBlock = false;
				StrLen_t iTagLen = (iOutLen - iBeginBlock) - 4;
				cStringA sTag(pszOut + iBeginBlock + 2, iTagLen);
				StrLen_t iOutMax = iOutLenMax - iBeginBlock;
				HRESULT hRes;
				if (pBlockReq != nullptr)
				{
					CStringI sVal;
					hRes = pBlockReq->PropGet(sTag, sVal);
					if (SUCCEEDED(hRes))
					{
						hRes = StrT::CopyLen<IniChar_t>(pszOut + iBeginBlock, sVal, iOutMax);
					}
				}
				else
				{
					hRes = HRESULT_WIN32_C(ERROR_READ_FAULT);
				}
				if (FAILED(hRes))
				{
					// Just in case this really is a >= operator ?
					DEBUG_ERR(("Parse '%s' ERR='%s'", LOGSTR(sTag), LOGERR(hRes)));
				}
				else
				{
					iOutLen = iBeginBlock + hRes;
				}
				if (bRecursing)
				{
					i++;
					break; // end of recurse block.
				}
			}
		}

		if (pBlockReq == nullptr)
		{
			return k_ITERATE_BAD;
		}
		if (bRecursing)
		{
			return i;
		}
		pszOut[iOutLen] = '\0';
		return(iOutLen);
	}
}

//*************************************************************************
#if USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(StrA)
{
	UNITTEST_METHOD(StrA)
	{
		UNITTEST_TRUE(StrA::IsBoolTrue("true"));
		UNITTEST_TRUE(!StrA::IsBoolTrue("sdf"));
		UNITTEST_TRUE(StrA::IsBoolTrue("1"));
		UNITTEST_TRUE(!StrA::IsBoolTrue("0"));
		UNITTEST_TRUE(StrA::IsBoolTrue("True"));
		UNITTEST_TRUE(!StrA::IsBoolFalse("true"));
		UNITTEST_TRUE(!StrA::IsBoolTrue("false"));
		UNITTEST_TRUE(StrA::IsBoolFalse("false"));

		UNITTEST_TRUE(StrA::IsPlural("hair"));
		UNITTEST_TRUE(!StrA::IsPlural("foot"));
		UNITTEST_TRUE(StrA::IsPlural("feet"));

		// StrA::MakeNamedBitmask
		static const char* k_aszNames[] =
		{
			"0", "1", "2", "3",
			"4", "5", "6", "7",
			"8", "9", "10", "11",
		};
		char szTmp1[_MAX_PATH];
		StrLen_t iRet2 = StrA::MakeNamedBitmask(szTmp1, STRMAX(szTmp1), 0x123, k_aszNames, _countof(k_aszNames));
		UNITTEST_TRUE(iRet2 >= 7);
		// "0,1,5,8"

		// StrA::BlockReplacement
		class CUnitTestBlock : public Gray::IIniBaseGetter
		{
		public:
			virtual HRESULT PropGet(const char* pszPropTag, OUT CStringI& rsVal) const
			{
				if (!Gray::StrT::CmpI(pszPropTag, "blocks"))
				{
					rsVal = "TESTBLOCK";
					return S_OK;
				}
				if (!Gray::StrT::CmpI(pszPropTag, "blo1"))
				{
					rsVal = "blo";
					return S_OK;
				}
				if (!Gray::StrT::CmpI(pszPropTag, "cks2"))
				{
					rsVal = "cks";
					return S_OK;
				}
				return HRESULT_WIN32_C(ERROR_UNKNOWN_PROPERTY);
			}
		} testBlock;

		char szTmp2[StrT::k_LEN_MAX];
		StrLen_t iRet3 = StrA::BlockReplacement(szTmp2, STRMAX(szTmp2),
			"Test with recursive <?<?blo1?><?cks2?>?>. and junk <?IntentionalUnknownProperty?>.",
			&testBlock, false);
		UNITTEST_TRUE(iRet3 == 71);

		StrLen_t iRet1 = StrA::BlockReplacement(szTmp1, STRMAX(szTmp1),
			"Test input string with <?blocks?>. And another <?blocks?>.",
			&testBlock, false);
		UNITTEST_TRUE(iRet1 == 56);
	}
};
UNITTEST_REGISTER(StrA, UNITTEST_LEVEL_Core);

#endif
