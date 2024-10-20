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
            _nReadIndex = CastN(ITERATE_t, iOffset);
            break;
        case SEEK_t::_Cur:  // advance the read. FILE_CURRENT
            _nReadIndex += CastN(ITERATE_t, iOffset);
            break;
        case SEEK_t::_End:  // FILE_END
            _nReadIndex = CastN(ITERATE_t, _nWriteIndex - iOffset);
            break;
    }
    if (_nReadIndex < 0) {  // seek before start.
        // FAILURE!! before start.
        _nReadIndex = 0;
        return HRESULT_WIN32_C(ERROR_EMPTY);
    }
    if (_nReadIndex > _nWriteIndex) {
        // FAILURE!! past end
        _nReadIndex = _nWriteIndex;
        return HRESULT_WIN32_C(ERROR_DATABASE_FULL);
    }
    return CastN(HRESULT, _nReadIndex);
}

#ifndef GRAY_STATICLIB                           // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cQueueRead<char>;  // Force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cQueueRW<char>;
template class GRAYCORE_LINK cQueueStatic<512, char>;  // Force implementation/instantiate for DLL/SO.
#endif
}  // namespace Gray
