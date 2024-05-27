//! @file cStreamStack.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cStreamStack.h"

namespace Gray {
HRESULT cStreamStackInp::ReadFill() {
    //! Fill to a cStreamQueue / cQueueBytes / cQueueRW from an upstream input source. m_pInp
    //! called by ReadX()
    //! @return how much i got.

    cMemSpan spanWrite = GetSpanWrite(this->get_GrowSizeChunk());
    if (spanWrite.isEmpty()) return 0;       // no room.
    if (m_pInp == nullptr) return E_HANDLE;  // must supply input stream

    // Read all i have room for.
    const HRESULT hRes = m_pInp->ReadX(spanWrite);
    if (FAILED(hRes)) return hRes;

    if (hRes == 0) return 0;  // return HRESULT_WIN32_C(ERROR_HANDLE_EOF);

    AdvanceWrite(CastN(ITERATE_t, hRes));
    return hRes;
}

HRESULT cStreamStackInp::ReadFillAligned(size_t nSizeBlockAlign) {
    //! Fill to a cStreamQueue / cQueueBytes / cQueueRW from an upstream input source. m_pInp
    //! @arg nSizeBlockAlign = I must get at least this amount. else get nothing.
    //! called by ReadX()
    //! @return how much i got.

    if (nSizeBlockAlign <= 1) return ReadFill();  // simple case.

    cMemSpan spanWrite = GetSpanWrite(this->get_GrowSizeChunk());
    if (spanWrite.isEmpty()) return 0;  // no room.

    size_t nWriteQty = spanWrite.get_SizeBytes();
    if (nWriteQty < nSizeBlockAlign) return 0;  // no room.

    nWriteQty -= nWriteQty % nSizeBlockAlign;  // Remove remainder.

    if (m_pInp == nullptr) return E_HANDLE;

    cStreamTransaction trans(m_pInp);
    HRESULT hRes = m_pInp->ReadX(cMemSpan(spanWrite.GetTPtrW(), nWriteQty));
    if (FAILED(hRes)) {
        trans.SetTransactionFailed();
        return hRes;
    }

    // NOTE Make sure the actual amount read is nSizeBlockAlign aligned. UN-read anything that is not aligned.
    ITERATE_t iWriteActual = (ITERATE_t)hRes;
    if (iWriteActual == 0) {  // Got no data.
        trans.SetTransactionComplete();
        return S_OK;
    }

    // Only get data on block alignments.
    const ITERATE_t nSizeOver = iWriteActual % nSizeBlockAlign;
    if (nSizeOver > 0) {  // must back out the unaligned part.
        hRes = m_pInp->SeekX(-nSizeOver, SEEK_t::_Cur);
        if (FAILED(hRes)) {
            trans.SetTransactionFailed();  // roll back.
            return hRes;
        }
        iWriteActual -= nSizeOver;
        DEBUG_CHECK(trans.m_lPosStart + iWriteActual == m_pInp->GetPosition());
    }

    trans.SetTransactionComplete();
    AdvanceWrite(iWriteActual);
    return CastN(HRESULT, iWriteActual);
}

//***********************************

HRESULT cStreamStackOut::WriteFlush() {  // virtual
    //! Push the compressed data from m_buffer cQueueRW out to a down stream/file. m_pStreamOut
    //! called by WriteX()

    const ITERATE_t iReadQty = get_ReadQty();
    if (iReadQty <= 0) return 0;
    if (m_pStreamOut == nullptr) return 0;  // just let the data sit in the buffer until read some other way ?

    const HRESULT hRes = m_pStreamOut->WriteX(get_SpanRead());
    if (SUCCEEDED(hRes) && hRes > 0) {
        // m_nStreamTotal += hRes;
        AdvanceRead(hRes);
        ReadCommitCheck();
    }
    return hRes;
}

HRESULT cStreamStackPackets::WriteX(const cMemSpan& m) {  // virtual
    //! Take all pData written to me and store in m_buffer.
    //! @arg pData = nullptr = just test if it has enough room.

    const ITERATE_t nWriteQty = this->WriteSpanQ(cSpan<BYTE>(m), true);
    if ((size_t)nWriteQty < m.get_SizeBytes()) {
        return HRESULT_WIN32_C(WSAEWOULDBLOCK);  // Just wait for the full packet.
    }

    // Push all the data from the m_buffer to m_pStreamOut
    for (;;) {
        const HRESULT hRes = this->WriteFlush();
        if (FAILED(hRes)) return hRes;
        if (hRes == 0) break;  // no full packet could be written.
    }

    return CastN(HRESULT, nWriteQty);
}
}  // namespace Gray
