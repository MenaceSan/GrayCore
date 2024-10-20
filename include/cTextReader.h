//! @file cTextReader.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cTextReader_H
#define _INC_cTextReader_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "ITextWriter.h"
#include "cStreamStack.h"
#include "cTextPos.h"

namespace Gray {
/// <summary>
/// read text lines from a buffer / stream. Similar to FILE*, cTextFile.
/// Allow control of read buffer size and line length.
/// Faster than cStreamInput::ReadStringLine() since it buffers ? maybe ?
/// _nGrowSizeMax = max line size.
/// opposite of ITextWriter.
/// </summary>
class GRAYCORE_LINK cTextReaderStream : public cStreamStackInp {
    ITERATE_t _iLineNumCur = 0;  /// track the line number we are on currently in _rInp. (0 based) (for cTextPos._nLineNum)
 public:
    cStreamInput& _rInp;  /// Source stream.

 public:
    cTextReaderStream(cStreamInput& rInp, size_t nSizeLineMax) : cStreamStackInp(&rInp, nSizeLineMax), _rInp(rInp) {
        // Max buffer size = max line length.
        this->put_AutoReadCommit(CastN(ITERATE_t, nSizeLineMax / 2));  // default = half buffer.
    }

 protected:
    HRESULT ReadX(cMemSpan ret) noexcept override {
        // Use ReadStringLine instead. Prevent use of this.
        ASSERT(0);
        UNREFERENCED_PARAMETER(ret);
        return E_NOTIMPL;
    }
    HRESULT WriteX(const cMemSpan& m) override {
        // Read ONLY. Prevent use of this.
        ASSERT(0);
        UNREFERENCED_PARAMETER(m);
        return E_NOTIMPL;
    }

 public:
    inline ITERATE_t get_CurrentLineNumber() const noexcept {
        return _iLineNumCur;
    }

    /// <summary>
    /// Read a line of text at a time. like fgets().
    /// Read up until (including) newline character = \n = The newline character, if read, is included in the string.
    /// like .NET StreamReader.ReadLine
    /// </summary>
    /// <param name="ppszLine"></param>
    /// <returns>length of the string read in chars. (includes \r\n) (not including null).
    /// 0 = EOF (legit end of file).
    /// -lt- 0 = other error.</returns>
    HRESULT ReadStringLine(OUT const char** ppszLine);

    HRESULT ReadStringLine(cSpanX<char> ret) override;

    HRESULT SeekX(STREAM_OFFSET_t iOffset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept override;
};
}  // namespace Gray
#endif
