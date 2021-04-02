//
//! @file StrFormat.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_StrFormat_H
#define _INC_StrFormat_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"
#include "StrBuilder.h"
#include "cDebugAssert.h"
#include "IUnknown.h"		// DECLARE_INTERFACE

namespace Gray
{
	DECLARE_INTERFACE(IIniBaseGetter);		// cIniBase.h
	typedef char IniChar_t;		//!< char format even on UNICODE system! Screw M$, INI files should ALWAYS have UTF8 contents

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
		static inline char FindSpec(char ch) noexcept
		{
			//! Find a valid spec char.
			for (size_t i = 0; i < _countof(k_Specs) - 1; i++)
			{
				const char chCur = k_Specs[i];
				if (ch > chCur)	// keep looking.
					continue;
				if (ch < chCur)	// they are sorted.
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
		typedef StrLen_t(_cdecl* STRFORMAT_t)(TYPE* pszOut, StrLen_t iLenOutMax, const TYPE* pszFormat, ...);	// signature for testing.

	public:
		StrLen_t ParseParam(const TYPE* pszFormat);

		void RenderString(StrBuilder<TYPE>& out, const TYPE* pszParam, StrLen_t nParamLen, short nPrecision) const;
		void RenderUInt(StrBuilder<TYPE>& out, const TYPE* pszPrefix, RADIX_t nRadix, char chRadixA, UINT64 uVal) const;
		void RenderFloat(StrBuilder<TYPE>& out, char chRadixA, double dVal) const;

		void RenderParam(StrBuilder<TYPE>& out, va_list* pvlist) const;

		static void GRAYCALL V(StrBuilder<TYPE>& out, const TYPE* pszFormat, va_list vlist);
		static inline StrLen_t GRAYCALL V(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszFormat, va_list vlist);

		static inline void _cdecl F(StrBuilder<TYPE>& out, const TYPE* pszFormat, ...);
		static inline StrLen_t _cdecl F(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszFormat, ...);
	};

	class GRAYCORE_LINK StrTemplate
	{
		//! @class Gray::StrTemplate
		//! strings may contain template blocks to be replaced. <?block?>
		//! similar to expressions.
	public:
		static bool GRAYCALL HasTemplateBlock(const IniChar_t* pszInp);
		static StrLen_t GRAYCALL ReplaceTemplateBlock(StrBuilder<IniChar_t>& out, const IniChar_t* pszInp, const IIniBaseGetter* pBlockReq, bool bRecursing = false);
	};

	template< typename TYPE>
	inline StrLen_t GRAYCALL StrFormat<TYPE>::V(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszFormat, va_list vlist)	// static
	{
		StrBuilder<TYPE> out(pszOut, nLenOutMax);
		V(OUT out, pszFormat, vlist);
		return out.get_Length();
	}

	template< typename TYPE>
	inline void _cdecl StrFormat<TYPE>::F(StrBuilder<TYPE>& out, const TYPE* pszFormat, ...)	// static
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		V(out, pszFormat, vargs);
		va_end(vargs);
	}

	template< typename TYPE>
	inline StrLen_t _cdecl StrFormat<TYPE>::F(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszFormat, ...)	// static
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		const StrLen_t nLenOut = V(pszOut, nLenOutMax, pszFormat, vargs);
		va_end(vargs);
		return nLenOut;
	}
}

#endif
