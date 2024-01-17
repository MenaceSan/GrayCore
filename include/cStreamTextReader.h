//
//! @file cStreamTextReader.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cStreamTextReader_H
#define _INC_cStreamTextReader_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cStreamStack.h"
#include "cTextPos.h"

namespace Gray {
/// <summary>
/// read text lines from a buffer / stream. Similar to FILE*, cTextFile.
/// Allow control of read buffer size and line length.
/// Faster than cStreamInput::ReadStringLine() since it buffers ? maybe ?
/// m_nGrowSizeMax = max line size.
/// </summary>
class GRAYCORE_LINK cStreamTextReader : public cStreamStackInp {
    ITERATE_t m_iCurLineNum;  /// track the line number we are on currently. (0 based) (for cTextPos)
 public:
    cStreamInput& m_rInp;  /// Source stream.

 public:
    cStreamTextReader(cStreamInput& rInp, size_t nSizeLineMax) : cStreamStackInp(&rInp, nSizeLineMax), m_iCurLineNum(0), m_rInp(rInp) {
        // Max buffer size = max line length.
        this->put_AutoReadCommit(CastN(ITERATE_t, nSizeLineMax / 2));  // default = half buffer.
    }

 protected:
    HRESULT ReadX(void* pData, size_t nDataSize) noexcept override {
        // Use ReadStringLine instead. Prevent use of this.
        ASSERT(0);
        UNREFERENCED_PARAMETER(pData);
        UNREFERENCED_PARAMETER(nDataSize);
        return E_NOTIMPL;
    }
    HRESULT WriteX(const void* pData, size_t nDataSize) override {
        // Read ONLY. Prevent use of this.
        ASSERT(0);
        UNREFERENCED_PARAMETER(pData);
        UNREFERENCED_PARAMETER(nDataSize);
        return E_NOTIMPL;
    }

 public:
    inline ITERATE_t get_CurrentLineNumber() const noexcept {
        return m_iCurLineNum;
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

    HRESULT ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) override;

    HRESULT SeekX(STREAM_OFFSET_t iOffset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept override;
};
}  // namespace Gray
#endif
