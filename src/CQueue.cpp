//
//! @file cQueue.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cQueue.h"
#include "cUnitTest.h"
#include "cLogMgr.h"

namespace Gray
{
	STREAM_SEEKRET_t cQueueBase::SeekQ(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin)	// support virtual
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
			// FAILURE!!
			m_iReadLast = 0;
			return (STREAM_SEEKRET_t)HRESULT_WIN32_C(ERROR_EMPTY);
		}
		if (m_iReadLast > m_iWriteLast)
		{
			// FAILURE!!
			m_iReadLast = m_iWriteLast;
			return (STREAM_SEEKRET_t)HRESULT_WIN32_C(ERROR_DATABASE_FULL);
		}
		return m_iReadLast;
	}
}

//******************************************************************

#if USE_UNITTESTS
UNITTEST_CLASS(cQueueBase)
{
	const GChar_t* UnitTest_GetSrc(ITERATE_t i, ITERATE_t& riSrcLenMax)
	{
		//! k_sTextBlob = cUnitTests::k_TEXTBLOB_LEN
		i %= cUnitTests::k_TEXTBLOB_LEN;
		riSrcLenMax = cUnitTests::k_TEXTBLOB_LEN - i;
		return static_cast<const GChar_t*>(k_sTextBlob) + i;
	}
	UNITTEST_METHOD(cQueueBase)
	{
		// Read test cQueueRead
		cQueueRead<GChar_t> qr(k_sTextBlob, 0, cUnitTests::k_TEXTBLOB_LEN);

		for (StrLen_t i = 0; i < cUnitTests::k_TEXTBLOB_LEN; i++)
		{
			GChar_t szTmp[2];
			qr.ReadQty(szTmp, _countof(szTmp)); // STRMAX
		}

		// Read/Write test. cQueueBytes
		cQueueBytes qb;
		for (StrLen_t i = 0; i < cUnitTests::k_TEXTBLOB_LEN; i++)
		{
		}

		const int UNITTEST_CHUNK = 10;

		ITERATE_t iSrcLenMax;

		//! Read/Write a bunch of stuff to cQueueStatic
		cQueueStatic<GChar_t, 512> qs;
		int k = 0;
		for (;;)
		{
			const GChar_t* pSrc = UnitTest_GetSrc(k, iSrcLenMax);
			int iPut = qs.WriteQty(pSrc, MIN(UNITTEST_CHUNK, iSrcLenMax));
			if (iPut <= 0)
				break;
			k += iPut;
		}
		UNITTEST_TRUE(qs.isFullQ());
		ITERATE_t j = 0;
		for (;;)
		{
			const GChar_t* pSrc = UnitTest_GetSrc(j, iSrcLenMax);
			GChar_t junk[UNITTEST_CHUNK];
			ITERATE_t iGot = qs.ReadQty(junk, MIN(iSrcLenMax, (int)_countof(junk)));
			if (iGot <= 0)
				break;
			UNITTEST_TRUE(!memcmp(pSrc, junk, iGot));
			j += iGot;
		}
		UNITTEST_TRUE(k == j);
		UNITTEST_TRUE(qs.isEmptyQ());

		// Read/Write a bunch of stuff to cQueueChunked
		cQueueChunked<GChar_t> qc(512);
		for (k = 0; k < 10000;)
		{
			const GChar_t* pSrc = UnitTest_GetSrc(k, iSrcLenMax);
			int iPut = qc.WriteQty(pSrc, MIN(UNITTEST_CHUNK, iSrcLenMax));
			k += iPut;
		}
		for (j = 0;;)
		{
			const GChar_t* pSrc = UnitTest_GetSrc(j, iSrcLenMax);
			GChar_t junk[UNITTEST_CHUNK];
			ITERATE_t iGot = qc.ReadQty(junk, MIN(iSrcLenMax, (int)_countof(junk)));
			if (iGot <= 0)
				break;
			UNITTEST_TRUE(!memcmp(pSrc, junk, iGot));
			j += iGot;
		}

		UNITTEST_TRUE(k == j);
		UNITTEST_TRUE(qc.isEmptyQ());
	}
};
UNITTEST_REGISTER(cQueueBase, UNITTEST_LEVEL_Core);
#endif
