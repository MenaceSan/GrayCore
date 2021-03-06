//
//! @file cQueue.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cQueue.h"
#include "cQueueChunked.h"
#include "cUnitTest.h"
#include "cLogMgr.h"

namespace Gray
{
	HRESULT cQueueIndex::SeekQ(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin)	// support virtual
	{
		//! eSeekOrigin = SEEK_CUR, etc
		//! move the current read start location.
		//! @return
		//!  the New stream/file position,  <0=FAILED = INVALID_SET_FILE_POINTER

		switch (eSeekOrigin&SEEK_MASK)
		{
		default:
		case SEEK_Set:	// FILE_BEGIN
			m_iReadLast = (ITERATE_t)iOffset;
			break;
		case SEEK_Cur:	// advance the read. FILE_CURRENT
			m_iReadLast += (ITERATE_t)iOffset;
			break;
		case SEEK_End:	// FILE_END
			m_iReadLast = (ITERATE_t)(m_iWriteLast - iOffset);
			break;
		}
		if (m_iReadLast < 0)	// seek before start.
		{
			// FAILURE!! before start.
			m_iReadLast = 0;
			return HRESULT_WIN32_C(ERROR_EMPTY);
		}
		if (m_iReadLast > m_iWriteLast)
		{
			// FAILURE!! past end
			m_iReadLast = m_iWriteLast;
			return HRESULT_WIN32_C(ERROR_DATABASE_FULL);
		}
		return (HRESULT) m_iReadLast;
	}

#ifndef GRAY_STATICLIB // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cQueueRead<char>;		// Force Instantiation for DLL.
	template class GRAYCORE_LINK cQueueRW<char>;
	template class GRAYCORE_LINK cQueueStatic<char, 512>;	// Force Instantiation for DLL.
#endif

}
 