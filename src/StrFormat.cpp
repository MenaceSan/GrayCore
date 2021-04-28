//
//! @file StrFormat.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// https://github.com/jpbonn/coremark_lm32/blob/master/ee_printf.c
// https://github.com/cjlano/tinyprintf/blob/master/tinyprintf.c
// http://stackoverflow.com/questions/16647278/minimal-fast-implementation-of-sprintf-for-embedded

#include "pch.h"
#include "StrFormat.h"
#include "StrBuilder.h"
#include "StrT.h"
#include "cTypes.h"
#include "cIniBase.h"
#include "cString.h"		// cStringI
#include "cIniBase.h"		//  IIniBaseGetter
#include "cLogMgr.h"	// LOGSTR()
#include "StrArg.h"

namespace Gray
{
	template < typename _TYPE_CH>
	void _cdecl StrBuilder<_TYPE_CH>::AddFormat(const _TYPE_CH* pszFormat, ...) // GRAYCORE_LINK
	{
		va_list vargs;
		va_start(vargs, pszFormat);
		StrFormat<_TYPE_CH>::V(*this, pszFormat, vargs);
		va_end(vargs);
	}

	const char StrFormatBase::k_Specs[16] = "EFGXcdefgiopsux";	// all legal sprintf format tags.  // Omit "S" "apnA"

	template< typename TYPE>
	StrLen_t StrFormat<TYPE>::ParseParam(const TYPE* pszFormat)
	{
		//! Parse the mode for 1 single argument/param from a sprintf() format string.
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
	void StrFormat<TYPE>::RenderString(StrBuilder<TYPE>& out, const TYPE* pszParam, StrLen_t nParamLen, short nPrecision) const
	{
		//! copy a string.
		//! @arg pszParam = a properly terminated string.
		//! @arg nPrecision = how many chars from pszParam do we take? (not including '\0')
		//! Does not terminate.

		if (nPrecision < 0 || nPrecision > nParamLen)
			nPrecision = (short)nParamLen;	// all

		// a truncated or shifted string.
		short nWidth = m_nWidthMin;				// Total width of what we place in pszOut
		if (nWidth == 0)
		{
			nWidth = (short)nPrecision; // all
		}

		StrLen_t i = 0;
		if (!m_bAlignLeft && nWidth > nPrecision) // pad left
		{
			const StrLen_t nWidth2 = nWidth - nPrecision;
			out.AddCharRepeat(' ', nWidth2);
			i = nWidth2;
		}

		out.AddStr2(pszParam, nPrecision);
		i += nPrecision;

		if (m_bAlignLeft && nWidth > i)	// pad right
		{
			out.AddCharRepeat(' ', nWidth - i);
		}
	}

	template< typename TYPE>
	void StrFormat<TYPE>::RenderUInt(StrBuilder<TYPE>& out, const TYPE* pszPrefix, RADIX_t nRadix, char chRadixA, UINT64 uVal) const
	{
		//! @arg nLenOutMax = max string chars including a space for terminator. (even though we don't terminate)

		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS_INT + 4];
		TYPE* pDigits = StrT::ULtoA2(uVal, szTmp, StrNum::k_LEN_MAX_DIGITS_INT, nRadix, chRadixA);

		StrLen_t nDigits = (StrNum::k_LEN_MAX_DIGITS_INT - StrT::Diff(pDigits, szTmp)) - 1;
		StrLen_t nPrecision = m_nPrecision;		// We can increase this to include pad 0 and sign.
		if (nPrecision > nDigits)
			nPrecision = (short)nDigits;

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

		RenderString(out, pDigits, nDigits, (short)nPrecision);
	}

	template< typename TYPE>
	void StrFormat<TYPE>::RenderFloat(StrBuilder<TYPE>& out, char chE, double dVal) const
	{
		// %g
		// @arg chE = <0 e.g. -'e' = optional.

		StrLen_t nPrecision = m_nPrecision;
		if (nPrecision < 0)
		{
			nPrecision = 6;	// default. for decimal places or whole string depending on chLogA < 0
		}

		TYPE szTmp[StrNum::k_LEN_MAX_DIGITS + 4];
		if (m_bPlusSign && dVal >= 0)
		{
			szTmp[0] = '+';	// prefix.
			nPrecision = 1 + StrT::DtoA<TYPE>(dVal, szTmp + 1, StrNum::k_LEN_MAX_DIGITS - 1, nPrecision, chE);		// default = 6.
		}
		else
		{
			nPrecision = StrT::DtoA<TYPE>(dVal, szTmp, StrNum::k_LEN_MAX_DIGITS, nPrecision, chE);		// default = 6.
		}

		RenderString(out, szTmp, nPrecision, (short)nPrecision);
	}

	template< typename TYPE>
	void StrFormat<TYPE>::RenderParam(StrBuilder<TYPE>& out, va_list* pvlist) const
	{
		//! Render a single parameter/spec.
		//! @arg nLenOutMax = max string chars including a space for terminator. (even though we don't terminate)

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
			szTmp[0] = (TYPE)va_arg(*pvlist, int);
			szTmp[1] = '\0';
			RenderString(out, szTmp, 1, m_nPrecision);
			return;
		}

		// case 'S':	// Opposite type? char/wchar_t
		case 's':
		{
			const TYPE* pszParam = va_arg(*pvlist, const TYPE*);
			if (pszParam == nullptr)	// null/bad pointer?
			{
				pszParam = CSTRCONST("(null)");
			}
			else if (cMem::IsCorrupt(pszParam, 16, false))
			{
				pszParam = CSTRCONST("(ERR)");
			}
			RenderString(out, pszParam, StrT::Len(pszParam), m_nPrecision);
			return;
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

			RenderUInt(out, pszPrefix, 10, '\0', (UINT64)nVal);
			return;
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

			RenderUInt(out, pszPrefix, nBaseRadix, chRadixA, nVal);
			return;
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
			RenderFloat(out, chRadixA, va_arg(*pvlist, double));
			return;

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
	}

	template< typename TYPE>
	void GRAYCALL StrFormat<TYPE>::V(StrBuilder<TYPE>& out, const TYPE* pszFormat, va_list vlist)	// static
	{
		//! Replace vsnprintf,_vsnprintf_s,_vsnprintf like StrT::vsprintfN
		//! Make consistent overflow behavior for __linux__ and _WIN32
		//! http://opensource.apple.com//source/ruby/ruby-67.6/ruby/sprintf.c
		//! Emulate the _TRUNCATE flag in _WIN32. just stop at the overflow of nLenOutMax
		//! Do not throw exceptions from this !?
		//! @todo Allow .NET style "{0}" format for strings.  
		//!
		//! @arg out = buffer to write to. max length allowing for '\0' terminator.
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

		if (pszFormat == nullptr)
		{
			return;	// Error? or just do nothing?
		}

		ASSERT(out.get_Str() != pszFormat);
		bool bHasFormatting = false;

		for (StrLen_t iLenForm = 0;; )
		{
			if (out.isOverflow())	// no room for anything more.
				break;

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
					// Render param for int.
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

					paramx.RenderParam(out,
#ifdef __GNUC__
					(va_list*)vlist
#else
						& vlist
#endif

					);
					continue;
				}
			}

			// Just copy stuff into pszOut.
			out.AddChar(ch);
		}
	}

	bool GRAYCALL StrTemplate::HasTemplateBlock(const IniChar_t* pszInp)
	{
		// Does the string include "<?something?>" ?

		ASSERT_N(pszInp != nullptr);

		return StrT::FindStr(pszInp, "<?") != nullptr && StrT::FindStr(pszInp, "?>") != nullptr;
	}

	StrLen_t GRAYCALL StrTemplate::ReplaceTemplateBlock(StrBuilder<IniChar_t>& out, const IniChar_t* pszInp, const IIniBaseGetter* pBlockReq, bool bRecursing)
	{
		//! Replace strings in a marked/delimited block using results from pBlockReq
		//! Used for <?X?> replacement in scripts. also recursive like: <? FUNCTION("i say <VAR.X?>") ?>
		//! e.g. SPEAK "hello there <?SRC.NAME?> my name is <?NAME?>"
		//! @arg
		//!	 pszOut = nullptr = if for testing.
		//!  pBlockReq = submit text found in block for block replacement. nullptr = this is just a test for blocks.
		//! @note
		//!  Allowed to be recursive. ignore blocks inside quotes ?
		//!  Bad properties are just blank.
		//! @return
		//!   length of input used or k_ITERATE_BAD

		ASSERT_N(pszInp != nullptr);

		StrLen_t iBeginBlock = -1;	// output index.
		StrLen_t i = 0;	// input index.

		if (bRecursing)
		{
			ASSERT(pszInp[0] == '<' && pszInp[1] == '?');
		}

		for (; i < StrT::k_LEN_MAX; i++)
		{
			char ch = pszInp[i];
			if (ch == '\0')
				break;

			// just copy the text to pszOut.
			out.AddChar(ch);

			if (iBeginBlock < 0)	// not in block
			{
				if (ch == '?' && i && pszInp[i - 1] == '<')	// found the start !
				{
					iBeginBlock = out.get_Length() - 2; // point to opening <?
					continue;
				}
				continue;
			}

			ASSERT(iBeginBlock >= 0);
			ASSERT(i > 1);

			if (ch == '<' && pszInp[i + 1] == '?')	// found recursive start block.
			{
				out.AdvanceWrite(-1);	// back up.
				StrLen_t iLen = ReplaceTemplateBlock(out, pszInp + i, pBlockReq, true);
				i += iLen;	// skip.
				continue;
			}

			if (ch == '>' && pszInp[i - 1] == '?') // found end of block.
			{
				// NOTE: take the Template Expression from output side in case it is the product of recursive blocks.
				const StrLen_t iTemplateLen = (out.get_Length() - iBeginBlock) - 4;
				cStringI sTemplate(out.get_DataWork() + iBeginBlock + 2, iTemplateLen);
				iBeginBlock = -1;

				HRESULT hRes;
				if (pBlockReq != nullptr)
				{
					cStringI sVal;
					hRes = pBlockReq->PropGet(sTemplate, OUT sVal);
					if (SUCCEEDED(hRes))
					{
						out.AdvanceWrite(-(iTemplateLen + 4));	// back up.
						out.AddStr2(sVal.get_CPtr(), sVal.GetLength());
					}
				}
				else
				{
					hRes = HRESULT_WIN32_C(ERROR_READ_FAULT);
				}
				if (FAILED(hRes))
				{
					// Just in case this really is a >= operator ?
					DEBUG_ERR(("StrTemplate '%s' ERR='%s'", LOGSTR(sTemplate), LOGERR(hRes)));
				}
				if (bRecursing)
				{
					break; // end of recurse block.
				}
			}
		}

		return i;
	}

	template class GRAYCORE_LINK StrBuilder<char>;		// Force Instantiation for DLL.
	template class GRAYCORE_LINK StrBuilder<wchar_t>;	// Force Instantiation for DLL.

	template class GRAYCORE_LINK StrBuilderDyn<char>;		// Force Instantiation for DLL.
	template class GRAYCORE_LINK StrBuilderDyn<wchar_t>;	// Force Instantiation for DLL.

	template class GRAYCORE_LINK StrFormat<char>;		// Force Instantiation for DLL.
	template class GRAYCORE_LINK StrFormat<wchar_t>;	// Force Instantiation for DLL.
}
