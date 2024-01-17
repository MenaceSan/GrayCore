//
//! @file cStreamTextReader.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cStreamTextReader.h"

namespace Gray {

HRESULT cStreamTextReader::ReadStringLine(OUT const char** ppszLine) {   
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
    m_iCurLineNum++;
    if (ppszLine != nullptr) {
        *ppszLine = pData;
    }
    return i;  // length.
}

HRESULT cStreamTextReader::ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) { // override // virtual
    //! @arg iSizeMax = Maximum number of characters to be copied into pszBuffer (including room for the the terminating '\0' character).
    //! @return
    //!  length of the string read in chars. (includes \r\n) (not including null). 0 = EOF

    if (iSizeMax <= 0) return 0;
    const char* pszLine = nullptr;
    HRESULT hRes = ReadStringLine(&pszLine);
    if (FAILED(hRes)) return hRes;
    size_t nSizeCopy = cValT::Min<size_t>(hRes, iSizeMax - 1);
    cMem::Copy(pszBuffer, pszLine, nSizeCopy);
    pszBuffer[nSizeCopy] = '\0';
    return (HRESULT)nSizeCopy;
}

HRESULT cStreamTextReader::SeekX(STREAM_OFFSET_t iOffset, SEEK_t eSeekOrigin) noexcept { // override;
    //! Seek to a particular position in the file.
    //! This will corrupt m_iCurLineNum. The caller must manage that themselves.
    //! @arg eSeekOrigin = // SEEK_SET ?
    //! @return the New position, <0=FAILED

    STREAM_POS_t nPosFile = m_rInp.GetPosition();  // WriteIndex
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
            return m_rInp.SeekX(nPosFile + iOffset - iReadQty, SEEK_t::_Set);

        case SEEK_t::_End:  // relative to end.
            if (iOffset > 0) break;
            iOffset += m_rInp.GetLength();

            // Fall through.

        case SEEK_t::_Set:  // from beginning.
            // Are we inside the current buffer? then just reposition.
            if (iOffset == 0) {
                m_iCurLineNum = 0;
            } else {
                m_iCurLineNum = -1;  // No idea.
            }
            if (iOffset >= (STREAM_OFFSET_t)(nPosFile - get_WriteIndex()) && iOffset <= (STREAM_OFFSET_t)nPosFile) {
                // Move inside buffer.
                AdvanceRead((ITERATE_t)(nPosFile - iOffset));
                return (HRESULT)iOffset;
            }
            // outside current buffer.
            this->SetEmptyQ();
            return m_rInp.SeekX(iOffset, SEEK_t::_Set);

        default:
            break;
    }

    ASSERT(0);
    return E_FAIL;
}
}  // namespace Gray
