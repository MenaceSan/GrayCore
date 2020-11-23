//
//! @file StrFormat.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// https://github.com/jpbonn/coremark_lm32/blob/master/ee_printf.c
// https://github.com/cjlano/tinyprintf/blob/master/tinyprintf.c
// http://stackoverflow.com/questions/16647278/minimal-fast-implementation-of-sprintf-for-embedded

#include "pch.h"
#include "StrFormat.h"
#include "StrT.h"
#include "cTypes.h"

namespace Gray
{
	const char StrFormatBase::k_Specs[16] = "EFGXcdefgiopsux";	// all legal sprintf format tags.  // Omit "S" "apnA"

	template< typename TYPE>
	StrLen_t StrFormat<TYPE>::ParseParam(const TYPE* pszFormat)
	{
		//! Parse the mode for 1 single argument/param from a printf() format string.
		//! e.g. %d,%s,%u,%g,%f, etc.
		//! http://www.cplusplus.com/reference/cstdio/printf/
		//! @arg pszFormat = "%[flags][width][.precision][length]specifier"
		//! @return 0 = not a valid mode format. just flush the format.

		m_nSpec = '\0';
		m_nWidthMin = 0;	// default = all. No padding.
		m_nPrecision = -1;
		m_nLong = 0;	// Account for "ll"
		m_bAlignLeft = false;
		m_bPlusSign = false;
		m_bLeadZero = false;
		m_bWidthArg = false;
		m_bAddPrefix = false;

		ASSERT(pszFormat != nullptr);

		BYTE nVal = 0;
		bool bHasDot = false;

		StrLen_t i = 0;
		for (;; i++)
		{
			TYPE ch = pszFormat[i];
			if (ch <= ' ' || ch >= 128)
				break;	// junk

			m_nSpec = FindSpec((char)ch);
			if (m_nSpec != '\0')	// legit end.
			{
				if (bHasDot)
					m_nPrecision = nVal;
				else
					m_nWidthMin = nVal;
				return i + 1;	// Display it.
			}

			switch (ch)
			{
			case '%':
				if (i == 0)
					return 1;	// Legit. %%. skip first and output the second.
				break;	// junk
			case '.':
				bHasDot = true;
				m_nWidthMin = nVal;	// next is m_nPrecision
				nVal = 0;
				continue;
			case 'l':		// NOT 'L'
				m_nLong++;
				continue;
			case  '-':
				m_bAlignLeft = true;
				continue;
			case '+':
				m_bPlusSign = true;
				continue;
			case '*':	// Width is an argument.
				m_bWidthArg = true;
				continue;
			case '#':	// add prefix. or forces the written output to contain a decimal point
				m_bAddPrefix = true;
				continue;
			case ' ':	// is this allowed??
				break;
			}

			if (StrChar::IsDigit(ch))
			{
				if (ch == '0' && nVal == 0)
				{
					m_bLeadZero = true;
					continue;
				}
				nVal *= 10;
				nVal += (BYTE)StrChar::Dec2U(ch);
				continue;
			}
			break;	// junk.
		}

		ASSERT(0);	// Junk Should not happen? Not a valid format string.
		return 0;	// junk. just copy stuff.
	}

	template< typename TYPE>
	StrLen_t StrFormat<TYPE>::RenderString(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszParam, StrLen_t nParamLen, short nPrecision) const
	{
		//! @arg nLenOutMax = max string chars including a space for terminator. (even though we don't terminate)
		//! @arg pszParam = a properly terminated string.
		//! @arg nPrecision = how many chars from pszParam do we take? (not including '\0')
		//! Does not terminate.

		if (nPrecision < 0 || nPrecision > nParamLen)
			nPrecision = (short)nParamLen;	// all

		if (nPrecision >= m_nWidthMin)	// simple case = copy
		{
			return StrT::CopyLen(pszOut, pszParam, MIN(nLenOutMax, nPrecision + 1));
		}

		nLenOutMax--;
		short nWidth = m_nWidthMin;				// Total width of what we place in pszOut
		if (nWidth == 0)
			nWidth = (short)nPrecision; // all

		StrLen_t nEnd;
		StrLen_t i = 0;
		if (!m_bAlignLeft && nWidth > nPrecision) // pad left
		{
			nEnd = nWidth - nPrecision;
			if (nEnd > nLenOutMax)
				nEnd = nLenOutMax;
			cValArray::FillQty<TYPE>(pszOut, nEnd, ' ');
			i = nEnd;
		}

		nEnd = i + nPrecision;
		if (nEnd > nLenOutMax)
			nPrecision = (short)(nLenOutMax - i);
		cMem::Copy(pszOut + i, pszParam, sizeof(TYPE) * nPrecision);
		i += nPrecision;

		if (m_bAlignLeft && nWidth > i)	// pad right
		{
			if (nWidth > nLenOutMax)
				nWidth = (short) nLenOutMax;
			cValArray::FillQty<TYPE>(pszOut + i, nWidth - i, ' ');
			i = nWidth;
		}

		return i; // +
	}

	template< typename TYPE>
	StrLen_t StrFormat<TYPE>::RenderUInt(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszPrefix, RADIX_t nRadix, char chRadixA, UINT64 uVal) const
	{
		//! @arg nLenOutMax = max string chars including a space for terminator. (even though we don't terminate)

		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 4];
		TYPE* pDigits = StrT::ULtoA2(uVal, szTmp, StrNum::k_LEN_MAX_DIGITS_INT, nRadix, chRadixA);

		StrLen_t nDigits = (StrNum::k_LEN_MAX_DIGITS_INT - StrT::Diff(pDigits, szTmp)) - 1;
		StrLen_t nPrecision = m_nPrecision;		// We can increase this to include pad 0 and sign.
		if (nPrecision > nDigits)
			nPrecision = (short) nDigits;

		if (m_bLeadZero && m_nWidthMin > nDigits)
		{
			// 0 pad is part of szTmp. Replaces ' ' padding.
			StrLen_t nPad = m_nWidthMin;
			if (nPad >= StrNum::k_LEN_MAX_DIGITS_INT)
				nPad = StrNum::k_LEN_MAX_DIGITS_INT;
			nPad -= nDigits;
			pDigits -= nPad;
			cValArray::FillQty<TYPE>(pDigits, nPad, '0');
			nDigits += nPad;
			if (nPrecision >= 0)
				nPrecision += nPad;
		}

		if (pszPrefix != nullptr)
		{
			// Sign is part of szTmp. Can't be padded out.
			StrLen_t nPrefix = StrT::Len(pszPrefix);
			ASSERT(nPrefix <= 2);	// we left some prefix space for this.
			pDigits -= nPrefix;
			cMem::Copy(pDigits, pszPrefix, nPrefix * sizeof(TYPE));
			nDigits += nPrefix;
			if (nPrecision >= 0)
				nPrecision += nPrefix;
		}

		return RenderString(pszOut, nLenOutMax, pDigits, nDigits, (short) nPrecision);
	}

	template< typename TYPE>
	StrLen_t StrFormat<TYPE>::RenderFloat(TYPE* pszOut, StrLen_t nLenOutMax, char chRadixA, double dVal) const
	{
		StrLen_t nPrecision = m_nPrecision;
		if (nPrecision < 0)
		{
			nPrecision = 6;	// default. for decimal places or whole string depending on chRadixA < 0
		}

		if (nPrecision > m_nWidthMin)	// simple case = copy
		{
			if (m_bPlusSign && dVal >= 0)
			{
				*pszOut = '+';	// prefix.
				return 1 + StrT::DtoA<TYPE>(dVal, pszOut + 1, nLenOutMax - 1, nPrecision, chRadixA);		// default = 6.
			}

			return StrT::DtoA<TYPE>(dVal, pszOut, nLenOutMax, nPrecision, chRadixA);		// default = 6.
		}

		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
		if (m_bPlusSign && dVal >= 0)
		{
			szTmp[0] = '+';	// prefix.
			nPrecision = 1 + StrT::DtoA<TYPE>(dVal, szTmp + 1, StrNum::k_LEN_MAX_DIGITS - 1, nPrecision, chRadixA);		// default = 6.
		}
		else
		{
			nPrecision = StrT::DtoA<TYPE>(dVal, szTmp, StrNum::k_LEN_MAX_DIGITS, nPrecision, chRadixA);		// default = 6.
		}

		return RenderString(pszOut, nLenOutMax, szTmp, nPrecision, (short) nPrecision);
	}

	template< typename TYPE>
	StrLen_t StrFormat<TYPE>::RenderParam(TYPE* pszOut, StrLen_t nLenOutMax, va_list* pvlist) const
	{
		//! Render a single parameter/spec.
		//! @arg nLenOutMax = max string chars including a space for terminator. (even though we don't terminate)

		if (nLenOutMax <= 0)	// Not even room for '\0'
			return 0;

		RADIX_t nBaseRadix = 10;
		char chRadixA = 'a';
		const TYPE* pszPrefix = nullptr;
		BYTE nLong = m_nLong;

		switch (m_nSpec)
		{
		case 'c': // char.
		{
			if (m_nWidthMin != 0 || m_bWidthArg)
			{
				// repeat char!
			}
			TYPE szTmp[2];
			szTmp[0] = (TYPE) va_arg(*pvlist, int);
			szTmp[1] = '\0';
			return RenderString(pszOut, nLenOutMax, szTmp, 1, m_nPrecision);
		}

		// case 'S':	// Opposite type? char/wchar_t
		case 's':
		{
			const TYPE* pszParam = va_arg(*pvlist, const TYPE*);
			if (pszParam == nullptr)	// null/bad pointer?
			{
				pszParam = CSTRCONST("(null)");
			}
			else if (!cMem::IsValid(pszParam, 16))
			{
				pszParam = CSTRCONST("(ERR)");
			}
			return RenderString(pszOut, nLenOutMax, pszParam, StrT::Len(pszParam), m_nPrecision);
		}

		case 'd':
		case 'i':
		{
			INT64 nVal;
			if (nLong > 1)
			{
				nVal = va_arg(*pvlist, INT64);
			}
			else
			{
				nVal = va_arg(*pvlist, INT32);
			}

			if (nVal < 0)
			{
				nVal = -nVal;
				pszPrefix = CSTRCONST("-");
			}
			else if (m_bPlusSign)
			{
				pszPrefix = CSTRCONST("+");
			}

			return RenderUInt(pszOut, nLenOutMax, pszPrefix, 10, '\0', (UINT64)nVal);
		}

		case 'u':
		do_num_uns:
		{
			UINT64 nVal;
			if (nLong > 1)
			{
				nVal = va_arg(*pvlist, UINT64);
			}
			else
			{
				nVal = va_arg(*pvlist, UINT32);
			}

			return RenderUInt(pszOut, nLenOutMax, pszPrefix, nBaseRadix, chRadixA, nVal);
		}

		case 'X':	// Upper case
			chRadixA = 'A';
			nBaseRadix = 16;
			if (m_bAddPrefix)
			{
				pszPrefix = CSTRCONST("0X");
			}
			goto do_num_uns;

		case 'p':	// A pointer. M$ specific ?
			// Set Size for the pointer.
#ifdef USE_64BIT
			nLong = 2;
#else
			nLong = 1;
#endif

		case 'x':	// int hex.
			nBaseRadix = 16;
			if (m_bAddPrefix)
			{
				pszPrefix = CSTRCONST("0x");
			}
			goto do_num_uns;
		case 'o':	// int octal.
			nBaseRadix = 8;
			if (m_bAddPrefix)
			{
				pszPrefix = CSTRCONST("0");
			}
			goto do_num_uns;
		case 'b':	// int binary
			nBaseRadix = 2;
			if (m_bAddPrefix)
			{
				pszPrefix = CSTRCONST("0");
			}
			goto do_num_uns;

		case 'E':	// Upper case. Scientific notation (mantissa/exponent), uppercase
			chRadixA = 'E';
			goto do_num_float;
		case 'e': // Float precision = decimal places
			chRadixA = 'e';
			goto do_num_float;
		case 'F':	// Upper case. Decimal floating point, uppercase. default = 6
		case 'f': // Float precision = decimal places. default m_nPrecision = 6
			chRadixA = '\0';
		do_num_float:
			return RenderFloat(pszOut, nLenOutMax, chRadixA, va_arg(*pvlist, double));

		case 'G':	// Upper case. Use the shortest representation: %E or %F
			chRadixA = -'E';
			goto do_num_float;
		case 'g': // Float - precision = total digits.
			chRadixA = -'e';
			goto do_num_float;

			// case 'a':	// Float hex
		default:		// Should never happen.
			break;
		}

		ASSERT(false);	// Should never happen.
		return 0;
	}

	template< typename TYPE>
	StrLen_t GRAYCALL StrFormat<TYPE>::FormatV(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszFormat, va_list vlist)	// static
	{
		//! Replace vsnprintf,_vsnprintf_s,_vsnprintf like StrT::vsprintfN
		//! Make consistent overflow behavior for __linux__ and _WIN32
		//! http://opensource.apple.com//source/ruby/ruby-67.6/ruby/sprintf.c
		//! Emulate the _TRUNCATE flag in _WIN32. just stop at the overflow of nLenOutMax
		//! Do not throw exceptions from this !?
		//! @todo Allow .NET style "{0}" format for strings.  
		//!
		//! @arg pszOut = nullptr = just estimate for 2 pass formatting.
		//! @arg nLenOutMax = max length allowing for '\0' terminator.
		//! @return the size i used (in chars)(or would use if pszOut==nullptr). Not including '\0'.
		//!
		//! Examples: (must pass UnitTestFormat)
		//! "{ts'%04d-%02d-%02d %02d:%02d:%02d'}",
		//! "%s %0 %1 %2"
		//!
		//! @note Windows _snprintf and _vsnprintf are not compatible to Linux versions.
		//!  Linux result is the size of the buffer that it should have.
		//!  Windows Result value is not size of buffer needed, but -1 if no fit is possible.
		//!  Newer Windows versions have a _TRUNCATE option to just truncate the string and return used size.
		//!  _vscwprintf can be used to estimate the size needed in advance using a 2 pass method.

		if (pszFormat == nullptr || nLenOutMax <= 1)
		{
			if (nLenOutMax <= 0)
			{
				return 0;	// Weird.
			}
			if (pszOut != nullptr)
			{
				*pszOut = '\0';
			}
			return 0;	// Error? or just do nothing?
		}

		ASSERT(pszOut != pszFormat);
		bool bHasFormatting = false;
		StrLen_t nLenOut = 0;

		for (StrLen_t iLenForm = 0;; )
		{
			TYPE ch = pszFormat[iLenForm++];
			if (ch == '\0')	// At end.
			{
				// ASSERT(bHasFormatting); Don't use this as a string copy! its dangerous and wasteful.
				break;
			}

			if (ch == '%')	// Start of new param format. Maybe.
			{
				bHasFormatting = true;
				StrFormat<TYPE> paramx;
				iLenForm += paramx.ParseParam(pszFormat + iLenForm);

				if (paramx.m_nSpec != '\0')
				{
					// Render param.
					if (paramx.m_bWidthArg)
					{
						int iVal = va_arg(vlist, int);
						if (iVal < 0)
						{
							iVal = -iVal;
							paramx.m_bAlignLeft = true;
						}
						paramx.m_nWidthMin = (BYTE)iVal;
					}

					StrLen_t nLenOut2 = paramx.RenderParam(pszOut, nLenOutMax - nLenOut,
#ifdef __GNUC__
							(va_list *)vlist
#else
							&vlist
#endif

					);
					nLenOut += nLenOut2;
					if (pszOut != nullptr)
					{
						pszOut += nLenOut2;
					}
					continue;
				}
			}

			// Just copy stuff into pszOut.
			if (++nLenOut >= nLenOutMax)	// past limit. Stop.
			{
				nLenOut--;
				break;
			}
			if (pszOut != nullptr)
			{
				*pszOut = ch;
				pszOut++;
			}
		}

		// Terminate!
		if (pszOut != nullptr)
		{
			ASSERT(nLenOut <= nLenOutMax);
			*pszOut = '\0';
		}

		return nLenOut;		// Length not including '\0'.
	}

	template< typename TYPE>
	StrLen_t _cdecl StrFormat<TYPE>::FormatF(TYPE* pszOut, StrLen_t nLenOutMax, const TYPE* pszFormat, ...)	// static
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		StrLen_t nLenOut = FormatV(pszOut, nLenOutMax, pszFormat, vargs);
		va_end(vargs);
		return nLenOut;
	}

	template class GRAYCORE_LINK StrFormat<char>;		// Force Instantiation for DLL.
	template class GRAYCORE_LINK StrFormat<wchar_t>;	// Force Instantiation for DLL.
}
