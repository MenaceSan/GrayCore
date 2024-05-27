//! @file cQueue.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cLogMgr.h"
#include "cQueue.h"
#include "cUnitTest.h"

namespace Gray {
HRESULT cQueueIndex::SeekQ(STREAM_OFFSET_t iOffset, SEEK_t eSeekOrigin) noexcept {  // support virtual
    switch (CastN(SEEK_t, CastN(BYTE, eSeekOrigin) & 0x0f)) {
        default:
        case SEEK_t::_Set:  // FILE_BEGIN
            m_nReadIndex = CastN(ITERATE_t, iOffset);
            break;
        case SEEK_t::_Cur:  // advance the read. FILE_CURRENT
            m_nReadIndex += CastN(ITERATE_t, iOffset);
            break;
        case SEEK_t::_End:  // FILE_END
            m_nReadIndex = CastN(ITERATE_t, m_nWriteIndex - iOffset);
            break;
    }
    if (m_nReadIndex < 0) {  // seek before start.
        // FAILURE!! before start.
        m_nReadIndex = 0;
        return HRESULT_WIN32_C(ERROR_EMPTY);
    }
    if (m_nReadIndex > m_nWriteIndex) {
        // FAILURE!! past end
        m_nReadIndex = m_nWriteIndex;
        return HRESULT_WIN32_C(ERROR_DATABASE_FULL);
    }
    return CastN(HRESULT, m_nReadIndex);
}

#ifndef GRAY_STATICLIB                           // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cQueueRead<char>;  // Force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cQueueRW<char>;
template class GRAYCORE_LINK cQueueStatic<512, char>;  // Force implementation/instantiate for DLL/SO.
#endif
}  // namespace Gray
