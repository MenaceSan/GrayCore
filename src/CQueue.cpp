//
//! @file cQueue.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cLogMgr.h"
#include "cQueue.h"
#include "cQueueChunked.h"
#include "cUnitTest.h"

namespace Gray {
HRESULT cQueueIndex::SeekQ(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin) noexcept { // support virtual
    switch (eSeekOrigin & SEEK_MASK) {
        default:
        case SEEK_Set:  // FILE_BEGIN
            m_nReadLast = CastN(ITERATE_t,iOffset);
            break;
        case SEEK_Cur:  // advance the read. FILE_CURRENT
            m_nReadLast += CastN(ITERATE_t,iOffset);
            break;
        case SEEK_End:  // FILE_END
            m_nReadLast = CastN(ITERATE_t,m_nWriteLast - iOffset);
            break;
    }
    if (m_nReadLast < 0) {  // seek before start.
        // FAILURE!! before start.
        m_nReadLast = 0;
        return HRESULT_WIN32_C(ERROR_EMPTY);
    }
    if (m_nReadLast > m_nWriteLast) {
        // FAILURE!! past end
        m_nReadLast = m_nWriteLast;
        return HRESULT_WIN32_C(ERROR_DATABASE_FULL);
    }
    return CastN(HRESULT,m_nReadLast);
}

#ifndef GRAY_STATICLIB                          // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cQueueRead<char>;  // Force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cQueueRW<char>;
template class GRAYCORE_LINK cQueueStatic<char, 512>;  // Force implementation/instantiate for DLL/SO.
#endif
}  // namespace Gray
