//
//! @file StrFormat.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_StrFormat_H
#define _INC_StrFormat_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrT.h"

UNITTEST_PREDEF(StrFormat)

namespace Gray
{
	struct GRAYCORE_LINK StrFormatBase
	{
		//! @class Gray::StrFormatBase
		//! Stuff common for char and wchar_t StrFormat for use with printf() type functions.
		//! "%[flags][width][.precision][length]specifier"
		//! http://en.cppreference.com/w/cpp/io/c/fprintf

	public:
		static const char k_Specs[16]; // "EFGXcdefgiosux"  // Omit "S" "apnA"

		char m_nSpec;		//!< % type. 0 = invalid. from k_Specs.

		BYTE m_nWidthMin;	//!< specifies minimum field width. Total width of what we place in pszOut
		short m_nPrecision;	//!< how many chars from pszParam do we take? (not including '\0') -1 = default.

		// Flags.
		BYTE m_nLong;		//!< 0=int, 1=long (32 bit usually), or 2=long long (64 bit usually). 'l' or 'll'. (char or wchar_t?)
		bool m_bAlignLeft;	//!< - sign indicates left align.
		bool m_bPlusSign;	//!< + indicates to Show sign.
		bool m_bLeadZero;	//!< StrNum::k_LEN_MAX_DIGITS_INT
		bool m_bWidthArg;	//!< Get an argument that will supply the m_nWidth.
		bool m_bAddPrefix;	//!< Add a prefix 0x for hex or 0 for octal.

	public:
		static inline char FindSpec(char ch)
		{
			//! Find a valid spec char.
			for (size_t i = 0; i < _countof(k_Specs) - 1; i++)
			{
				char chTest = k_Specs[i];
				if (ch > chTest)	// keep looking.
					continue;
				if (ch < chTest)	// they are sorted.
					return '\0';
				return ch;
			}
			return '\0';	// nothing.
		}
	};

	template< typename TYPE = char >
	class GRAYCORE_LINK StrFormat : public StrFormatBase
	{
		//! @class Gray::StrFormat
		//! A single formatter for a string. Like printf().
		//! Hold a single printf type format parameter/specifier and render it.

	public:
		typedef StrLen_t(_cdecl *STRFORMAT_t)(TYPE* pszOut, StrLen_t iLenOutMax, const TYPE* pszFormat, ...);

	public:
		StrLen_t ParseParam(const TYPE* pszFormat);
		StrLen_t RenderString(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszParam, StrLen_t nParamLen, short nPrecision) const;
		StrLen_t RenderUInt(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszPrefix, RADIX_t nRadix, char chRadixA, UINT64 uVal) const;
		StrLen_t RenderFloat(TYPE* pszOut, StrLen_t nLenOutMax, char chRadixA, double dVal) const;

		StrLen_t RenderParam(TYPE* pszOut, StrLen_t nLenOutMax, va_list* pvlist) const;

		static StrLen_t GRAYCALL FormatV(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszFormat, va_list vlist);
		static StrLen_t _cdecl FormatF(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszFormat, ...);

#ifdef USE_UNITTESTS
		static StrLen_t GRAYCALL UnitTestFormat1(STRFORMAT_t pFormat, TYPE* pszOut, StrLen_t nLenOut, const TYPE* pszFormat, int eArgs);
		static void GRAYCALL UnitTestFormatX(STRFORMAT_t pFormat, TYPE* pszOut, StrLen_t nLenOut);
		static void GRAYCALL UnitTestFormat(STRFORMAT_t pFormat);
		UNITTEST_FRIEND(StrFormat);
#endif
	};
};

#endif
