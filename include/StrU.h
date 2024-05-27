//! @file StrU.h
//! Convert to/from UTF8 and UNICODE
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_StrU_H
#define _INC_StrU_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"
#include "cSpan.h"

namespace Gray {
/// <summary>
/// A bunch of functions for UNICODE strings and UTF8. Might be named StrW ? Opposite of StrA.
/// </summary>
struct GRAYCORE_LINK StrU { // : public StrT // static
    static const StrLen_t k_UTF8_SIZE_MAX = 4;  /// Max of 4 UTF8 BYTEs to encode any UNICODE char.

    /// <summary>
    /// http://www.unicode.org/faq/utf_bom.html
    /// Invalid UTF8 sequences are used for special meaning by M$. Placed at start of text file to indicate encoding.
    /// ef bb bf (M$ "lead bytes")
    /// ef bf be
    /// ef bf bf
    /// </summary>
    enum UTFLead_TYPE {
        UTFLead_0 = 0xefU,  // Might be the first part of a UTF8 sequence or a special M$ signal.
        UTFLead_1 = 0xbbU,
        UTFLead_2 = 0xbfU,
        UTFLead_X = 0xbeU,  // alternate.
    };

    /// <summary>
    /// Does this have a Microsoft UTF-8 Byte order marks that are put at the start of a file.
    /// </summary>
    /// <param name="pvU"></param>
    /// <returns></returns>
    static bool GRAYCALL IsUTFLead(const void* pvU) noexcept;

    /// <summary>
    /// How big would this UNICODE char be as UTF8?
    /// RFC 3629 = http://www.ietf.org/rfc/rfc3629.txt
    /// </summary>
    /// <param name="wideChar">int (not wchar_t) just to allow any overflow to be detected.</param>
    /// <returns>The length in bytes (UTF8) i need to store the single char, 0=FAILED, k_UTF8_SIZE_MAX</returns>
    static StrLen_t GRAYCALL UTF8SizeChar(int wideChar) noexcept;

    static StrLen_t inline UTF8StartBits(StrLen_t nSizeChar) noexcept {
        return nSizeChar <= 1 ? 0 : (7 - nSizeChar);
    }

    /// <summary>
    /// How many more bytes in this UTF8 sequence? estimated from the first byte of a UTF sequence. decode UTF8StartBits()
    /// </summary>
    /// <param name="firstByte">the first char/byte of the UTF8 sequence.</param>
    /// <returns>(le) StrU::k_UTF8_SIZE_MAX </returns>
    static StrLen_t GRAYCALL UTF8SizeChar1(char firstChar) noexcept;

    /// <summary>
    /// Convert a single UTF8 encoded character (multi chars) to a single UNICODE char.
    /// like _WIN32 MultiByteToWideChar()
    /// multi byte chars can be up to 4 bytes long! StrU::k_UTF8_SIZE_MAX
    /// Bytes bits representation:
    /// 1 7	 0bbbbbbb
    /// 2 11 110bbbbb 10bbbbbb
    /// 3 16 1110bbbb 10bbbbbb 10bbbbbb
    /// 4 21 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb
    /// </summary>
    /// <param name="wChar"></param>
    /// <param name="pInp"></param>
    /// <param name="iSizeInpBytes"></param>
    /// <returns>The length used from input string. (lt) iSizeInpBytes, 0=FAILED. k_UTF8_SIZE_MAX</returns>
    static StrLen_t GRAYCALL UTF8toUNICODEChar(OUT wchar_t& wChar, const char* pInp, StrLen_t iSizeInpBytes) noexcept;

    /// <summary>
    /// Convert a single UNICODE char to UTF8 encoded char (maybe using multi chars).
    /// like _WIN32 ::WideCharToMultiByte()
    /// bytes bits representation:
    /// 1 7	 0bbbbbbb
    /// 2 11 110bbbbb 10bbbbbb
    /// 3 16 1110bbbb 10bbbbbb 10bbbbbb
    /// 4 21 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb
    /// </summary>
    /// <param name="pOut"></param>
    /// <param name="iSizeOutMaxBytes"></param>
    /// <param name="wChar"></param>
    /// <returns>The length (lt) iSizeOutMaxBytes, 0=FAILED. k_UTF8_SIZE_MAX</returns>
    static StrLen_t GRAYCALL UNICODEtoUTF8Char(char* pOut, StrLen_t iSizeOutMaxBytes, int wideChar) noexcept;

    /// <summary>
    /// How many UNICODE chars to store this UTF8 string ?
    /// @note if return size is same as input size then no multi char encoding was used. (isANSI)
    /// </summary>
    /// <param name="pInp"></param>
    /// <param name="iSizeInpBytes"></param>
    /// <returns>Number of wide chars. not including null.</returns>
    static StrLen_t GRAYCALL UTF8toUNICODELen(const cSpan<char>& src) noexcept;

    /// <summary>
    /// How many UTF8 bytes to store this UNICODE string ?
    /// @note if return size is same as input size then no multi char encoding is needed. (isANSI)
    /// </summary>
    /// <param name="pInp"></param>
    /// <param name="iSizeInpChars"></param>
    /// <returns>Number of bytes. (not including null)</returns>
    static StrLen_t GRAYCALL UNICODEtoUTF8Size(const cSpan<wchar_t>& src) noexcept;

    /// <summary>
    /// Convert the CODEPAGE_t CP_UTF8 default text format to UNICODE
    /// May be network byte order!
    /// Adds null.
    /// similar to _WIN32 ::MultiByteToWideChar().
    /// </summary>
    /// <param name="pOut"></param>
    /// <param name="iSizeOutMaxChars">max output size in chars (not bytes) (MUST HAVE ROOM FOR '\0')</param>
    /// <param name="pInp"></param>
    /// <param name="iSizeInpBytes">size of the input string. -1 = '\0' terminated.</param>
    /// <returns>Number of wide chars copied. not including '\0'.</returns>
    static StrLen_t GRAYCALL UTF8toUNICODE(cSpanX<wchar_t> ret, const cSpan<char>& src) noexcept;

    /// <summary>
    /// convert CODEPAGE_t CP_UTF8 to UNICODE.
    /// similar to _WIN32 ::WideCharToMultiByte().
    /// @note This need not be a properly terminated string.
    /// </summary>
    /// <param name="pOut"></param>
    /// <param name="iSizeOutMaxBytes">max output size in bytes (MUST HAVE ROOM FOR '\0')</param>
    /// <param name="pInp"></param>
    /// <param name="iSizeInpChars">limit UNICODE chars incoming. -1 = go to null.</param>
    /// <returns>Number of bytes. (not including null)</returns>
    static StrLen_t GRAYCALL UNICODEtoUTF8(cSpanX<char> ret, const cSpan<wchar_t>& src) noexcept;
};
}  // namespace Gray

#endif  // _INC_StrU_H
