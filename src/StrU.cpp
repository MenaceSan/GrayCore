//
//! @file StrU.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrT.h"
#include "StrU.h"
#include "cBits.h"
#include "cLogMgr.h"
#include "cSystemInfo.h"

namespace Gray {
bool GRAYCALL StrU::IsUTFLead(const void* pvU) {  // static
    if (pvU == nullptr) return false;
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

StrLen_t GRAYCALL StrU::UTF8Size(int nChar, OUT int& riStartBits) {  // static
    if (nChar < 0x80) {                                              // needs NO special UTF8 encoding.
        riStartBits = 0;
        return 1;
    } else if (nChar < cBits::Mask1<int>(11)) {
        riStartBits = 5;
        return 2;
    } else if (nChar < cBits::Mask1<int>(16)) {  // TODO: THIS CANT BE RIGHT UNICODE IS 16 Bits TOTAL !!
        riStartBits = 4;
        return 3;
    } else if (nChar < cBits::Mask1<int>(21)) {  // UNICODE can have 21 bits of info ??
        riStartBits = 3;
        return 4;  // StrU::k_UTF8_SIZE_MAX
    }

    // This is technically valid for UTF8!?
    riStartBits = 0;
    return k_StrLen_UNK;  // not valid UNICODE char. stop?
}

StrLen_t GRAYCALL StrU::UTF8Size1(unsigned char firstByte, OUT int& riStartBits) {  // static
    if (firstByte < 0x80) {                                                         // needs NO special UTF8 decoding.
        riStartBits = 0;
        return 1;
    } else if ((firstByte & 0xe0) == 0x0c0) {  // 2 bytes
        riStartBits = 5;
        return 2;
    } else if ((firstByte & 0xf0) == 0x0e0) {  // 3 bytes
        riStartBits = 4;
        return 3;
    } else if ((firstByte & 0xf8) == 0x0f0) {  // 3 bytes
        riStartBits = 3;
        return 4;  // StrU::k_UTF8_SIZE_MAX
    }

#if 0
	// Faster ?
	static const BYTE k_utf8ByteTable[256] = {
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
    return k_StrLen_UNK;  // invalid format !? stop
}

StrLen_t GRAYCALL StrU::UTF8Size(const char* pInp, StrLen_t iSizeInpBytes, OUT int& riStartBits) {  // static
    if (iSizeInpBytes < 1 || pInp == nullptr) return 0;                                             // No data here !!
    return StrU::UTF8Size1(CastN(unsigned char, *pInp), riStartBits);
}

StrLen_t GRAYCALL StrU::UTF8toUNICODE(OUT wchar_t& wChar, const char* pInp, StrLen_t iSizeInpBytes) {  // static
    int iStartBits;
    const StrLen_t iBytes = StrU::UTF8Size(pInp, iSizeInpBytes, iStartBits);
    if (iBytes <= 0) return iBytes;  // FAILED

    if (iBytes == 1) {  // needs NO special UTF8 decoding.
        wChar = CastN(wchar_t, *pInp);
        return 1;
    }
    if (iBytes > iSizeInpBytes) {  // not big enough bytes to provide it. (broken char)
        return 0;                  // FAILED
    }

    unsigned char ch = CastN(unsigned char, *pInp);
    wchar_t wCharTmp = ch & cBits::MaskLT<unsigned char>(iStartBits);
    StrLen_t iInp = 1;
    for (; iInp < iBytes; iInp++) {
        ch = CastN(unsigned char, pInp[iInp]);
        // if (( ch & 0xc0 ) != 0x08 )	// bad coding.
        //	return k_StrLen_UNK;
        wCharTmp <<= 6;
        wCharTmp |= ch & 0x3f;
    }

    wChar = wCharTmp;
    return iBytes;
}

StrLen_t GRAYCALL StrU::UNICODEtoUTF8(char* pOut, StrLen_t iSizeOutMaxBytes, wchar_t wChar) {  // static
    int iStartBits = 0;
    const StrLen_t iBytes = StrU::UTF8Size(wChar, iStartBits);
    if (iBytes <= 0) return 0;        // FAILED
    if (iBytes > iSizeOutMaxBytes) {  // not big enough to hold it!
        return 0;                     // FAILED
    }
    if (iBytes == 1) {  // needs NO special UTF8 encoding.
        *pOut = CastN(char, wChar);
        return 1;
    }

    StrLen_t iOut = iBytes - 1;
    for (; iOut > 0; iOut--) {
        pOut[iOut] = CastN(char, 0x80 | (wChar & cBits::MaskLT<wchar_t>(6)));
        wChar >>= 6;
    }

    ASSERT(wChar < cBits::Mask1<wchar_t>(iStartBits));
    pOut[0] = (char)((0xfe << iStartBits) | wChar);

    return iBytes;
}

//*********************************************

StrLen_t GRAYCALL StrU::UTF8toUNICODELen(const char* pInp, StrLen_t iSizeInpBytes) {  // static
    if (pInp == nullptr) return 0;
    if (iSizeInpBytes <= k_StrLen_UNK) {
        iSizeInpBytes = StrT::k_LEN_MAX;
    }

    int iStartBits;
    StrLen_t iOut = 0;
    for (; iSizeInpBytes > 0;) {
        char ch = *pInp;
        if (ch == '\0') break;
        StrLen_t iInpTmp = StrU::UTF8Size(pInp, iSizeInpBytes, iStartBits);
        if (iInpTmp <= 0) {
            return k_ITERATE_BAD;
        }
        iOut++;
        pInp += iInpTmp;
        iSizeInpBytes -= iInpTmp;
    }
    return iOut;
}

StrLen_t GRAYCALL StrU::UNICODEtoUTF8Size(const wchar_t* pwInp, StrLen_t iSizeInpChars) {  // static
    if (pwInp == nullptr) return 0;
    if (iSizeInpChars <= k_StrLen_UNK) {
        iSizeInpChars = StrT::k_LEN_MAX;
    }

    int iStartBits;
    StrLen_t iOut = 0;
    for (StrLen_t iInp = 0; iInp < iSizeInpChars; iInp++) {
        wchar_t wChar = pwInp[iInp];
        if (wChar == '\0') break;
        StrLen_t iOutTmp = StrU::UTF8Size(wChar, iStartBits);
        if (iOutTmp <= 0) {
            return k_ITERATE_BAD;
        }
        iOut += iOutTmp;
    }
    return iOut;
}

StrLen_t GRAYCALL StrU::UTF8toUNICODE(OUT wchar_t* pwOut, StrLen_t iSizeOutMaxChars, const char* pInp, StrLen_t iSizeInpBytes) {  // static
    ASSERT_NN(pwOut);
    ASSERT_NN(pInp);
    if (iSizeOutMaxChars <= 0) {
        DEBUG_CHECK(iSizeOutMaxChars > 0);
        return k_ITERATE_BAD;
    }

    if (iSizeInpBytes <= k_StrLen_UNK) {
        iSizeInpBytes = iSizeOutMaxChars * k_UTF8_SIZE_MAX;  // max possible. to '\0'
    }
    if (iSizeInpBytes <= 0 || pInp == nullptr) {
        pwOut[0] = '\0';
        return 0;
    }

    iSizeOutMaxChars--;

    StrLen_t iOut = 0;

    // Win95 or __linux__
    for (StrLen_t iInp = 0; iInp < iSizeInpBytes;) {
        const unsigned char ch = pInp[iInp];
        if (ch == '\0') break;
        if (iOut >= iSizeOutMaxChars) break;

        if (ch >= 0x80)  // special UTF8 encoded char.
        {
            wchar_t wChar;
            const StrLen_t iInpTmp = StrU::UTF8toUNICODE(wChar, pInp + iInp, iSizeInpBytes - iInp);
            if (iInpTmp <= 0) {
                if (iInpTmp == 0) break;
                pwOut[iOut] = ch;
                iInp++;
            } else {
                pwOut[iOut] = wChar;
                iInp += iInpTmp;
            }
        } else {
            pwOut[iOut] = ch;
            iInp++;
        }

        iOut++;
    }

    pwOut[iOut] = '\0';
    return (iOut);
}

StrLen_t GRAYCALL StrU::UNICODEtoUTF8(OUT char* pOut, StrLen_t iSizeOutMaxBytes, const wchar_t* pwInp, StrLen_t iSizeInpChars) {  // static
    if (iSizeInpChars <= k_StrLen_UNK) {
        // just go to '\0'.
        iSizeInpChars = iSizeOutMaxBytes;  // max possible size.
    }
    if (iSizeInpChars <= 0 || pwInp == nullptr) {
        pOut[0] = '\0';
        return 0;
    }

    iSizeOutMaxBytes--;

    StrLen_t iOut = 0;

    // Win95 or __linux__ = just assume its really ASCII
    for (StrLen_t iInp = 0; iInp < iSizeInpChars; iInp++) {
        // Flip all from network order.
        const wchar_t wChar = pwInp[iInp];
        if (wChar == '\0') break;
        if (iOut >= iSizeOutMaxBytes) break;
        if (wChar >= 0x80) {  // needs special UTF8 encoding.
            const StrLen_t iOutTmp = StrU::UNICODEtoUTF8(pOut + iOut, iSizeOutMaxBytes - iOut, wChar);
            if (iOutTmp <= 0) {
                DEBUG_CHECK(iOutTmp == 0);  // just skip it!
                continue;
            }
            iOut += iOutTmp;
        } else {
            pOut[iOut++] = (char)wChar;
        }
    }

    pOut[iOut] = '\0';  // make sure it's null terminated
    return iOut;
}
}  // namespace Gray
