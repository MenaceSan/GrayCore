//! @file cStream.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrT.h"
#include "cStack.h"  // include this some palce just to compile.
#include "cStream.h"
#include "cThreadLock.h"
#include "cValT.h"

namespace Gray {

cStreamTransaction ::cStreamTransaction(cStreamInput* pInp) : cStreamReader(pInp) {
    ASSERT_NN(m_pInp);
    m_lPosStart = m_pInp->GetPosition();
    if (CastN(UINT_PTR, m_lPosStart) > CastN(STREAM_POS_t, cMem::k_ALLOC_MAX)) {
        m_lPosStart = k_STREAM_POS_ERR;  // Rollback not allowed!
        return;
    }
    m_nSeekSizeMinPrev = m_pInp->SetReadCommitSize(0);  // Don't use AutoReadCommit inside cStreamTransaction.
    ASSERT(m_nSeekSizeMinPrev >= 0 && m_nSeekSizeMinPrev <= cMem::k_ALLOC_MAX);
    ASSERT(isTransactionActive());
}
cStreamTransaction ::~cStreamTransaction() {
    //! if we didn't say it was a success, do a rollback on destruct.
    if (m_pInp == nullptr) return;
    if (isTransactionActive()) {  // We failed ! didn't call SetTransactionComplete or SetTransactionFailed()
        TransactionRollback();
    }
    // Restore commit ability
    m_pInp->SetReadCommitSize(m_nSeekSizeMinPrev);  // Complete. we can now commit reads. e.g. toss data we have declared read.
}
HRESULT cStreamTransaction ::TransactionRollback() {
    // Roll back to m_lPosStart
    const STREAM_POS_t lPosCur = m_pInp->GetPosition();
    if (lPosCur == m_lPosStart) return S_OK;  // SetTransactionPartial was full success.
    return m_pInp->SeekX(CastN(STREAM_OFFSET_t, m_lPosStart), SEEK_t::_Set);
}

STREAM_POS_t cStreamBase::GetLength() const noexcept {  // virtual // default impl
    cStreamBase* pThis = const_cast<cStreamBase*>(this);
    const STREAM_POS_t nCurrent = GetPosition();   // save current position.
    HRESULT hRes = pThis->SeekX(0, SEEK_t::_End);  // seek to the end to find the length.
    if (FAILED(hRes)) return k_STREAM_POS_ERR;
    const STREAM_POS_t nLength = GetPosition();                           // find end.
    hRes = pThis->SeekX(CastN(STREAM_OFFSET_t, nCurrent), SEEK_t::_Set);  // restore the position pointer back.
    if (FAILED(hRes)) return k_STREAM_POS_ERR;
    return nLength;
}

//*************************************************************************

HRESULT cStreamOutput::WriteString(const char* pszStr) {  // virtual
    return WriteSpan(StrT::ToSpanStr(pszStr));
}
HRESULT cStreamOutput::WriteString(const wchar_t* pszStr) {  //  virtual
    const HRESULT hRes = WriteSpan(StrT::ToSpanStr(pszStr));
    if (FAILED(hRes)) return hRes;
    return hRes / sizeof(wchar_t);
}

HRESULT cStreamOutput::WriteStream(cStreamInput& stmIn, STREAM_POS_t nSizeMax, IStreamProgressCallback* pProgress, TIMESYSD_t nTimeout) {
    const cTimeSys tStart(cTimeSys::GetTimeNow());
    cBlob blob(cStream::k_FILE_BLOCK_SIZE);  // temporary buffer.
    STREAM_POS_t dwAmount = 0;               // how much written so far.
    for (; dwAmount < nSizeMax;) {
        const size_t nSizeBlock = cValT::Min<size_t>(cStream::k_FILE_BLOCK_SIZE, nSizeMax - dwAmount);
        const HRESULT hResRead = stmIn.ReadX(cMemSpan(blob, nSizeBlock));
        if (FAILED(hResRead)) {
            if (hResRead == HRESULT_WIN32_C(ERROR_HANDLE_EOF)) break;  // legit end of file. Done.
            return hResRead;
        }
        if (hResRead == 0) {  // Nothing to read at the moment.
            if (nTimeout > 0) {
                if (tStart.get_AgeSys() >= nTimeout) break;  // max time exceeded. with NO data in stream. done.
                cThreadId::SleepCurrent(1);                  // wait for more.
                continue;
            }
            break;  // legit end of file. Done.
        }

        ASSERT((size_t)hResRead <= nSizeBlock);
        const HRESULT hResWrite = this->WriteSpan(cMemSpan(blob, hResRead));  // write it all or fail!
        if (FAILED(hResWrite)) return hResWrite;
        dwAmount += hResRead;  // block written.

        if (pProgress != nullptr) {
            const HRESULT hResProg = pProgress->onProgressCallback(cStreamProgress(dwAmount, nSizeMax));
            if (hResProg != S_OK) return hResProg;  // cancel?
        }
        if (nTimeout > 0) {
            if (tStart.get_AgeSys() >= nTimeout) return HRESULT_WIN32_C(ERROR_TIMEOUT);  // max time exceeded. with data in stream.
            if (hResRead < CastN(HRESULT, nSizeBlock)) cThreadId::SleepCurrent(1);       // partial block. at end of input stream? wait for more.
        }
    }

    return CastN(HRESULT, dwAmount);  // done.
}

HRESULT cStreamOutput::WriteSize(size_t nSize) {
    while (nSize >= k_SIZE_MASK) {
        const BYTE bSize = CastN(BYTE, (nSize & ~k_SIZE_MASK) | k_SIZE_MASK);  // take 7 bits.
        const HRESULT hRes = WriteT(bSize);
        if (FAILED(hRes)) return hRes;
        nSize >>= 7;
    }
    return WriteT(CastN(BYTE, nSize));  // end of dynamic range. last byte
}

//*************************************************************************

HRESULT cStreamInput::ReadSize(OUT size_t& nSize) noexcept {
    nSize = 0;
    unsigned nBits = 0;
    for (;;) {
        BYTE bSize = 0;  // read 1 byte at a time.
        const HRESULT hRes = ReadT(bSize);
        if (FAILED(hRes)) return hRes;
        nSize |= CastN(size_t, bSize & ~k_SIZE_MASK) << nBits;
        if (!(bSize & k_SIZE_MASK)) break;  // end marker.
        nBits += 7;
        ASSERT(nBits <= 80);
    }
    return CastN(HRESULT, nBits);
}

HRESULT cStreamInput::ReadAll(OUT cBlob& blob, size_t nSizeExtra) {
    const STREAM_POS_t nLengthStream = this->GetLength();
    const size_t nLengthRead = CastN(size_t, nLengthStream);
    const size_t nLengthAlloc = nLengthRead + nSizeExtra;
    if (nLengthAlloc == 0) return S_OK;  // do nothing.

    if (!blob.AllocSize(nLengthAlloc)) return E_OUTOFMEMORY;
    const HRESULT hRes = ReadSpan(cMemSpan(blob, nLengthRead));  // must get all.
    if (FAILED(hRes)) return hRes;
    // Zero the extra (if any);
    if (nSizeExtra > 0) {
        blob.GetTPtrW<BYTE>()[hRes] = '\0';  // terminator as default. I want to use the blob as a c string.
    }
    return hRes;
}

HRESULT cStreamInput::ReadStringLine(cSpanX<char> ret) {  // virtual
    //! Read a string up until (including) a "\n" or "\r\n". end of line. FILE_EOL.
    //! Some streams can support this better than others. like fgets(FILE*)
    //! @arg iSizeMax = Maximum number of characters to be copied into pszBuffer (including room for the the terminating '\0' character).
    //! @return
    //!  >= 0 = size of the string in characters not bytes. NOT pointer to pBuffer like fgets()
    //!  HRESULT < 0 = error. HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = not full line.

    const StrLen_t iSizeMax = ret.get_MaxLen() - 1;
    char* pWrite = ret.get_PtrWork();
    if (pWrite == nullptr) return E_POINTER;
    StrLen_t i = 0;
    for (;;) {
        if (i >= iSizeMax) return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);  // overrun ERROR_INVALID_DATA
        const HRESULT hRes = this->ReadSpan(cMemSpan(pWrite + i, 1));      // read one character at a time.
        if (FAILED(hRes)) {
            pWrite[i] = '\0';
            return hRes;  // HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) is ok.
        }
        if (pWrite[i++] == '\n') break;  // "\n" or "\r\n"
    }
    pWrite[i] = '\0';
    return i;
}

HRESULT cStreamInput::ReadStringLine(cSpanX<wchar_t> ret) {  // virtual
    //! Read a string up until (including) a "\n" or "\r\n". end of line. FILE_EOL.
    //! Some streams can support this better than others. like fgets(FILE*).
    //! @arg iSizeMax = Maximum number of characters to be copied into str (including the terminating null-character).
    //! @return
    //!  >= 0 = size of the string in characters not bytes. NOT pointer to pBuffer like fgets()
    //!  HRESULT < 0 = error.

    const StrLen_t iSizeMax = ret.get_MaxLen() - 1;
    wchar_t* pWrite = ret.get_PtrWork();
    if (pWrite == nullptr) return E_POINTER;
    StrLen_t i = 0;
    for (;;) {
        if (i >= iSizeMax) return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);  // overrun ERROR_INVALID_DATA
        const HRESULT hRes = this->ReadSpan(cMemSpan(pWrite + i, 1));      // read one character at a time.
        if (FAILED(hRes)) {
            pWrite[i] = '\0';
            return hRes;
        }
        if (pWrite[i++] == '\n') break;  // "\n" or "\r\n"
    }
    pWrite[i] = '\0';
    return i;
}

HRESULT cStreamInput::ReadPeek(cMemSpan ret) noexcept {  // virtual
    //! Peek ahead in the stream if possible. Non blocking.
    //! just try to read data but not remove from the queue.
    //! @return Amount peeked.

    const HRESULT hResRead = ReadX(ret);
    if (FAILED(hResRead) || hResRead == 0) return hResRead;

    const HRESULT hResSeek = SeekX(-hResRead, SEEK_t::_Cur);  // back up.
    if (FAILED(hResSeek)) return hResSeek;                    // ERROR.
    return hResRead;
}
}  // namespace Gray
