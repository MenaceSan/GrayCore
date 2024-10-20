//! @file cTextReader.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cTextReader.h"

namespace Gray {

HRESULT cTextReaderStream::ReadStringLine(OUT const char** ppszLine) {   
    ITERATE_t iReadAvail = this->get_ReadQty();
    const char* pData = (const char*)this->get_ReadPtr();
    int i = 0;
    for (;; i++) {
        if (i >= iReadAvail) { // run out of data.
            // Try to get more.
            ReadCommitNow();  // pData may be invalidated.

            HRESULT hRes = this->ReadFill();
            if (FAILED(hRes)) return hRes;

            pData = (const char*)this->get_ReadPtr();
            if (hRes <= 0) {  // We have no more data (EOF) or no more room to read data (line is too long)
                break;
            }
            ASSERT(iReadAvail < this->get_ReadQty());
            iReadAvail = this->get_ReadQty();
        }

        // Find the '\n' EOL
        char ch = pData[i];
        if (ch == '\n') {
            i++;  // include it.
            break;
        }
    }

    AdvanceRead(i);

    // Found a line. return it.
    _iLineNumCur++;
    if (ppszLine != nullptr) {
        *ppszLine = pData;
    }
    return i;  // length.
}

HRESULT cTextReaderStream::ReadStringLine(cSpanX<char> ret) {  // override // virtual
    //! @arg iSizeMax = Maximum number of characters to be copied into pszBuffer (including room for the the terminating '\0' character).
    //! @return
    //!  length of the string read in chars. (includes \r\n) (not including null). 0 = EOF

    if (ret.isEmpty()) return 0;
    const char* pszLine = nullptr;
    HRESULT hRes = ReadStringLine(&pszLine);
    if (FAILED(hRes)) return hRes;

    const size_t nSizeCopy = cValT::Min<size_t>(hRes, ret.get_MaxLen() - 1);
    cMem::Copy(ret.get_PtrWork(), pszLine, nSizeCopy);
    ret.get_PtrWork()[nSizeCopy] = '\0';   // Just in case.
    return (HRESULT)nSizeCopy;
}

HRESULT cTextReaderStream::SeekX(STREAM_OFFSET_t iOffset, SEEK_t eSeekOrigin) noexcept {  // override;
    //! Seek to a particular position in the file.
    //! This will corrupt _iLineNumCur. The caller must manage that themselves.
    //! @arg eSeekOrigin = // SEEK_SET ?
    //! @return the New position, <0=FAILED

    STREAM_POS_t nPosFile = _rInp.GetPosition();  // WriteIndex
    ITERATE_t iReadQty = this->get_ReadQty();      // what i have buffered.

    switch (eSeekOrigin) {
        case SEEK_t::_Cur:  // relative
            if (iOffset >= -get_ReadIndex() && iOffset <= iReadQty) {
                // Move inside buffer.
                AdvanceRead((ITERATE_t)iOffset);
                return (HRESULT)(nPosFile + iOffset - iReadQty);
            }
            // outside current buffer.
            this->SetEmptyQ();
            return _rInp.SeekX(nPosFile + iOffset - iReadQty, SEEK_t::_Set);

        case SEEK_t::_End:  // relative to end.
            if (iOffset > 0) break;
            iOffset += _rInp.GetLength();

            // Fall through.

        case SEEK_t::_Set:  // from beginning.
            // Are we inside the current buffer? then just reposition.
            if (iOffset == 0) {
                _iLineNumCur = 0;
            } else {
                _iLineNumCur = -1;  // No idea.
            }
            if (iOffset >= (STREAM_OFFSET_t)(nPosFile - get_WriteIndex()) && iOffset <= (STREAM_OFFSET_t)nPosFile) {
                // Move inside buffer.
                AdvanceRead((ITERATE_t)(nPosFile - iOffset));
                return (HRESULT)iOffset;
            }
            // outside current buffer.
            this->SetEmptyQ();
            return _rInp.SeekX(iOffset, SEEK_t::_Set);

        default:
            break;
    }

    ASSERT(0);
    return E_FAIL;
}
}  // namespace Gray
