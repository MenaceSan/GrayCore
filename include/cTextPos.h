//! @file cTextPos.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cTextPos_H
#define _INC_cTextPos_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrChar.h"
#include "StrConst.h"
#include "cSpan.h"
#include "cStreamProgress.h"

namespace Gray {
/// <summary>
/// Where inside of a cFileText, cFileTextReader (or other type of text buffer) are we ?
/// Text files have line numbers that won't always correspond to offset when newlines have been transformed.
/// ASSUME: we know which file it is in.
/// We have no idea how long the file is.
/// </summary>
class GRAYCORE_LINK cTextPos {
 protected:
    STREAM_POS_t _nOffset = k_STREAM_POS_ERR;  /// byte offset into the file. 0 based. first line may not be start of file?
    ITERATE_t _nLineNum = k_ITERATE_BAD;       /// 0 based row/line, for debug purposes if there is an error.
    StrLen_t _nColNum = k_StrLen_UNK;          /// 0 based column number. if used. # of characters, not bytes. UTF can have multi bytes per char. if _nLineNum = -1 or -2 can be used to indicate errors.

 public:
    static const cTextPos k_Invalid;  /// Set to invalid values. k_STREAM_POS_ERR
    static const cTextPos k_Zero;     /// Top of file.

 public:
    cTextPos(STREAM_POS_t lOffset, ITERATE_t iLineNum, StrLen_t iColNum) noexcept : _nOffset(lOffset), _nLineNum(iLineNum), _nColNum(iColNum) {}

    void InitTop(STREAM_POS_t lOffset = 0) noexcept {
        _nOffset = lOffset;  // first line may not be start of file?
        _nLineNum = 0;
        _nColNum = 0;
    }

    /// <summary>
    /// is it on the top line? k_Zero
    /// </summary>
    bool isTopLine() const noexcept {
        return _nLineNum == 0;
    }

    /// <summary>
    /// is invalid values? Not k_Invalid
    /// </summary>
    /// <returns></returns>
    bool isValidPos() const noexcept {
        return _nLineNum >= 0;  // _nOffset >= 0
    }
    STREAM_POS_t get_Offset() const noexcept {
        //! Offset in bytes into the stream.
        return _nOffset;
    }
    ITERATE_t get_LineNum() const noexcept {  /// Get 0 based line.
        return this->_nLineNum;
    }
    ITERATE_t get_Line1() const noexcept {  /// Get 1 based line.
        return this->_nLineNum + 1;
    }
    StrLen_t get_Column1() const noexcept {  /// Get 1 based column.
        return this->_nColNum + 1;
    }

    void IncOffset(StrLen_t nLenOffsetSrc) noexcept {
        _nOffset += nLenOffsetSrc;
        _nColNum += nLenOffsetSrc;
    }
    void IncOffset(StrLen_t nLenOffsetSrc, StrLen_t nLenCol) noexcept {
        // nLenCol = 0 = invisible chars don't count.
        _nOffset += nLenOffsetSrc;
        _nColNum += nLenCol;
    }
    /// <summary>
    /// Add one single UTF-8 char that is not a newline or tab.
    /// </summary>
    void IncChar(StrLen_t nLenChar = 1) noexcept {
        _nOffset += nLenChar;  // UTF8 can span multiple bytes.
        _nColNum++;
    }

    /// We just read a single line.
    void IncLine(StrLen_t nLenChars = 1) noexcept {
        // CRLF or LF
        _nOffset += nLenChars;  // UTF8 can span multiple bytes.
        _nLineNum++;
        _nColNum = 0;
    }

    StrLen_t GetStr2(OUT cSpanX<char> ret) const;
};

/// <summary>
/// current File/cXmlReader/cJSONReader/Etc parsing position. include cTextPos
/// similar to cStreamInput but for a memory buffer.
/// cTextPos = Current cursor position for the file. used for error messages, etc.
/// </summary>
class GRAYCORE_LINK cTextReaderSpan : public cTextPos {
    typedef cTextPos SUPER_t;

 protected:
    cSpan<char> _Text;  /// the UTF8 text to be read. don't advance cTextPos::_nOffset outside this.

 public:
    const StrLen_t _nTabSize;  /// for proper tracking of the column number on errors. and cTextPos. 0 = not used/don't care.

 public:
    cTextReaderSpan(const cSpan<char>& span, StrLen_t nTabSize = cStrConst::k_TabSize) noexcept : cTextPos(0, 0, 0), _Text(span), _nTabSize(nTabSize) {}

    StrLen_t get_LenMax() const noexcept {
        return _Text.get_MaxLen();
    }
    StrLen_t get_LenRemaining() const noexcept {
        const StrLen_t nLenMax = get_LenMax();
        if (nLenMax <= CastN(StrLen_t, this->_nOffset)) return 0;
        return nLenMax - CastN(StrLen_t, this->_nOffset);
    }
    bool isValidIndex() const noexcept {
        return _Text.IsValidIndex(CastN(ITERATE_t, this->_nOffset));  // includes _nOffset >= 0
    }
    bool isValidPos() const noexcept {
        //! is invalid values? Not k_Invalid
        return SUPER_t::isValidPos() && _Text.isValidPtr() && isValidIndex();  // includes _nOffset >= 0
    }
    const char* get_CursorPtr() const noexcept {
        return reinterpret_cast<const char*>(_Text.GetInternalPtr(CastN(size_t, this->_nOffset)));
    }
    char get_CursorChar() const noexcept {
        const char* p = get_CursorPtr();
        if (p == nullptr) return '\0';
        return *p;
    }
    bool isEOF() const noexcept {
        return get_CursorChar() == '\0';
    }

    void IncToks(StrLen_t nLen = 1) {
        // Advance the cursor x.
        // Skip over some known token. It is not a new line. It has no tabs. It is not past the end of the data.
#ifdef _DEBUG
        ASSERT(isValidPos());
        const char ch = get_CursorChar();
        ASSERT(!StrChar::IsSpaceX(ch) && ch != '\0');
#endif
        IncOffset(nLen);  // eat chars
    }
    void IncTab(StrLen_t nLenChar = 1) noexcept {
        // Skip to next tab stop
        _nOffset += nLenChar;  // eat tab
        if (_nTabSize <= 0)
            _nColNum++;
        else
            _nColNum = (_nColNum / _nTabSize + 1) * _nTabSize;
    }
    StrLen_t IncLineCR(StrLen_t nLenChar = 1) noexcept {
        // Check for \r\n sequence, and treat this as a single character
        IncLine(nLenChar);  // bump down to the next line
        StrLen_t nLen = 1;
        if (get_CursorChar() == '\n') {
            _nOffset += nLenChar;  // eat combo char
            nLen++;
        }
        return nLen;
    }

    void ResetSpan(const cSpan<char>& span) {
        _Text.SetSpan(span);
        InitTop();
    }
};
}  // namespace Gray
#endif
