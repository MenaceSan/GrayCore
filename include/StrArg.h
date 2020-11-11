//
//! @file StrArg.h
//! Make some argument into a string of desired format.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrArg_H
#define _INC_StrArg_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"
#include "cUnitTestDecl.h"

UNITTEST_PREDEF(StrArg)

namespace Gray
{
	// Convert some type into a string. ALA ToString()
	// Define temporary string values for use as sprintf() arguments.
	// Use cTempPool

	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(const char* pszStr);
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(const wchar_t* pszStr);

	// TODO Front padding with max X chars.

	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(char ch, StrLen_t nRepeat = 1);
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(wchar_t ch, StrLen_t nRepeat = 1);

	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(INT32 iVal);
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(UINT32 uVal, RADIX_t uRadix = 10);

#ifdef USE_INT64
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(INT64 iVal);
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(UINT64 uVal, RADIX_t uRadix = 10);
#endif

	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(double dVal);	// , int iPlaces, bool bFormat=false

	template<> inline const char* GRAYCALL StrArg<char>(const char* pszStr) // static
	{
		//! safe convert arguments for sprintf("%s") type params. ONLY if needed.
		//! for string args to _cdecl (variable ...) functions
		//! inline this because no processing is needed.
		return pszStr;
	}
	template<> inline const wchar_t* GRAYCALL StrArg<wchar_t>(const wchar_t* pszStr) // static
	{
		//! safe convert arguments for sprintf("%s") type params. ONLY if needed.
		//! for string args to _cdecl (variable ...) functions
		//! inline this because no processing is needed.
		return pszStr;
	}
};
#endif
