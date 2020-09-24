//
//! @file StrNum.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrNum.h"
#include "StrT.h"
#include "CTypes.h"
#include "CFloat.h"
#include "CFloatDeco.h"
#include "CValT.h"

namespace Gray
{
	StrLen_t GRAYCALL StrNum::GetTrimCharsLen(const char* pszInp, StrLen_t nLen, char ch) // static
	{
		//! Get Length of string if all ch chars are trimmed from the end.
		for (; nLen > 0; nLen--)
		{
			if (pszInp[nLen - 1] != ch)
				break;
		}
		return nLen;
	}

	StrLen_t GRAYCALL StrNum::GetNumberString(OUT char* pszOut, const wchar_t* pszInp, StrLen_t iStrMax) // static
	{
		//! Get a string in ASCII from UNICODE that represents a number.
		//! Only get numeric type chars.
		//! Like a simple version of StrU::UNICODEtoUTF8() and Opposite of StrU::UTF8toUNICODE()
		//! @note ASSUME pszOut[k_LEN_MAX_DIGITS]

		StrLen_t nLen = 0;
		for (; nLen < iStrMax; nLen++)
		{
			// Is this a possible number?
			wchar_t ch = pszInp[nLen];
			if (ch <= ' ' || ch >= 127)	// not a number! or letter for hex.
				break;
			// if (ch == ':' || ch == ';' || ch == '(' || ch == ')')	// not a number!
			// 	break;
			// allow and characters that might be part of a number. Digit, '.', 'e','E','+', '-', 'A' - 'Z', 'a' - 'z' for hex values,
			// allow comma ?
			pszOut[nLen] = (char)ch;
		}

		pszOut[nLen] = '\0';	// Terminate.
		return nLen;
	}

	//*************************************************************************************

	UINT64 GRAYCALL StrNum::toUL(const char* pszInp, const char** ppszInpEnd, RADIX_t nBaseRadix) // static
	{
		//! Similar to strtoul(). skip leading spaces. BUT NOT newlines. We should use StrNum::GetNumberString ?
		//! May have 0x# prefix to indicate hex
		//! @arg nBaseRadix = Radix, 0 = default to 10 and allow the string to override this. '0x' prefix will override.
		//!  ppszInpEnd = the non number digit at the end . might be '\0';
		//! @note
		//!  TYPE* ppszInpEnd; return( strtol( pszInp, &ppszInpEnd, nBaseRadix )); 
		//!  > X digits is an overflow ?

		bool bFlexible = false;	// allow hex ?
		if (nBaseRadix < StrChar::k_uRadixMin)
		{
			bFlexible = true;
			nBaseRadix = 10;	// default.
		}
		ASSERT(nBaseRadix <= StrChar::k_uRadixMax);
		if (pszInp == nullptr)
			return 0;

		pszInp = StrT::GetNonWhitespace(pszInp);
		char ch = pszInp[0];
		if (ch == '0')
		{
			ch = pszInp[1];
			if (ch == 'X' || ch == 'x')
			{
				// its really just hex!
				nBaseRadix = 16;
				pszInp += 2;
			}
			else if (bFlexible)	// Is this octal or hex ?
			{
				nBaseRadix = 16;
				pszInp++;
			}
		}

		UINT64 uVal = 0;
		for (;;)
		{
			ch = *pszInp; // Char_Radix2U
			if (StrChar::IsDigit(ch))
			{
				ch = (char) StrChar::Dec2U(ch);
			}
			else
			{
				if (nBaseRadix < 10)	// not valid character for this radix.
					break;
				if (ch >= 'A' && ch <= (('A' - 11) + nBaseRadix))
				{
					ch -= 'A' - 10;
				}
				else if (ch >= 'a' && ch <= (('a' - 11) + nBaseRadix))
				{
					ch -= 'a' - 10;
				}
				else
				{
					break;	// end of useful chars. // not valid character for this radix.
				}
			}
			uVal *= nBaseRadix;
			uVal += ch;
			pszInp++;
		}

		if (ppszInpEnd != nullptr)
		{
			*ppszInpEnd = pszInp;
		}
		return uVal;
	}

	INT64 GRAYCALL StrNum::toIL(const char* pszInp, const char** ppszInpEnd, RADIX_t nBaseRadix)
	{
		//! convert string to integer value. like strtol(), or a bit like atoi()
		//! May have 0x# prefix to indicate hex
		if (pszInp == nullptr)
			return 0;
		pszInp = StrT::GetNonWhitespace(pszInp);
		if (pszInp[0] == '-')
		{
			return(-(INT64)StrNum::toUL(pszInp + 1, ppszInpEnd, nBaseRadix));
		}
		return (INT64)StrNum::toUL(pszInp, ppszInpEnd, nBaseRadix);
	}

	//*************************************************************************************

	char* GRAYCALL StrNum::ULtoA2(UINT64 uVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix, char chRadixA)
	{
		//! Internal function to format a number BACKWARDS as a string similar to sprintf("%u") padded from right.
		//! Padded from right. No lead padding.  upper case radix default.
		//! @arg iStrMax must include space for null.
		//! @return First digit (most significant)

		ASSERT(nBaseRadix <= StrChar::k_uRadixMax);
		ASSERT(iStrMax > 0);
		if (nBaseRadix < StrChar::k_uRadixMin)
			nBaseRadix = 10;
		iStrMax--;
		char* pDigits = pszOut + iStrMax;
		*pDigits = '\0';

		// ? Shortcut for nBaseRadix = 16 = no modulus

		while (pDigits > pszOut)
		{
			UINT64 d = uVal % nBaseRadix;
			*(--pDigits) = (char)(d + ((d < 10) ? '0' : (chRadixA - 10))); // StrChar::U2Radix
			uVal /= nBaseRadix;
			if (!uVal)
				break;
		}
		return pDigits;
	}

	StrLen_t GRAYCALL StrNum::ULtoA(UINT64 uVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix)
	{
		//! Format a number as a string similar to sprintf("%u")  upper case radix default.
		//! like _itoa(iValue,pszOut,iRadix), FromInteger() and RtlIntegerToUnicodeString() or _itoa_s()
		//! Leading zero on hex string. (if room)
		//! @arg
		//!  nBaseRadix = 10 default
		//!  iStrMax = _CVTBUFSIZE = _countof(Dst) = includes room for '\0'. (just like memcpy)
		//! @return
		//!  length of the string.

		char szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 2];	// bits in int is all we really need max. (i.e. nBaseRadix=2)
		char* pDigits = ULtoA2(uVal, szTmp, _countof(szTmp), nBaseRadix);
		if (nBaseRadix == 16 && uVal != 0) // give hex a leading 0 if there is room. except if its 0 value.
		{
			StrLen_t iLenInc = StrT::Diff(szTmp + _countof(szTmp), pDigits);
			if (iLenInc < iStrMax)
			{
				*(--pDigits) = '0';
			}
		}
		return StrT::CopyLen(pszOut, pDigits, iStrMax);
	}

	StrLen_t GRAYCALL StrNum::ILtoA(INT64 nVal, OUT char* pszOut, StrLen_t iStrMax, RADIX_t nBaseRadix)
	{
		//! Make a string from a number. like ltoa(). upper case radix default.
		//! @arg iStrMax = _countof(Dst) = includes room for '\0'. (just like memcpy)
		//! @return
		//!  length of the string.

		if (iStrMax <= 0)
			return 0;
		char szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 2];	// bits in int is all we really need max. (i.e. nBaseRadix=2 + sign + '\0')
		if (nVal < 0)
		{
			nVal = -nVal;
			*pszOut++ = '-';
			iStrMax--;
		}
		char* pDigits = ULtoA2((UINT64)nVal, szTmp, STRMAX(szTmp), nBaseRadix);
		return StrT::CopyLen(pszOut, pDigits, iStrMax);
	}

	//*************************************************************************************

	StrLen_t GRAYCALL StrNum::DtoAG2(double dVal, OUT char* pszOut, int iDecPlacesWanted, char chE) // static
	{
		//! Make a string from a double number. like _gcvt() or "%g"
		//! like dtoa(), gcvt(), fcvt(), ecvt(), _fcvt() and the opposite of toDouble(), 
		//! @arg
		//!  pszOut ASSUME size >= k_LEN_MAX_DIGITS
		//!  iDecPlacesWanted = count of decimal places i desire. Number of digits after the decimal point. -1 = don't care.
		//!  chE = 0=%f,<0=%g,>0=%e
		//! @return
		//!  length of the string. pszOut = 'inf' or 'NaN'
		//!  e.g. 123.449997 should equal "123.45" ?
		//! @note _WIN32 wsprintf() does NOT do floats of any sort!!!
		//!		there is no UNICODE version of fcvt()
		//! @note %g looks nice but can lose precision! Non reversible.

		// @todo implement gcvt(), fcvt(), _fcvt_s locally ? no UNICODE version of fcvt().

		// Not handling NaN and inf
		ASSERT(!CTypeFloat::IsNaN(dVal));
		ASSERT(!CTypeFloat::IsInfinite(dVal));

		if (dVal < 0)
		{
			pszOut[0] = '-';
			return 1 + DtoAG2(-dVal, pszOut + 1, iDecPlacesWanted, chE);
		}

		int nExp10; // decimal exponent
		StrLen_t nMantLength = CFloatDeco::Grisu2(dVal, pszOut, &nExp10);
		ASSERT(nMantLength > 0);

		StrLen_t nOutLen;
		if (chE < '\0')
		{
			// "%g" or "%G"
			// iDecPlacesWanted = precision = total mantissa digits (not true decimal places).
			if (iDecPlacesWanted >= 0 && iDecPlacesWanted < nMantLength)
			{
				// Chop off decimal places.
				nExp10 += nMantLength - iDecPlacesWanted;
				nMantLength = CFloatDeco::MantRound(pszOut, iDecPlacesWanted);
			}

			// Decide what format to use for the number. like gcvt() selects F or E format.
			// 26 digits. mid point = 8; +/- 13
			const StrLen_t nDecPlaceO = nMantLength + nExp10;	// 10^(nDecPlaceO-1) <= v < 10^nDecPlaceO
			if (nDecPlaceO >= -5 && nDecPlaceO <= 21)		// 26 digits. mid point = 8; +/- 13
			{
				// Prefer F format
				if (nExp10 >= 0) // Whole numbers only. No decimal places. 
					iDecPlacesWanted = 0;
				else
					iDecPlacesWanted = -1;
				nOutLen = CFloatDeco::FormatF(pszOut, nMantLength, nExp10, iDecPlacesWanted);	// iDecPlacesWanted
			}
			else
			{
				// Prefer E Format.
				nOutLen = CFloatDeco::FormatE(pszOut, nMantLength, nExp10, -chE);
			}
		}
		else if (chE != '\0')
		{
			// E format. always has 1 whole digit.
			if (iDecPlacesWanted >= 0)	// restrict decimal places.
			{
				StrLen_t iDelta = CFloatDeco::MantAdjust(pszOut, nMantLength, iDecPlacesWanted + 1);
				nMantLength += iDelta;
				nExp10 -= iDelta;
			}
			nOutLen = CFloatDeco::FormatE(pszOut, nMantLength, nExp10, chE);
		}
		else
		{
			// F format.
			if (nExp10 >= 0 && iDecPlacesWanted < 0) // Whole numbers only. No decimal places. 
				iDecPlacesWanted = 1;	// default = have at least 1 decimal place. 

			nOutLen = CFloatDeco::FormatF(pszOut, nMantLength, nExp10, iDecPlacesWanted);
		}

		ASSERT(pszOut[nOutLen] == '\0');
		return nOutLen;
	}

	StrLen_t GRAYCALL StrNum::DtoAG(double dVal, OUT char* pszOut, StrLen_t iStrMax, int iDecPlacesWanted, char chE) // static
	{
		if (iStrMax >= StrNum::k_LEN_MAX_DIGITS)
		{
			return StrNum::DtoAG2(dVal, pszOut, iDecPlacesWanted, chE);
		}
		char szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
		StrNum::DtoAG2(dVal, szTmp, iDecPlacesWanted, chE); // StrLen_t iStrLen = 
		return StrT::CopyLen(pszOut, szTmp, iStrMax);
	}

	double GRAYCALL StrNum::toDouble(const char* pszInp, const char** ppszInpEnd) // static
	{
		//! Convert a string to a double precision decimal value.
		//! MUST emulate/match the C compiler. The C++ compiler will generate a double value from a string in code.
		//! MUST be reversible using DtoA().
		//! like atof(), wcstod() or strtod() and opposite of DtoA(), dtoa()
		//! don't bother with toFloat() since it will do the same thing.
		//! e.g. decode stuff in format. "12.2", ".12", "123e234",
		//!
		//! http://www.cplusplus.com/reference/cstdlib/strtod/
		//! see http://www.exploringbinary.com/how-strtod-works-and-sometimes-doesnt/ for why this is dangerous/difficult.
		//! http://www.opensource.apple.com/source/tcl/tcl-10/tcl/compat/strtod.c
		//! It can have rounding problems? e.g. "3.1416" = 3.1415999999999999. Deal with this issue on conversion to string. 
		//! http://www.ampl.com/netlib/fp/dtoa.c (David Gay)

		if (pszInp == nullptr)
			return 0;

		// Strip off leading blanks and check for a sign.
		pszInp = StrT::GetNonWhitespace(pszInp);

		const char* pszStart = pszInp;	// Sign would be here.
		char ch = *pszInp;
		if (ch == '-' || ch == '+')
			++pszInp;

		StrLen_t nSizeMant = 0;	// Number of digits in mantissa.
		StrLen_t nSizeInt = -1;	// Number of mantissa digits BEFORE decimal point.
		for (; ; nSizeMant++, pszInp++)
		{
			ch = *pszInp;
			if (StrChar::IsDigit(ch))
				continue;
			if ((ch != '.') || (nSizeInt >= 0))		// First and only decimal point.
				break;
			nSizeInt = nSizeMant;	// Found the decimal point.
		}

		// Now suck up the digits in the mantissa. 
		// Use two integers to collect 9 digits each (this is faster than using floating-point).
		// If the mantissa has more than 18 digits, ignore the extras, since they can't affect the value anyway.

		const char* pszExp = pszInp;
		pszInp -= nSizeMant;		// back to start.
		if (nSizeInt < 0)	// had no decimal place.
			nSizeInt = nSizeMant;	// Put decimal point at the end.
		else
			nSizeMant--;			// One of the digits was the point. drop it.

		int nExpFrac;	// Exponent that derives from the fractional part
		if (nSizeMant > 18)
		{
			// Ignore unusable mantissa accuracy.
			nExpFrac = nSizeInt - 18;
			nSizeMant = 18;	// truncate
		}
		else
		{
			if (nSizeMant == 0) // No value? maybe just a decimal place. thats odd.
			{
				if (ppszInpEnd != nullptr)
				{
					*ppszInpEnd = (char *)pszStart;	// Nothing here that was a number.
				}
				return 0.0;
			}
			nExpFrac = nSizeInt - nSizeMant;
		}

		// Do math as 2 integers for speed. like GetFixedIntRef()
		UINT32 fracHi = 0;
		for (; nSizeMant > 9; )
		{
			ch = *pszInp;
			pszInp++;
			if (ch != '.')	// Just skip .
			{
				fracHi = 10 * fracHi + StrChar::Dec2U(ch);
				nSizeMant--;
			}
		}

		UINT32 fracLo = 0;
		for (; nSizeMant > 0; )
		{
			ch = *pszInp;
			pszInp++;
			if (ch != '.')	// Just skip .
			{
				fracLo = 10 * fracLo + StrChar::Dec2U(ch);
				nSizeMant--;
			}
		}

		// Skim off the exponent.
		pszInp = pszExp;	// nSizeMant may have been truncated to 18.
		ch = *pszInp;
		bool bExpNegative = false;
		int nExp = 0;		// Exponent read from "e" field. 
		if ((ch == 'E') || (ch == 'e'))
		{
			pszInp++;
			ch = *pszInp;
			if (ch == '-')
			{
				bExpNegative = true;
				pszInp++;
			}
			else if (ch == '+')
			{
				pszInp++;
			}

			const char* pszExp2 = pszInp;
			while (StrChar::IsDigit(*pszInp))
			{
				nExp = nExp * 10 + StrChar::Dec2U(*pszInp);
				pszInp++;
			}
			if (pszInp == pszExp2)
			{
				// e NOT followed by a valid number. Ignore it.
				ASSERT(nExp == 0);
				pszInp = pszExp;
			}
		}

		if (ppszInpEnd != nullptr)
		{
			*ppszInpEnd = (char *)pszInp;
		}

		double fraction = CFloatDeco::toDouble(fracHi, fracLo, (bExpNegative) ? (nExpFrac - nExp) : (nExpFrac + nExp));

		return (*pszStart == '-') ? (-fraction) : fraction;
	}

}

//***************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CTypes.h"

namespace Gray
{
	StrLen_t GRAYCALL StrNum::DToATestLegacy(double dVal, OUT char* pszOut, StrLen_t iStrMax, StrLen_t iDecPlaces)
	{
		// Used for compatibility testing.
		char szFormat[64];
		szFormat[0] = '%';
		StrLen_t iLenFormat = 1;
		if (iDecPlaces >= 0)
		{
			szFormat[iLenFormat++] = '.';
			iLenFormat += StrT::UtoA(iDecPlaces, szFormat + iLenFormat, STRMAX(szFormat) - iLenFormat, 10);
		}
		szFormat[iLenFormat++] = 'f';	// will not produce E notation.
		szFormat[iLenFormat] = '\0';

		StrLen_t iLen = StrT::sprintfN<char>(pszOut, iStrMax, szFormat, dVal); // %.2f
																			   // Trim zeros after the .
		if (StrT::HasChar<char>(pszOut, '.'))
		{
			iLen = StrNum::GetTrimCharsLen(pszOut, iLen, '0');
			if (iLen > 0 && pszOut[iLen - 1] == '.') // get rid of the dot too.
			{
				iLen--;
			}
			pszOut[iLen] = '\0';	// eat it.
			if (iLen == 1 && pszOut[0] == '0' && dVal != 0)	// similar effect to %g
			{
				iLen = StrT::sprintfN<char>(pszOut, iStrMax, CSTRCONST("%e"), dVal);
				// Trim 0 before e
				StrLen_t iLenE = StrT::FindCharN<char>(pszOut, 'e');
				if (iLenE > 0)
				{
					StrLen_t iLenE2 = StrNum::GetTrimCharsLen(pszOut, iLenE, '0');
					if (iLenE > iLenE2)
					{
						// e.g. "6.667000e-20"
						CMem::CopyOverlap(pszOut + iLenE2, pszOut + iLenE, ((iLen + 1) - iLenE) * sizeof(char));
					}
				}
			}
		}
		return iLen;
	}

	UNITTEST_CLASS(StrNum)
	{
		void TestDoubleStr(double dVal1, const char* pszVal)
		{
			// Test MUST be reversible. AND do same string to double convert as C++ compiler.
			// Test 1 - Simple reversal of a string.
			double dVal2 = StrNum::toDouble(pszVal, nullptr);
			UNITTEST_TRUE(dVal2 == dVal1);	// NOT 123.12299999

			char szTmp[StrNum::k_LEN_MAX_DIGITS];
			StrLen_t nLenRet = StrNum::DtoAG2(dVal2, szTmp);	// not clipped decimal.
			UNITTEST_TRUE(nLenRet > 0);
			UNITTEST_TRUE(!StrT::Cmp(szTmp, pszVal));
		}

		template <typename _TYPE>
		void TestArray()
		{
			_TYPE aVals[4];
			size_t nRet = StrNum::ToValArray<_TYPE>(aVals, _countof(aVals), "1,2,3,4");
			UNITTEST_TRUE(nRet == 4);

			char szOut[256];
			StrLen_t nLen = StrNum::ValArrayToA<_TYPE>(szOut, STRMAX(szOut), aVals, _countof(aVals)); // static
			UNITTEST_TRUE(nLen == 7);
		}

		UNITTEST_METHOD(StrNum)
		{
			// Test float,double.  

			// Test 1 - Simple reversal.
			TestDoubleStr(0, "0");
			TestDoubleStr(123.123, "123.123");
			TestDoubleStr(-1e-14, "-1e-14");
			// TestDoubleStr(6.667e-20, "6.667e-20");

			// Test 2 - Clipping

			char szTmp[StrNum::k_LEN_MAX_DIGITS];
			StrLen_t nLenRet = StrNum::DtoAG2(123.123, szTmp, 2, '\0');	// Test clip decimal places.
			UNITTEST_TRUE(!StrT::Cmp(szTmp, "123.12"));

			nLenRet = StrNum::DtoAG2(99.99999, szTmp, 2, '\0');	// Test clip decimal places.
			UNITTEST_TRUE(!StrT::Cmp(szTmp, "100.00"));

			// Test 3 - Exponent.
			double dVal = StrNum::toDouble("3.141600E+000", nullptr); // dVal = 3.1415999999999999
			dVal -= 3.1415;
			UNITTEST_TRUE(dVal <= 0.0001);	// near

			nLenRet = StrT::DtoA<char>(dVal, szTmp, STRMAX(szTmp));
			// UNITTEST_TRUE(!StrT::Cmp(szTmp, "123.123"));

			// Test formats.

			dVal = StrNum::toDouble(".123", nullptr);
			dVal -= 0.123;
			UNITTEST_TRUE(dVal <= 0.0001);

			dVal = StrNum::toDouble("123.1234567", nullptr);
			UNITTEST_TRUE(dVal == 123.1234567);

			nLenRet = StrNum::DtoAG2(dVal, szTmp, 6, '\0');	// clip.
			UNITTEST_TRUE(!StrT::Cmp<char>(szTmp, "123.123457"));	// rounds last digit up.
			nLenRet = StrNum::DtoAG2(dVal, szTmp, 7, '\0');	// clip does nothing.
			UNITTEST_TRUE(!StrT::Cmp<char>(szTmp, "123.1234567"));	// NO round last digit.

			// test extremes.
			dVal = -0.00000000000001;
			nLenRet = StrNum::DtoAG2(dVal, szTmp);	// "-0" ?
			UNITTEST_TRUE(!StrT::Cmp<char>(szTmp, "-1e-14"));
			UNITTEST_TRUE(nLenRet == 6);
			dVal = StrNum::toDouble(szTmp);
			UNITTEST_TRUE(dVal == -0.00000000000001);

			// DBL_MAX

			// way too much precision.
			dVal = StrNum::toDouble("123.1234567890123456789012345678901234567890123456789012345678901234567890123456789", nullptr);
			UNITTEST_TRUE(dVal == 123.12345678901235);

			dVal = 123.1234567890123456789012345678901234567890123456789012345678901234567890123456789;
			UNITTEST_TRUE(dVal == 123.12345678901235);

			// Test Large Exponent.
			dVal = CTypeLimit<double>::k_Max;
			nLenRet = StrNum::DtoAG2(dVal, szTmp);
			UNITTEST_TRUE(!StrT::Cmp<char>(szTmp, "1.7976931348623157e+308"));

			dVal = CTypeLimit<double>::k_Min;
			nLenRet = StrNum::DtoAG2(dVal, szTmp);
			UNITTEST_TRUE(!StrT::Cmp<char>(szTmp, "-1.7976931348623157e+308"));

			// Test Exponent sign,
			dVal = StrNum::toDouble("-1.797693e-100");
			UNITTEST_TRUE(dVal > -1 && dVal < 0);
			nLenRet = StrNum::DtoAG2(dVal, szTmp);
			UNITTEST_TRUE(!StrT::Cmp<char>(szTmp, "-1.7976929999999997e-100"));	// ???

			// Test convert to float from double.
			float f1 = 1.5f;	// same accuracy
			dVal = 1.5;
			float f2 = (float)dVal;
			UNITTEST_TRUE(f2 == f1);
			UNITTEST_TRUE(dVal == f1);

			dVal = 1.23;			// more accurate
			f1 = 1.23f;
			f2 = (float)dVal;
			UNITTEST_TRUE(f2 == f1);
			UNITTEST_TRUE(dVal != f1);

			dVal = 1.38;
			f1 = 1.38f;		// more accurate
			f2 = (float)dVal;
			UNITTEST_TRUE(f2 == f1);
			UNITTEST_TRUE(dVal != f1);

			// 
			TestArray<double>();
			TestArray<int>();
		}
	};
	UNITTEST_REGISTER(StrNum, UNITTEST_LEVEL_Core);	// UNITTEST_LEVEL_Off UNITTEST_LEVEL_Core
}

#endif // USE_UNITTESTS
