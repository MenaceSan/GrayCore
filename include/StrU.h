//
//! @file StrU.h
//! Convert to/from UTF8 and UNICODE
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrU_H
#define _INC_StrU_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"
#include "cUnitTestDecl.h"

UNITTEST_PREDEF(StrU)

namespace Gray
{
	struct GRAYCORE_LINK StrU // : public StrT // static
	{
		//! @struct Gray::StrU
		//! A bunch of functions for UNICODE strings and UTF8. Might be named StrW ? Opposite of StrA.

		static const StrLen_t k_UTF8_SIZE_MAX = 4;	//!< Max of 4 BYTEs to encode any UNICODE char.

		enum UTFLead_TYPE
		{
			//! @enum Gray::StrU::UTFLead_TYPE
			//! http://www.unicode.org/faq/utf_bom.html
			//! Invalid UTF8 sequences are used for special meaning by M$. Placed at start of text file to indicate encoding.
			//!	 ef bb bf (M$ "lead bytes")
			//!	 ef bf be
			//!	 ef bf bf
			UTFLead_0 = 0xefU,		// Might be the first part of a UTF8 sequence or a special M$ signal.
			UTFLead_1 = 0xbbU,
			UTFLead_2 = 0xbfU,
			UTFLead_X = 0xbeU,		// alternate.
		};

		static bool GRAYCALL IsUTFLead(const void* pvU);

		static StrLen_t GRAYCALL UTF8Size(wchar_t wChar, int& riStartBits);
		static StrLen_t GRAYCALL UTF8Size1(unsigned char chFirst, int& riStartBits);
		static StrLen_t GRAYCALL UTF8Size(const char* pInp, StrLen_t iSizeInpBytes, int& riStartBits);
		static StrLen_t GRAYCALL UTF8toUNICODE(wchar_t& wChar, const char* pInp, StrLen_t iSizeInpBytes);
		static StrLen_t GRAYCALL UNICODEtoUTF8(char* pOut, StrLen_t iSizeOutMaxBytes, wchar_t wChar);

		static StrLen_t GRAYCALL UTF8toUNICODELen(const char* pInp, StrLen_t iSizeInpBytes = k_StrLen_UNK);
		static StrLen_t GRAYCALL UNICODEtoUTF8Size(const wchar_t* pInp, StrLen_t iSizeInpChars = k_StrLen_UNK);

		static StrLen_t GRAYCALL UTF8toUNICODE(OUT wchar_t* pOut, StrLen_t iSizeOutMaxChars, const char* pInp, StrLen_t iSizeInpBytes = k_StrLen_UNK);
		static StrLen_t GRAYCALL UNICODEtoUTF8(OUT char* pOut, StrLen_t iSizeOutMaxBytes, const wchar_t* pInp, StrLen_t iSizeInpChars = k_StrLen_UNK);

#if defined(USE_UNITTESTS)
		static bool GRAYCALL UnitTestU(const wchar_t* pszText, StrLen_t nLen);
		static bool GRAYCALL UnitTest8(const char* pszText, StrLen_t nLen);
		UNITTEST_FRIEND(StrU);
#endif
	};
}

#endif // _INC_StrU_H
