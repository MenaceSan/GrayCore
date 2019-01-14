//
//! @file StrNum.h
//! convert numbers to/from string.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_StrNum_H
#define _INC_StrNum_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"
#include "CUnitTestDecl.h"

UNITTEST_PREDEF(StrNum)

namespace Gray
{
	struct GRAYCORE_LINK StrNum	// static
	{
		//! @struct Gray::StrNum
		//! Convert ASCII (8 bit) string to/from numbers.
		//! Numbers are in a restricted set of ASCII (8 bit) characters.
		//! It doesn't make sense to have UTF8 and UNICODE versions of these. All actions are in 'char' type.
		//! Just convert to/from UNICODE as needed using StrU::UTF8toUNICODE and GetNumberString.

		static const StrLen_t k_LEN_MAX_DIGITS = (309 + 40);	//!< Largest number we can represent in double format + some extra places for post decimal. (). like _CVTBUFSIZE or StrT::k_LEN_MAX_KEY
		static const StrLen_t k_LEN_MAX_DIGITS_INT = 64;	//!< Largest 64 bits base 2 not including sign or '\0' is only 64 digits.

		static StrLen_t GRAYCALL GetTrimCharsLen(const char* pStr, StrLen_t nLen, char ch);
		static StrLen_t GRAYCALL GetNumberString(OUT char* pszOut, const wchar_t* pszInp, StrLen_t iStrMax = k_LEN_MAX_DIGITS);

		static UINT64 GRAYCALL toUL(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr, RADIX_t nBaseRadix = 0);
		static INT64 GRAYCALL toIL(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr, RADIX_t nBaseRadix = 10);

		static UINT32 GRAYCALL toU(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr, RADIX_t nBaseRadix = 0)
		{
			//! Just cast down from 64.
			return (UINT32)toUL(pszInp, ppszInpEnd, nBaseRadix);
		}
		static INT32 GRAYCALL toI(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr, RADIX_t nBaseRadix = 10)
		{
			//! Just cast down from 64.
			return (INT32)toIL(pszInp, ppszInpEnd, nBaseRadix);
		}

		static char* GRAYCALL ULtoA2(UINT64 uVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10, char chRadixA = 'A');
		static StrLen_t GRAYCALL ULtoA(UINT64 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10);
		static StrLen_t GRAYCALL ILtoA(INT64 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10);

		static StrLen_t GRAYCALL UtoA(UINT32 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10)
		{
			//! Just cast up to 64.
			return ULtoA(nVal, pszOut, iStrMax, nBaseRadix);
		}
		static StrLen_t GRAYCALL ItoA(INT32 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix = 10)
		{
			//! Just cast up to 64.
			return ILtoA(nVal, pszOut, iStrMax, nBaseRadix);
		}

		static double GRAYCALL toDouble(const char* pszInp, const char** ppszInpEnd = (const char**) nullptr);
		static StrLen_t GRAYCALL DtoAG2(double dVal, OUT char* pszOut, int iDecPlacesWanted = -1, char chE = -'e');
		static StrLen_t GRAYCALL DtoAG(double dVal, OUT char* pszOut, StrLen_t iStrMax, int iDecPlacesWanted = -1, char chE = -'e');

#ifdef USE_UNITTESTS
		static StrLen_t GRAYCALL DToATestLegacy(double dVal, OUT char* pszOut, StrLen_t iStrMax, StrLen_t iDecPlaces);
#endif

		template < typename _TYPE >
		static _TYPE inline toValue(const char* pszInp, const char** ppszInpEnd = (const char**)nullptr);
		template < typename _TYPE >
		static StrLen_t inline ValueToA(_TYPE val, OUT char* pszOut, StrLen_t iStrMax);

		template < typename _TYPE >
		static size_t GRAYCALL ToValArray(OUT _TYPE* pOut, size_t iQtyMax, const char* pszInp);

		template < typename _TYPE >
		static StrLen_t GRAYCALL ValArrayToA(OUT char* pszOut, StrLen_t nDstMax, const _TYPE* pSrc, size_t nSrcSize);

		template < typename _TYPE >
		static StrLen_t _cdecl ValArrayToAF(OUT char* pszDst, StrLen_t iSizeDstMax, size_t nSrcQty, ...);

		UNITTEST_FRIEND(StrNum);
	};

	template<> inline INT32 StrNum::toValue<INT32>(const char* pszInp, const char** ppszInpEnd)
	{
		return (INT32)StrNum::toIL(pszInp, ppszInpEnd);
	}
	template<> inline UINT32 StrNum::toValue<UINT32>(const char* pszInp, const char** ppszInpEnd)
	{
		return (UINT32)StrNum::toUL(pszInp, ppszInpEnd);
	}
	template<> inline INT64 StrNum::toValue<INT64>(const char* pszInp, const char** ppszInpEnd)
	{
		return StrNum::toIL(pszInp, ppszInpEnd);
	}
	template<> inline UINT64 StrNum::toValue<UINT64>(const char* pszInp, const char** ppszInpEnd)
	{
		return StrNum::toUL(pszInp, ppszInpEnd);
	}
	template<> inline float StrNum::toValue<float>(const char* pszInp, const char** ppszInpEnd)
	{
		return (float)StrNum::toDouble(pszInp, ppszInpEnd);
	}
	template<> inline double StrNum::toValue<double>(const char* pszInp, const char** ppszInpEnd)
	{
		return StrNum::toDouble(pszInp, ppszInpEnd);
	}

	template<> inline StrLen_t StrNum::ValueToA<INT32>(INT32 val, OUT char* pszOut, StrLen_t iStrMax)
	{
		return StrNum::ILtoA(val, pszOut, iStrMax);
	}
	template<> inline StrLen_t StrNum::ValueToA<UINT32>(UINT32 val, OUT char* pszOut, StrLen_t iStrMax)
	{
		return StrNum::ULtoA(val, pszOut, iStrMax);
	}
	template<> inline StrLen_t StrNum::ValueToA<INT64>(INT64 val, OUT char* pszOut, StrLen_t iStrMax)
	{
		return StrNum::ILtoA(val, pszOut, iStrMax);
	}
	template<> inline StrLen_t StrNum::ValueToA<UINT64>(UINT64 val, OUT char* pszOut, StrLen_t iStrMax)
	{
		return StrNum::ULtoA(val, pszOut, iStrMax);
	}
	template<> inline StrLen_t StrNum::ValueToA<float>(float val, OUT char* pszOut, StrLen_t iStrMax)
	{
		return StrNum::DtoAG(val, pszOut, iStrMax);
	}
	template<> inline StrLen_t StrNum::ValueToA<double>(double val, OUT char* pszOut, StrLen_t iStrMax)
	{
		return StrNum::DtoAG(val, pszOut, iStrMax);
	}

	template < typename _TYPE >
	size_t GRAYCALL StrNum::ToValArray(OUT _TYPE* pOut, size_t iQtyMax, const char* pszInp) // static
	{
		//! @todo Merge with CMem::ReadFromString
		//! Similar to StrT::ParseCmds()

		if (pszInp == nullptr)
			return 0;
		size_t i = 0;
		for (; i < iQtyMax;)
		{
			for (; StrChar::IsSpace(*pszInp); pszInp++)
			{
			}
			const char* pszInpStart = pszInp;
			if (*pszInpStart == '\0')
				break;
			pOut[i++] = StrNum::toValue<_TYPE>(pszInpStart, &pszInp);
			if (pszInpStart == pszInp)	// must be the field terminator? ")},;". End.
				break;
			for (; StrChar::IsSpace(*pszInp); pszInp++)
			{
			}
			if (pszInp[0] != ',')
				break;
			pszInp++;
		}
		return i;
	}

	template < typename _TYPE >
	StrLen_t GRAYCALL StrNum::ValArrayToA(OUT char* pszDst, StrLen_t iSizeDstMax, const _TYPE* pSrc, size_t nSrcQty) // static
	{
		//! @todo Merge with CMem::ConvertToString
		//! Write bytes out to a string as comma separated base 10 number values. 
		//! Try to use SetHexDigest() instead.
		//! opposite of CMem::ReadFromString().
		//! @return the actual size of the string.
		//! @note using Base64 would be better.

		iSizeDstMax -= 4;	// room to terminate < max sized number.
		StrLen_t iLenOut = 0;
		for (size_t i = 0; i < nSrcQty; i++)
		{
			if (i > 0)
			{
				pszDst[iLenOut++] = ',';
			}

			StrLen_t iLenThis = StrNum::ValueToA<_TYPE>(pSrc[i], pszDst + iLenOut, iSizeDstMax - iLenOut);
			if (iLenThis <= 0)
				break;
			iLenOut += iLenThis;
			if (iLenOut >= iSizeDstMax)
				break;
		}
		return iLenOut;
	}

	template < typename _TYPE >
	StrLen_t _cdecl StrNum::ValArrayToAF(OUT char* pszDst, StrLen_t iSizeDstMax, size_t nSrcQty, ...) // static
	{
		//! @todo ValArrayToAF
		//
		return -1;
	}
}

#endif
