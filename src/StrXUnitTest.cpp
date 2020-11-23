//
//! @file StrXUnitTest.cpp
//! String global functions as a template.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#include "pch.h"
#include "StrT.h"
#include "StrU.h"
#include "StrChar.h"
#include "StrFormat.h"
#include "StrConst.h"
#include "cLogMgr.h"
#include "cDebugAssert.h"
#include "cUnitTest.h"

namespace Gray
{
 
	template< typename TYPE>
	void GRAYCALL StrX<TYPE>::UnitTestT() // static
	{
		//! @note In a static lib, there is no good way to force a template function to instantiate. other than calling it!

		static const cStrConst k_t1 = CSTRCONST("sdfsdf1");
		static const cStrConst k_t2 = CSTRCONST("sdfsdF23 5");	// lower case = higher number ASCII.
		TYPE szTmp[StrT::k_LEN_MAX];

		STATIC_ASSERT('\n' == 0x0a, Check_NL);	// StrChar::k_NL
		STATIC_ASSERT('\r' == 0x0d, Check_CR);	// StrChar::k_CR  

		StrLen_t nLen = Len<TYPE>(k_t1);
		UNITTEST_TRUE(nLen == 7);

		COMPARE_t eComp = Cmp<TYPE>(k_t1, k_t2);
		UNITTEST_TRUE(eComp > 0);

		eComp = CmpI<TYPE>(k_t1, k_t2);
		UNITTEST_TRUE(eComp < 0);

		eComp = CmpN<TYPE>(k_t1, k_t2, 16);
		UNITTEST_TRUE(eComp > 0);

		eComp = CmpIN<TYPE>(k_t1, k_t2, 16);
		UNITTEST_TRUE(eComp < 0);

		bool bCmp = StartsWithI<TYPE>(k_t2, k_t1);
		UNITTEST_TRUE(!bCmp);

		bCmp = EndsWithI<TYPE>(k_t2, k_t1);
		UNITTEST_TRUE(!bCmp);

		nLen = CopyLen<TYPE>(szTmp, k_t1, 16);
		UNITTEST_TRUE(nLen == 7);

		HASHCODE32_t nHashCode = GetHashCode32<TYPE>(k_t1, k_StrLen_UNK, 0);
		UNITTEST_TRUE(nHashCode == 0x1488c5b4);

		const TYPE* pRetChar = FindChar<TYPE>(k_t1, (TYPE) 'f');
		UNITTEST_TRUE(pRetChar != nullptr && *pRetChar == 'f');

		nLen = FindCharN<TYPE>(k_t1, (TYPE) 'f');
		UNITTEST_TRUE(nLen == 2);

		pRetChar = FindCharRev<TYPE>(k_t1, (TYPE) 'f');
		UNITTEST_TRUE(pRetChar != nullptr && *pRetChar == 'f');	// f1

		static cStrConst k_tSent = CSTRCONST("This is a sentence. And another. // comment");

		nLen = FindWord<TYPE>(k_tSent, CSTRCONST("sentence"));
		UNITTEST_TRUE(nLen == 18);

		pRetChar = FindTokens<TYPE>(k_tSent, k_t2);
		UNITTEST_TRUE(pRetChar != nullptr);

		pRetChar = GetNonWhitespace<TYPE>(k_tSent);
		UNITTEST_TRUE(pRetChar != nullptr);

		//***************************

		const TYPE* pszTest = CSTRCONST("abcdefabcdefg");
		const TYPE* pszRet = StrT::FindStr<TYPE>(pszTest, CSTRCONST("abcdefg"));
		UNITTEST_TRUE(pszRet == pszTest + 6);

		pszTest = CSTRCONST("abcabcabcabc");
		pszRet = StrT::FindStr<TYPE>(pszTest, CSTRCONST("abca"));
		UNITTEST_TRUE(pszRet == pszTest + 0);

		pszRet = StrT::FindStrI<TYPE>(pszTest, CSTRCONST("AbCa"));
		UNITTEST_TRUE(pszRet == pszTest + 0);

		//***************************
		// Test Int

		UINT64 ulVal = StrT::toUL<TYPE>(CSTRCONST("0xFFFFFFFF"), nullptr, 8);
		UNITTEST_TRUE(ulVal == 0xFFFFFFFF);
		ulVal = StrT::toUL<TYPE>(CSTRCONST("0xFFFFFFFF"), nullptr);
		UNITTEST_TRUE(ulVal == 0xFFFFFFFF);

		ulVal = StrT::toUL<TYPE>(CSTRCONST("FFFFFFFF"), nullptr);
		UNITTEST_TRUE(ulVal == 0);

		ulVal = StrT::toUL<TYPE>(CSTRCONST("FFFFFFFF"), nullptr, 16);
		UNITTEST_TRUE(ulVal == 0xFFFFFFFF);

		INT64 iVal64 = StrT::toIL<TYPE>(CSTRCONST("1234567"), nullptr, 10);
		UNITTEST_TRUE(iVal64 == 1234567);

		int iVal = StrT::toI<TYPE>(CSTRCONST("-123"));
		UNITTEST_TRUE(iVal == -123);

		iVal = StrT::toI<TYPE>(CSTRCONST("123"));
		UNITTEST_TRUE(iVal == 123);

		iVal = StrT::toI<TYPE>(CSTRCONST("0x123"));
		UNITTEST_TRUE(iVal == 0x123);

		StrLen_t nLenRet = StrT::ILtoA<TYPE>(123123, szTmp, STRMAX(szTmp), 10);
		UNITTEST_TRUE(!StrT::Cmp<TYPE>(szTmp, CSTRCONST("123123")));

		nLenRet = StrT::ULtoAK<TYPE>(123123, szTmp, STRMAX(szTmp), 1024, true);
		UNITTEST_TRUE(nLenRet == 8);

		//*****************************************
		// Test float,double.  Test MUST be reversible.

		double dVal = StrT::toDouble<TYPE>(CSTRCONST("123.123"), nullptr);
		UNITTEST_TRUE(dVal == 123.123);	// NOT 123.12299999

		nLenRet = StrT::DtoA<TYPE>(dVal, szTmp, STRMAX(szTmp));
		UNITTEST_TRUE(!StrT::Cmp<TYPE>(szTmp, CSTRCONST("123.123")));

		//**********************

		const TYPE* pszText = CSTRCONST("a.b.c");
		StrLen_t nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("*.c"), false);
		UNITTEST_TRUE(nLenMatch == 5);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("*.c.d"), false);
		UNITTEST_TRUE(nLenMatch == 0);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("*.d"), false);
		UNITTEST_TRUE(nLenMatch == 0);

		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("*.b.*"), false);
		UNITTEST_TRUE(nLenMatch == 5);

		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("a*b"), false, 5);
		UNITTEST_TRUE(nLenMatch == 3);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("A*B"), true, 5);
		UNITTEST_TRUE(nLenMatch == 3);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("A*B"), false, 5);
		UNITTEST_TRUE(nLenMatch == 0);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("a.b.c.d"), false, 5);
		UNITTEST_TRUE(nLenMatch == -5);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("*.b.c.d"), false, 5);
		UNITTEST_TRUE(nLenMatch == -5);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("*.d"), false, 5);
		UNITTEST_TRUE(nLenMatch == 0);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("*c.d"), false, 5);
		UNITTEST_TRUE(nLenMatch == -5);

		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("d.*"), false);
		UNITTEST_TRUE(nLenMatch == 0);
		nLenMatch = StrT::MatchRegEx<TYPE>(pszText, CSTRCONST("*.d"), false);
		UNITTEST_TRUE(nLenMatch == 0);

		// No TYPE arg.
		pszRet = GetTableElem<TYPE>(0, nullptr, 0, 0);
		UNITTEST_TRUE(pszRet != nullptr && *pszRet == '?');

		ITERATE_t iCountRet = GetTableCount<TYPE>(nullptr, 0);
		UNITTEST_TRUE(iCountRet == 0);

		iCountRet = GetTableCountSorted<TYPE>(nullptr, 0);
		UNITTEST_TRUE(iCountRet == 0);

		iCountRet = StrT::TableFind(pszText, nullptr, 0);
		UNITTEST_TRUE(iCountRet == -1);

		iCountRet = StrT::TableFindHead(pszText, nullptr, 0);
		UNITTEST_TRUE(iCountRet == -1);

		iCountRet = StrT::TableFindSorted(pszText, nullptr, 0);
		UNITTEST_TRUE(iCountRet == -1);

		iCountRet = StrT::TableFindHeadSorted(pszText, nullptr, 0);
		UNITTEST_TRUE(iCountRet == -1);

		bool bPrintable = StrT::IsPrintable(pszText, 2);
		UNITTEST_TRUE(bPrintable);
		bPrintable = StrT::IsPrintable("a\177", 2);
		UNITTEST_TRUE(!bPrintable);

		//******************************************

		nLenRet = StrT::CopyLen<TYPE>(szTmp, CSTRCONST("123 "), STRMAX(szTmp));
		UNITTEST_TRUE(nLenRet == 4);
		pRetChar = StrT::TrimWhitespace(szTmp);
		UNITTEST_TRUE(pRetChar == szTmp);
		UNITTEST_TRUE(StrT::Len(szTmp) == 3);
		pRetChar = StrT::StripBlock(szTmp);
		UNITTEST_TRUE(pRetChar != nullptr && *pRetChar == '1'); // ""

		bool bIsWhitespace = StrT::IsWhitespace<TYPE>(CSTRCONST("  \f\r\n\t "), k_StrLen_UNK);
		UNITTEST_TRUE(bIsWhitespace);
		bIsWhitespace = StrT::IsWhitespace<TYPE>(nullptr);
		UNITTEST_TRUE(bIsWhitespace);
		bIsWhitespace = StrT::IsWhitespace<TYPE>(cStrConst::k_Empty);
		UNITTEST_TRUE(bIsWhitespace);

		//**********************

		TYPE* ppCmds[128];
		iCountRet = StrT::ParseCmdsTmp<TYPE>(szTmp, STRMAX(szTmp), CSTRCONST("0"), ppCmds, _countof(ppCmds), nullptr, STRP_DEF);
		UNITTEST_TRUE(iCountRet == 1);
		UNITTEST_TRUE(ppCmds[0] == szTmp);

		const TYPE* pszCmd = CSTRCONST("0 ,1 ,2.234,3.0 ");
		iCountRet = StrT::ParseCmdsTmp<TYPE>(szTmp, STRMAX(szTmp), pszCmd, ppCmds, 3, nullptr, STRP_DEF);
		UNITTEST_TRUE(iCountRet == 3);
		UNITTEST_TRUE(ppCmds[2] == szTmp + 6);

		//***************************

		nLenRet = StrT::CopyLen<TYPE>(szTmp, CSTRCONST("this are a string"), STRMAX(szTmp)); // sic
		UNITTEST_TRUE(nLenRet);

		StrT::ReplaceX<TYPE>(szTmp, STRMAX(szTmp), 5, 3, CSTRCONST("is"));
		UNITTEST_TRUE(!StrT::Cmp<TYPE>(szTmp, CSTRCONST("this is a string")));

		//***************************
		static cStrConst k_tEsc = CSTRCONST("sd\nf\tsd\tf2\n");
		const StrLen_t iLenStr = StrT::Len<TYPE>(k_tEsc);
		UNITTEST_TRUE(iLenStr == 11);

		// @todo > 127
		TYPE szTmpEsc[127];
		for (int i = 0; i < (int)STRMAX(szTmpEsc); i++)
			szTmpEsc[i] = (TYPE)(i + 1);
		szTmpEsc[STRMAX(szTmpEsc)] = '\0';

		TYPE szTmpE1[128];
		StrLen_t iLenE = StrT::EscSeqAdd<TYPE>(szTmpE1, k_tEsc, STRMAX(szTmpE1));
		UNITTEST_TRUE(iLenE == 15);

		TYPE szTmpE2[4 * 1024];
		iLenE = StrT::EscSeqAdd<TYPE>(szTmpE2, szTmpEsc, STRMAX(szTmpE2));
		UNITTEST_TRUE(iLenE == 225); // 225 for 127, 725 for ?

		TYPE szTmpD1[STRMAX(szTmpE1)];
		StrLen_t iLenD = StrT::EscSeqRemove<TYPE>(szTmpD1, szTmpE1, STRMAX(szTmpD1));
		UNITTEST_TRUE(iLenD == iLenStr);
		UNITTEST_TRUE(!StrT::Cmp<TYPE>(szTmpD1, k_tEsc));

		static cStrConst k_Q = CSTRCONST("\"abcdefgh\"");
		const TYPE* pszQ = k_Q;
		iLenD = StrT::EscSeqRemoveQ<TYPE>(szTmpD1, pszQ, STRMAX(szTmpD1), StrT::k_LEN_MAX);
		UNITTEST_TRUE(iLenD == 10);

		iLenE = StrT::EscSeqAddQ<TYPE>(szTmpE1, k_tEsc, STRMAX(szTmpE1));	// Quoted
		UNITTEST_TRUE(iLenE == 17);

		TYPE szTmpD2[STRMAX(szTmpE2)];
		iLenD = StrT::EscSeqRemove<TYPE>(szTmpD2, szTmpE2, STRMAX(szTmpD2));
		//	UNITTEST_TRUE(iLenD == _countof(szTmpEsc));
		for (int i = 0; i < (int)STRMAX(szTmpEsc); i++)
		{
			UNITTEST_TRUE(szTmpEsc[i] == (TYPE)(i + 1));
			UNITTEST_TRUE(szTmpD2[i] == (TYPE)(i + 1));
		}
		UNITTEST_TRUE(szTmpEsc[STRMAX(szTmpEsc)] == '\0');
		UNITTEST_TRUE(szTmpD2[STRMAX(szTmpEsc)] == '\0');

		//******************************************

		UNITTEST_TRUE(StrT::Cmp<TYPE>(StrX<TYPE>::GetBoolStr(true), CSTRCONST("1")) == 0);
		UNITTEST_TRUE(StrT::Cmp<TYPE>(StrX<TYPE>::GetBoolStr(false), CSTRCONST("0")) == 0);

		// TODO FIXME
		// StrFormat<TYPE>::UnitTestFormat(StrT::sprintfN<TYPE>);	// StrT::sprintfN<TYPE>
	}

	template struct GRAYCORE_LINK StrX<char>;		// Force Instantiation for DLL
	template struct GRAYCORE_LINK StrX<wchar_t>;	// Force Instantiation for DLL.
}

