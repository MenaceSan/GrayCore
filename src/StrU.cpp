﻿//! @file StrU.cpp
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
bool GRAYCALL StrU::IsUTFLead(const void* pvU) noexcept {  // static
    if (pvU == nullptr) return false;
    const BYTE* pU = reinterpret_cast<const BYTE*>(pvU);
    if (pU[0] != StrU::UTFLead_0) return false;
    if (pU[1] == StrU::UTFLead_1 && pU[2] == StrU::UTFLead_2) return true;  // normal.
    if (pU[1] == StrU::UTFLead_2 && pU[2] == StrU::UTFLead_X) return true;
    if (pU[1] == StrU::UTFLead_2 && pU[2] == StrU::UTFLead_2) return true;
    return false;
}

StrLen_t GRAYCALL StrU::UTF8SizeChar(UINT32 wideChar) noexcept {  // static
#if 0                                                            
    if( !wideChar ) return 1;  // test this! k_UTF8_SIZE_MAX
	static const StrLen_t kWidthBits[32] = { 1 , 1 , 1 , 1 , 1 , 1 , 1 , 2 , 2 , 2 , 2 , 3 , 3 , 3 , 3 , 3 , 4 , 4 , 4 , 4 , 4 , 5 , 5 , 5 , 5 , 5 , 6 , 6 , 6 , 6 , 6 , 7 };
    return kWidthBits[ cBits::Highest1Bit( wideChar ) ];
#else
    if (wideChar < 0x80) return 1;  // 7, needs NO special UTF8 encoding.
    if (wideChar < cBits::Mask1<UINT32>(11)) return 2;
    if (wideChar < cBits::Mask1<UINT32>(16)) return 3;  // wchar_t UNICODE IS 16 Bits TOTAL!
    if (wideChar < cBits::Mask1<UINT32>(21)) return 4;  // StrU::k_UTF8_SIZE_MAX ?? // UNICODE can have 21 bits of info?
    if (wideChar < 0x4000000) return 5;
    if (wideChar < 0x80000000) return 6;
    return 7;
#endif
}

StrLen_t GRAYCALL StrU::UTF8SizeChar1(char firstChar) noexcept {  // static
    const unsigned char firstByte = CastN(unsigned char, firstChar);
    if (firstByte < 0x80) return 1;  // needs NO special UTF8 decoding.
    if ((firstByte & 0xe0) == 0x0c0) return 2;
    if ((firstByte & 0xf0) == 0x0e0) return 3;
    if ((firstByte & 0xf8) == 0x0f0) return 4;  // StrU::k_UTF8_SIZE_MAX
    return k_StrLen_UNK;                        // invalid format !? stop
}

StrLen_t GRAYCALL StrU::UTF8toUNICODEChar(OUT wchar_t& wChar, const char* pInp, StrLen_t iSizeInpBytes) noexcept {  // static
    if (iSizeInpBytes <= 0) return 0;                                                                               // FAILED

    const StrLen_t iSizeChar = UTF8SizeChar1(*pInp);
    if (iSizeChar == 1) {  // needs NO special UTF8 decoding.
        wChar = CastN(wchar_t, *pInp);
        return 1;
    }
    if (iSizeChar <= 0 || iSizeChar > iSizeInpBytes) return 0;  // not big enough bytes to provide it. (broken char) // FAILED

    unsigned char ch = CastN(unsigned char, *pInp);
    wchar_t wCharTmp = ch & cBits::MaskLT<unsigned char>(UTF8StartBits(iSizeChar));
    StrLen_t iInp = 1;
    for (; iInp < iSizeChar; iInp++) {
        ch = CastN(unsigned char, pInp[iInp]);
        // if (( ch & 0xc0 ) != 0x08 )	// bad coding.
        //	return k_StrLen_UNK;
        wCharTmp <<= 6;
        wCharTmp |= ch & 0x3f;
    }

    wChar = wCharTmp;
    return iSizeChar;
}

StrLen_t GRAYCALL StrU::UNICODEtoUTF8Char(char* pOut, StrLen_t iSizeOutMaxBytes, int wideChar) noexcept {  // static

    const StrLen_t iSizeChar = UTF8SizeChar(wideChar);
    if (iSizeChar == 1) {  // needs NO special UTF8 encoding.
        *pOut = CastN(char, wideChar);
        return 1;
    }

    if (iSizeChar <= 0 || iSizeChar > iSizeOutMaxBytes) return 0;  // not big enough to hold it! // FAILED

    StrLen_t iOut = iSizeChar - 1;
    for (; iOut > 0; iOut--) {
        pOut[iOut] = CastN(char, 0x80 | (wideChar & cBits::MaskLT<wchar_t>(6)));
        wideChar >>= 6;
    }

    const int iStartBits = UTF8StartBits(iSizeChar);
    ASSERT(wideChar < cBits::Mask1<wchar_t>(iStartBits));
    pOut[0] = (char)((0xfe << iStartBits) | wideChar);
    return iSizeChar;
}

//*********************************************

StrLen_t GRAYCALL StrU::UTF8toUNICODELen(const cSpan<char>& src) noexcept {  // static
    if (src.isNull()) return 0;

    StrLen_t iOut = 0;
    const char* pInp = src.get_PtrConst();
    StrLen_t iSizeInpBytes = src.GetSize();
    for (; iSizeInpBytes > 0;) {
        const char ch = pInp[0];
        if (ch == '\0') break;
        const StrLen_t iSizeChar = UTF8SizeChar1(ch);
        if (iSizeChar <= 0 || iSizeChar > iSizeInpBytes) return k_ITERATE_BAD;  // broken char.
        iOut++;
        pInp += iSizeChar;
        iSizeInpBytes -= iSizeChar;
    }
    return iOut;
}

StrLen_t GRAYCALL StrU::UNICODEtoUTF8Size(const cSpan<wchar_t>& src) noexcept {  // static
    if (src.isNull()) return 0;

    StrLen_t iOut = 0;
    const wchar_t* pwInp = src.get_PtrConst();
    const StrLen_t iSizeInpChars = src.GetSize();
    for (StrLen_t iInp = 0; iInp < iSizeInpChars; iInp++) {
        const wchar_t wChar = pwInp[iInp];
        if (wChar == '\0') break;
        const StrLen_t iSizeChar = UTF8SizeChar(wChar);
        if (iSizeChar <= 0) return k_ITERATE_BAD;
        iOut += iSizeChar;
    }
    return iOut;
}

StrLen_t GRAYCALL StrU::UTF8toUNICODE(cSpanX<wchar_t> ret, const cSpan<char>& src) noexcept {  // static
    if (ret.isEmpty()) {
        DEBUG_CHECK(!ret.isEmpty());
        return k_ITERATE_BAD;
    }

    wchar_t* pwOut = ret.get_PtrWork();
    if (pwOut == nullptr) return UTF8toUNICODELen(src);

    if (src.isEmpty() || src.isNull()) {
        StrT::SetIfSafe(pwOut);
        return 0;
    }
    const StrLen_t iSizeOutMaxChars = ret.GetSize() - 1;
    StrLen_t iOut = 0;

    // Win95 or __linux__
    for (StrLen_t iInp = 0; iInp < src.GetSize();) {
        const unsigned char ch = src[iInp];
        if (ch == '\0') break;
        if (iOut >= iSizeOutMaxChars) break;
        if (ch >= 0x80) {  // special UTF8 encoded char.
            wchar_t wChar;
            const StrLen_t lenChar = UTF8toUNICODEChar(wChar, src.get_PtrConst() + iInp, src.GetSize() - iInp);
            if (lenChar <= 0) {
                if (lenChar == 0) break;
                pwOut[iOut] = ch;
                iInp++;
            } else {
                pwOut[iOut] = wChar;
                iInp += lenChar;
            }
        } else {
            pwOut[iOut] = ch;
            iInp++;
        }
        iOut++;
    }

    StrT::SetIfSafe(pwOut + iOut);  // make sure it's '\0' terminated
    return iOut;
}

StrLen_t GRAYCALL StrU::UNICODEtoUTF8(cSpanX<char> ret, const cSpan<wchar_t>& src) noexcept {  // static
    if (ret.isEmpty()) {
        DEBUG_CHECK(!ret.isEmpty());
        return 0;
    }

    char* pOut = ret.get_PtrWork();
    if (pOut == nullptr) return UNICODEtoUTF8Size(src);

    if (src.isEmpty() || src.isNull()) {
        StrT::SetIfSafe(pOut);
        return 0;
    }

    const StrLen_t iSizeOutMaxBytes = ret.GetSize() - 1;
    StrLen_t iOut = 0;

    // Win95 or __linux__ = just assume its really ASCII
    for (const wchar_t wChar : src) {
        if (wChar == '\0') break;
        if (iOut >= iSizeOutMaxBytes) break;
        if (wChar >= 0x80) {  // needs special UTF8 encoding.
            const StrLen_t iOutTmp = UNICODEtoUTF8Char(pOut + iOut, iSizeOutMaxBytes - iOut, wChar);
            if (iOutTmp <= 0) {
                DEBUG_CHECK(iOutTmp == 0);  // just skip it!
                continue;
            }
            iOut += iOutTmp;
        } else {
            pOut[iOut++] = CastN(char, wChar);
        }
    }

    StrT::SetIfSafe(pOut + iOut);  // make sure it's '\0' terminated
    return iOut;
}
}  // namespace Gray
