//
//! @file StrA.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrA_H
#define _INC_StrA_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrConst.h"
#include "Index.h"

namespace Gray
{
	struct GRAYCORE_LINK StrA // : public StrT // static
	{
		//! @struct Gray::StrA
		//! Functions on 8 bit char ANSI C strings. Opposite of StrU.

		static bool GRAYCALL IsBoolTrue(const char* pszStr, bool bHead = false);
		static bool GRAYCALL IsBoolFalse(const char* pszStr, bool bHead = false);

		static const char* GRAYCALL GetArticleAndSpace(const char* pszWords);
		static bool GRAYCALL IsPlural(const char* pszWord);
		static INT_PTR GRAYCALL GetFixedIntRef(const char*& rpszExp, int nPlaces);

		//*****************************************************************************
		// String Modifying

		static StrLen_t GRAYCALL MakeNamedBitmask(char* pszOut, StrLen_t iOutSizeMax, UINT dwFlags, const char** ppszNames, ITERATE_t iMaxNames, char chSep = '\0');

	};
} 

#endif
