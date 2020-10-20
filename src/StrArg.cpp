//
//! @file StrArg.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrArg.h"
#include "StrU.h"
#include "StrT.h"
#include "CTempPool.h"
#include "CDebugAssert.h"

namespace Gray
{
	template<> GRAYCORE_LINK const wchar_t* GRAYCALL StrArg<wchar_t>(const char* pszStrInp) // static
	{
		//! Get a temporary string that only lives long enough to satisfy a sprintf() argument.
		//! @note the UNICODE size is variable and <= Len(pszStr)
		if (pszStrInp == nullptr)
			return __TOW("NULL");
		StrLen_t iLenOut = StrU::UTF8toUNICODELen(pszStrInp); // needed UNICODE size is variable and <= Len(pszStr).
		wchar_t* pszTmp = CTempPool::GetTempST<wchar_t>(iLenOut);	//
		StrU::UTF8toUNICODE(pszTmp, iLenOut + 1, pszStrInp, k_StrLen_UNK);	// true size is variable and < iLen
		return pszTmp;
	}
	template<> GRAYCORE_LINK const char* GRAYCALL StrArg<char>(const wchar_t* pwStrInp) // static
	{
		//! Get a temporary string that only lives long enough to satisfy a sprintf() argument.
		//! @note the UTF8 size is variable and >= Len(pwStr)
		if (pwStrInp == nullptr)
			return __TOA("NULL");
		StrLen_t iLenOut = StrU::UNICODEtoUTF8Size(pwStrInp);	// needed UTF8 size is variable and >= Len(pwStr)!
		char* pszTmp = CTempPool::GetTempST<char>(iLenOut);
		StrU::UNICODEtoUTF8(pszTmp, iLenOut + 1, pwStrInp, iLenOut);
		return pszTmp;
	}

	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(char ch, StrLen_t nRepeat)
	{
		//! Get a temporary string that is nRepeat chars repeating
		TYPE* pszTmp = CTempPool::GetTempST<TYPE>(nRepeat);
		CValArray::FillQty<TYPE>(pszTmp, nRepeat, (TYPE)ch);
		pszTmp[nRepeat] = '\0';
		return pszTmp;
	}
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(wchar_t ch, StrLen_t nRepeat)
	{
		//! Get a temporary string that is nRepeat chars repeating
		TYPE* pszTmp = CTempPool::GetTempST<TYPE>(nRepeat);
		CValArray::FillQty<TYPE>(pszTmp, nRepeat, (TYPE)ch);
		pszTmp[nRepeat] = '\0';
		return pszTmp;
	}

	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(INT32 iVal)
	{
		//! Get a temporary string that only lives long enough to satisfy the sprintf()
		//! Assume auto convert char, short to int/INT32.
		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 1];
		StrLen_t nLen = StrT::ItoA(iVal, szTmp, STRMAX(szTmp), 10);
		return CTempPool::GetTempST<TYPE>(nLen, szTmp);
	}
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(UINT32 uVal, RADIX_t uRadix)
	{
		//! Get a temporary string that only lives long enough to satisfy the sprintf()
		//! Assume auto convert BYTE, WORD to UINT/UINT32/DWORD.
		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 1];
		StrLen_t nLen = StrT::UtoA(uVal, szTmp, STRMAX(szTmp), uRadix);
		return CTempPool::GetTempST<TYPE>(nLen, szTmp);
	}

#ifdef USE_INT64
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(INT64 iVal)
	{
		//! Get a temporary string that only lives long enough to satisfy the sprintf()
		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 1];
		StrLen_t nLen = StrT::ILtoA(iVal, szTmp, STRMAX(szTmp), 10);
		return CTempPool::GetTempST<TYPE>(nLen, szTmp);
	}
	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(UINT64 uVal, RADIX_t uRadix)
	{
		//! Get a temporary string that only lives long enough to satisfy the sprintf()
		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 1];
		StrLen_t nLen = StrT::ULtoA(uVal, szTmp, STRMAX(szTmp), uRadix);
		return CTempPool::GetTempST<TYPE>(nLen, szTmp);
	}
#endif

	template< typename TYPE>
	GRAYCORE_LINK const TYPE* GRAYCALL StrArg(double dVal)
	{
		//! Get a temporary string that only lives long enough to satisfy the sprintf()
		//! assume float gets converted to double.
		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS+1];
		StrLen_t nLen = StrT::DtoA(dVal, szTmp, STRMAX(szTmp));
		return CTempPool::GetTempST<TYPE>(nLen, szTmp);
	}

	template< typename TYPE>
	GRAYCORE_LINK bool GRAYCALL StrArg_UnitTestT()
	{
		//! Include this code event if ! USE_UNITTESTS to force DLL implementation.
		const TYPE* pszStr1 = StrArg<TYPE>('a', (StrLen_t)10);
		if(StrT::Cmp<TYPE>(pszStr1, CSTRCONST("aaaaaaaaaa")) != 0)
			{ ASSERT(0); return false; }

		INT32 iVal32 = 0x12;
		const TYPE* pszStr2 = StrArg<TYPE>(iVal32);
		if(StrT::Cmp<TYPE>(pszStr2, CSTRCONST("18")) != 0)
		{ ASSERT(0); return false; }

		UINT32 uVal32 = 0x12;
		const TYPE* pszStr3 = StrArg<TYPE>(uVal32, (RADIX_t)0x10);
		if(StrT::Cmp<TYPE>(pszStr3, CSTRCONST("012")) != 0)
		{ ASSERT(0); return false; }

		INT64 iVal64 = 0x12;
		const TYPE* pszStr4 = StrArg<TYPE>(iVal64);
		if(StrT::Cmp<TYPE>(pszStr4, CSTRCONST("18")) != 0)
		{ ASSERT(0); return false; }

		UINT64 uVal64 = 0x12;
		const TYPE* pszStr5 = StrArg<TYPE>(uVal64, (RADIX_t)0x10);
		if(StrT::Cmp<TYPE>(pszStr5, CSTRCONST("012")) != 0)
		{ ASSERT(0); return false; }

		double dVal = 123.123;
		const TYPE* pszStr6 = StrArg<TYPE>(dVal);
		if(StrT::Cmp<TYPE>(pszStr6, CSTRCONST("123.123")) != 0)
		{ ASSERT(0); return false; }

		return true;
	}
};

//***********************************************************
#if USE_UNITTESTS
#include "CUnitTest.h"
UNITTEST_CLASS(StrArg)
{
	UNITTEST_METHOD(StrArg)
	{
		StrArg_UnitTestT<char>();
		StrArg_UnitTestT<wchar_t>();
	}
};
UNITTEST_REGISTER(StrArg, UNITTEST_LEVEL_Core);
#else
namespace Gray
{
	void StrArg_ForceInstantiate()
	{
		//! @note In a static library, there is no good way to force a template function to instantiate. other than calling it!
		//! DLL will just use "template struct GRAYCORE_LINK CArrayString<char>;" declaration.
		StrArg_UnitTestT<char>();
		StrArg_UnitTestT<wchar_t>();
	}
};
#endif // USE_UNITTESTS
