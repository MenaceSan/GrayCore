//
//! @file StrU.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "StrU.h"
#include "StrT.h"
#include "cLogMgr.h"
#include "cSystemInfo.h"
#include "cBits.h"

namespace Gray
{
	bool GRAYCALL StrU::IsUTFLead(const void* pvU) // static ?
	{
		//! Skip the stupid Microsoft UTF-8 Byte order marks that are put at the start of a file.
		if (pvU == nullptr)
			return false;
		const BYTE* pU = reinterpret_cast<const BYTE*>(pvU);
		if (pU[0] != StrU::UTFLead_0)
			return false;
		else if (pU[1] == StrU::UTFLead_1 && pU[2] == StrU::UTFLead_2)
			return true;
		else if (pU[1] == StrU::UTFLead_2 && pU[2] == StrU::UTFLead_X)
			return true;
		else if (pU[1] == StrU::UTFLead_2 && pU[2] == StrU::UTFLead_2)
			return true;
		return false;
	}

	StrLen_t GRAYCALL StrU::UTF8Size(wchar_t wChar, int& riStartBits)
	{
		//! How big would this UNICODE char be as UTF8?
		//! @arg wChar = int not wchar_t just to allow any overflow to be detected.
		//! @return The length in bytes i need to store the UTF8, 0=FAILED
		//! RFC 3629 = http://www.ietf.org/rfc/rfc3629.txt
		if (wChar < 0x80)	// needs NO special UTF8 encoding.
		{
			riStartBits = 0;
			return 1;
		}
		else if (wChar < (int)_1BITMASK(11))
		{
			riStartBits = 5;
			return 2;
		}
		else if (wChar < (int)_1BITMASK(16))	// TODO: THIS CANT BE RIGHT UNICODE IS 16 Bits TOTAL !!
		{
			riStartBits = 4;
			return 3;
		}
		else if (wChar < (int)_1BITMASK(21))	// UNICODE can have 21 bits of info ??
		{
			riStartBits = 3;
			return 4;	// StrU::k_UTF8_SIZE_MAX
		}

		riStartBits = 0;
		return k_StrLen_UNK;	// not valid UNICODE char. stop?
	}

	StrLen_t GRAYCALL StrU::UTF8Size1(unsigned char chFirst, int& riStartBits) // static
	{
		//! How many more bytes in this UTF8 sequence? estimated from the first byte of a UTF sequence.
		//! @arg
		//!  chFirst = the first char of the UTF8 sequence.
		//! @return
		//!  <= StrU::k_UTF8_SIZE_MAX

		if (chFirst < 0x80)	// needs NO special UTF8 decoding.
		{
			riStartBits = 0;
			return 1;
		}
		else if ((chFirst & 0xe0) == 0x0c0) // 2 bytes
		{
			riStartBits = 5;
			return 2;
		}
		else if ((chFirst & 0xf0) == 0x0e0) // 3 bytes
		{
			riStartBits = 4;
			return 3;
		}
		else if ((chFirst & 0xf8) == 0x0f0) // 3 bytes
		{
			riStartBits = 3;
			return 4; // StrU::k_UTF8_SIZE_MAX
		}

#if 0
		// Faster ?
		static const BYTE k_utf8ByteTable[256] =
		{
			//	0	1	2	3	4	5	6	7	8	9	a	b	c	d	e	f
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x00
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x10
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x20
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x30
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x40
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x50
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x60
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x70	End of ASCII range
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x80 0x80 to 0xc1 invalid
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0x90
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0xa0
			1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	// 0xb0
			1,	1,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	// 0xc0 0xc2 to 0xdf 2 byte
			2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	// 0xd0
			3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	// 0xe0 0xe0 to 0xef 3 byte
			4,	4,	4,	4,	4,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1	// 0xf0 0xf0 to 0xf4 4 byte, 0xf5 and higher invalid
		};
#endif

		riStartBits = 0;
		return k_StrLen_UNK;	// invalid format !? stop
	}

	StrLen_t GRAYCALL StrU::UTF8Size(const char* pInp, StrLen_t iSizeInpBytes, int& riStartBits) // static
	{
		//! How much UTF8 data do i need to make the UNICODE char?
		//! @arg riStartBits = BIT_ENUM_t
		//! @return The length in bytes i need (from pInp) to make the UNICODE char, 0=FAILED

		if (iSizeInpBytes < 1 || pInp == nullptr)
		{
			return 0; // No data here !!
		}
		return StrU::UTF8Size1((unsigned char)(*pInp), riStartBits);
	}

	StrLen_t GRAYCALL StrU::UTF8toUNICODE(wchar_t& wChar, const char* pInp, StrLen_t iSizeInpBytes) // static
	{
		//! Convert a single UTF8 encoded character (multi chars) to a single UNICODE char.
		//! @return The length used from input string. < iSizeInpBytes, 0=FAILED
		//!	multi byte chars can be up to 4 bytes long! StrU::k_UTF8_SIZE_MAX
		//!
		//! Bytes bits representation:
		//! 1 7	0bbbbbbb
		//! 2 11 110bbbbb 10bbbbbb
		//! 3 16 1110bbbb 10bbbbbb 10bbbbbb
		//! 4 21 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb

		int iStartBits;
		StrLen_t iBytes = StrU::UTF8Size(pInp, iSizeInpBytes, iStartBits);
		if (iBytes <= 0)
		{
			return iBytes;	// FAILED
		}
		if (iBytes == 1)	// needs NO special UTF8 decoding.
		{
			wChar = (wchar_t)(*pInp);
			return 1;
		}
		if (iBytes > iSizeInpBytes)	// not big enough bytes to provide it. (broken char)
		{
			return 0; // FAILED
		}

		unsigned char ch = (unsigned char)(*pInp);
		wchar_t wCharTmp = (wchar_t)(ch & (_1BITMASK(iStartBits) - 1));
		StrLen_t iInp = 1;
		for (; iInp < iBytes; iInp++)
		{
			ch = (unsigned char)(pInp[iInp]);
			// if (( ch & 0xc0 ) != 0x08 )	// bad coding.
			//	return k_StrLen_UNK;
			wCharTmp <<= 6;
			wCharTmp |= ch & 0x3f;
		}

		wChar = wCharTmp;
		return(iBytes);
	}

	StrLen_t GRAYCALL StrU::UNICODEtoUTF8(char* pOut, StrLen_t iSizeOutMaxBytes, wchar_t wChar) // static
	{
		//! Convert a single UNICODE char to UTF8 encoded char (maybe using multi chars).
		//! @return The length < iSizeOutMaxBytes, 0=FAILED
		//!
		//! bytes bits representation:
		//! 1 7	0bbbbbbb
		//! 2 11 110bbbbb 10bbbbbb
		//! 3 16 1110bbbb 10bbbbbb 10bbbbbb
		//! 4 21 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb

		int iStartBits = 0;
		StrLen_t iBytes = StrU::UTF8Size(wChar, iStartBits);
		if (iBytes <= 0)
		{
			return 0;	// FAILED
		}
		if (iBytes > iSizeOutMaxBytes)	// not big enough to hold it!
		{
			return 0;	// FAILED
		}

		if (iBytes == 1)	// needs NO special UTF8 encoding.
		{
			*pOut = (char)wChar;
			return 1;
		}

		StrLen_t iOut = iBytes - 1;
		for (; iOut > 0; iOut--)
		{
			pOut[iOut] = (char)(0x80 | (wChar & (_1BITMASK(6) - 1)));
			wChar >>= 6;
		}

		ASSERT(wChar < (wchar_t)_1BITMASK(iStartBits));
		pOut[0] = (char)((0xfe << iStartBits) | wChar);

		return(iBytes);
	}

	//*********************************************

	StrLen_t GRAYCALL StrU::UTF8toUNICODELen(const char* pInp, StrLen_t iSizeInpBytes) // static
	{
		//! How many UNICODE chars to store this UTF8 string ?
		//! @note if return size is same as input size then no multi char encoding was used. (isANSI)
		//! @return
		//!  Number of wide chars. not including null.

		if (pInp == nullptr)
			return 0;
		if (iSizeInpBytes <= k_StrLen_UNK)
		{
			iSizeInpBytes = StrT::k_LEN_MAX;
		}

		int iStartBits;
		StrLen_t iOut = 0;
		for (; iSizeInpBytes > 0;)
		{
			char ch = *pInp;
			if (ch == '\0')
				break;
			StrLen_t iInpTmp = StrU::UTF8Size(pInp, iSizeInpBytes, iStartBits);
			if (iInpTmp <= 0)
			{
				return k_ITERATE_BAD;
			}
			iOut++;
			pInp += iInpTmp;
			iSizeInpBytes -= iInpTmp;
		}
		return iOut;
	}

	StrLen_t GRAYCALL StrU::UNICODEtoUTF8Size(const wchar_t* pwInp, StrLen_t iSizeInpChars) // static
	{
		//! How many UTF8 bytes to store this UNICODE string ?
		//! @note if return size is same as input size then no multi char encoding is needed. (isANSI)
		//! @return
		//!  Number of bytes. (not including null)

		if (pwInp == nullptr)
			return 0;
		if (iSizeInpChars <= k_StrLen_UNK)
		{
			iSizeInpChars = StrT::k_LEN_MAX;
		}

		int iStartBits;
		StrLen_t iOut = 0;
		for (StrLen_t iInp = 0; iInp < iSizeInpChars; iInp++)
		{
			wchar_t wChar = pwInp[iInp];
			if (wChar == '\0')
				break;
			StrLen_t iOutTmp = StrU::UTF8Size(wChar, iStartBits);
			if (iOutTmp <= 0)
			{
				return k_ITERATE_BAD;
			}
			iOut += iOutTmp;
		}
		return iOut;
	}

	StrLen_t GRAYCALL StrU::UTF8toUNICODE(OUT wchar_t* pwOut, StrLen_t iSizeOutMaxChars, const char* pInp, StrLen_t iSizeInpBytes) // static
	{
		//! Convert the CODEPAGE_t CP_UTF8 default text format to UNICODE
		//! May be network byte order!
		//! Adds null.
		//! similar to _WIN32 ::MultiByteToWideChar().
		//! @arg
		//!  iSizeOutMaxChars = max output size in chars (not bytes) (MUST HAVE ROOM FOR '\0')
		//!  iSizeInpBytes = size of the input string. -1 = '\0' terminated.
		//! @return
		//!  Number of wide chars copied. not including '\0'.
		//

		ASSERT_N(pwOut != nullptr);
		ASSERT_N(pInp != nullptr);
		if (iSizeOutMaxChars <= 0)
		{
			DEBUG_CHECK(iSizeOutMaxChars > 0);
			return k_ITERATE_BAD;
		}

		if (iSizeInpBytes <= k_StrLen_UNK)
		{
			iSizeInpBytes = iSizeOutMaxChars * k_UTF8_SIZE_MAX; // max possible. to '\0'
		}
		if (iSizeInpBytes <= 0 || pInp == nullptr)
		{
			pwOut[0] = '\0';
			return 0;
		}

		iSizeOutMaxChars--;

		StrLen_t iOut = 0;

		// Win95 or __linux__
		for (StrLen_t iInp = 0; iInp < iSizeInpBytes;)
		{
			const unsigned char ch = pInp[iInp];
			if (ch == '\0')
				break;
			if (iOut >= iSizeOutMaxChars)
				break;

			if (ch >= 0x80)	// special UTF8 encoded char.
			{
				wchar_t wChar;
				const StrLen_t iInpTmp = StrU::UTF8toUNICODE(wChar, pInp + iInp, iSizeInpBytes - iInp);
				if (iInpTmp <= 0)
				{
					if (iInpTmp == 0)
						break;
					pwOut[iOut] = ch;
					iInp++;
				}
				else
				{
					pwOut[iOut] = wChar;
					iInp += iInpTmp;
	}
}
			else
			{
				pwOut[iOut] = ch;
				iInp++;
			}

			iOut++;
		}

		pwOut[iOut] = '\0';
		return(iOut);
	}

	StrLen_t GRAYCALL StrU::UNICODEtoUTF8(OUT char* pOut, StrLen_t iSizeOutMaxBytes, const wchar_t* pwInp, StrLen_t iSizeInpChars)	// static
	{
		//! Copy CODEPAGE_t CP_UTF8 to UNICODE.
		//! similar to _WIN32 ::WideCharToMultiByte(). 
		//! @arg
		//!  iSizeInpChars = limit UNICODE chars incoming. -1 = go to null.
		//!  iSizeOutMaxBytes = max output size in bytes (MUST HAVE ROOM FOR '\0')
		//! @return
		//!  Number of bytes. (not including null)
		//! @note
		//!  This need not be a properly terminated string.

		if (iSizeInpChars <= k_StrLen_UNK)
		{
			// just go to '\0'.
			iSizeInpChars = iSizeOutMaxBytes;	// max possible size.
		}
		if (iSizeInpChars <= 0 || pwInp == nullptr)
		{
			pOut[0] = '\0';
			return 0;
		}

		iSizeOutMaxBytes--;

		StrLen_t iOut = 0;

		// Win95 or __linux__ = just assume its really ASCII
		for (StrLen_t iInp = 0; iInp < iSizeInpChars; iInp++)
		{
			// Flip all from network order.
			const wchar_t wChar = pwInp[iInp];
			if (wChar == '\0')
				break;
			if (iOut >= iSizeOutMaxBytes)
				break;
			if (wChar >= 0x80)	// needs special UTF8 encoding.
			{
				const StrLen_t iOutTmp = StrU::UNICODEtoUTF8(pOut + iOut, iSizeOutMaxBytes - iOut, wChar);
				if (iOutTmp <= 0)
				{
					DEBUG_CHECK(iOutTmp == 0); // just skip it!
					continue;
				}
				iOut += iOutTmp;
			}
			else
			{
				pOut[iOut++] = (char)wChar;
			}
		}

		pOut[iOut] = '\0';	// make sure it's null terminated
		return iOut;
	}
}

#if USE_UNITTESTS
#include "cUnitTest.h"

bool GRAYCALL StrU::UnitTestU(const wchar_t* pwText, StrLen_t nLen) // static
{
	// Does the input Unicode string match its equivalent UTF8 string ?
	if (nLen <= k_StrLen_UNK)
		nLen = StrT::Len(pwText);

	char szTmp8[1024];
	const StrLen_t iLen8 = StrU::UNICODEtoUTF8(szTmp8, STRMAX(szTmp8), pwText, nLen);
	if (iLen8 <= 0)		// should be same or larger than nLen 
		return false;

	wchar_t wTmpU[1024];
	if (iLen8 >= STRMAX(wTmpU))
		return false;
	const StrLen_t iLenU = StrU::UTF8toUNICODE(wTmpU, STRMAX(wTmpU), szTmp8, iLen8);
	if (iLenU != nLen)
		return false;
	if (StrT::Cmp<wchar_t>(wTmpU, pwText))	// back to original text?
		return false;
	return true;
	}

bool GRAYCALL StrU::UnitTest8(const char* pszText, StrLen_t nLen) // static
{
	// Does the input UTF8 string match its equivalent Unicode string ?

	if (nLen <= k_StrLen_UNK)
		nLen = StrT::Len(pszText);

	wchar_t wTmpU[1024];
	if (nLen >= STRMAX(wTmpU))
		return false;
	const StrLen_t iLenU = StrU::UTF8toUNICODE(wTmpU, STRMAX(wTmpU), pszText, nLen);
	if (iLenU <= 0)	// should be same or smaller than nLen 
		return false;

	char szTmp8[1024];
	const StrLen_t iLen8 = StrU::UNICODEtoUTF8(szTmp8, STRMAX(szTmp8), wTmpU, iLenU);
	if (iLen8 != nLen)
		return false;
	if (StrT::Cmp<char>(szTmp8, pszText))	// back to original text?
		return false;
	return true;
}

UNITTEST_CLASS(StrU)
{
	UNITTEST_METHOD(StrU)
	{
		// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/quickbrown.txt

		static const wchar_t* kGreekU = L"Σὲ γνωρίζω ἀπὸ τὴν κόψη";
		static const char* kGreek8 = "Î£á½² Î³Î½Ï‰Ïá½·Î¶Ï‰ á¼€Ï€á½¸ Ï„á½´Î½ Îºá½¹ÏˆÎ·";	// in UTF8

		UNITTEST_TRUE(StrU::UnitTestU(kGreekU, k_StrLen_UNK));

		UNITTEST_TRUE(StrU::UnitTest8(k_sTextBlob, k_TEXTBLOB_LEN));
		UNITTEST_TRUE(StrU::UnitTest8(kGreek8, k_StrLen_UNK));
	}
};
UNITTEST_REGISTER(StrU, UNITTEST_LEVEL_Core);
#endif
