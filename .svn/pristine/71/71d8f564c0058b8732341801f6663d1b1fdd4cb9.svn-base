//
//! @file CStream.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CStream.h"
#include "CHeap.h"
#include "CThreadLock.h"

namespace Gray
{
	STREAM_POS_t CStreamBase::GetPosition() const // virtual
	{
		//! Get current read position.
		//! default implementation. If Seek() is not overridden.
		CStreamBase* pThis = const_cast<CStreamBase*>(this);
		return (STREAM_POS_t)(pThis->Seek(0, SEEK_Cur));
	}

	STREAM_POS_t CStreamBase::GetLength() const // virtual
	{
		//! default implementation. override this for better implementation.
		//! @return total length of the stream in bytes. if available. not the same as Read Length.

		CStreamBase* pThis = const_cast<CStreamBase*>(this);
		STREAM_POS_t nCurrent = (STREAM_POS_t)pThis->Seek(0, SEEK_Cur);	// save current position.
		STREAM_POS_t nLength = (STREAM_POS_t)pThis->Seek(0, SEEK_End);		// seek to the end to find the length.
		pThis->Seek((STREAM_OFFSET_t)nCurrent, SEEK_Set);		// restore the position pointer back.
		return nLength;
	}

	//*************************************************************************

	HRESULT CStreamOutput::WriteStream(CStreamInput& stmIn, STREAM_POS_t nSizeMax, IStreamProgressCallback* pProgress, TIMESYSD_t nTimeout)
	{
		//! Copy data from one read stream (stmIn) to another write stream (this). 
		//! @arg nSizeMax = Length of file or some arbitrary max to the stream size.
		//! like the IStream::CopyTo() or MFC CopyFrom()
		//! @return Size of data moved.

		CTimeSys tStart(CTimeSys::GetTimeNow());
		CHeapBlock Data(CStream::k_FILE_BLOCK_SIZE);	// temporary buffer.
		STREAM_POS_t dwAmount = 0;
		for (; dwAmount < nSizeMax;)
		{
			size_t nSizeBlock = CStream::k_FILE_BLOCK_SIZE;
			if (dwAmount + nSizeBlock > nSizeMax)
			{
				nSizeBlock = nSizeMax - dwAmount;
			}
			
			HRESULT hResRead = stmIn.ReadX(Data.get_Data(), nSizeBlock);
			if (FAILED(hResRead))
			{
				if (hResRead == HRESULT_WIN32_C(ERROR_HANDLE_EOF))	// legit end of file. Done.
				{
					break;
				}
				return hResRead;
			}
			if (hResRead == 0)
			{
				// Nothing to read at the moment.
			do_inputdry:
				if (nTimeout > 0)
				{
					if (tStart.get_AgeSys() <= nTimeout) // wait for more.
					{
						CThreadId::SleepCurrent(1);
						continue;
					}
				}
				break; // legit end of file. Done.
			}

			ASSERT((size_t)hResRead <= nSizeBlock);
			HRESULT hResWrite = this->WriteX(Data.get_Data(), hResRead);
			if (FAILED(hResWrite))
			{
				return hResWrite;
			}
			if (hResWrite != hResRead)	// couldn't write it all!
			{
				return HRESULT_WIN32_C(ERROR_WRITE_FAULT);
			}
			if (hResRead < (HRESULT)nSizeBlock)	// must just be the end of input stream.
			{
				// partial block. at end?
				dwAmount += hResRead;
				goto do_inputdry;
			}

			// full block.
			dwAmount += nSizeBlock;
			if (pProgress != nullptr)
			{
				HRESULT hRes = pProgress->onProgressCallback(CStreamProgress(dwAmount, nSizeMax));
				if (hRes != S_OK)
				{
					return hRes;
				}
			}
			if (nTimeout > 0 && tStart.get_AgeSys() > nTimeout)
			{
				return HRESULT_WIN32_C(ERROR_TIMEOUT);
			}
		}

		return (HRESULT)dwAmount;	// done.
	}

	HRESULT CStreamOutput::WriteSize(size_t nSize)
	{
		//! Write a packed (unsigned) size_t. opposite of ReadSize(nSize)
		//! Packed low to high values.
		//! Bit 7 indicates more data needed.
		//! similar to ASN1 Length packing.
		//! @return <0 = error.
		// ASSERT( nSize>=0 );
		BYTE bSize;
		while (nSize >= k_SIZE_MASK)
		{
			bSize = (BYTE)((nSize &~k_SIZE_MASK) | k_SIZE_MASK);
			HRESULT hRes = WriteT(&bSize, 1);
			if (FAILED(hRes))
				return hRes;
			nSize >>= 7;
		}
		bSize = (BYTE)nSize;
		return WriteT(&bSize, 1);	// end of dynamic range.
	}

	//*************************************************************************

	HRESULT CStreamInput::ReadSize(OUT size_t& nSize)
	{
		//! Packed low to high values.
		//! Read a packed (variable length) unsigned size. <0 = error;
		//! Bit 7 reserved to indicate more bytes to come.
		//! opposite of WriteSize( size_t )
		//! similar to ASN1 Length packing.
		//! @return HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = no more data.
		nSize = 0;
		unsigned nBits = 0;
		for (;;)
		{
			BYTE bSize; // read 1 byte at a time.
			HRESULT hRes = ReadT(&bSize, sizeof(bSize));
			if (FAILED(hRes))
				return hRes;
			nSize |= ((size_t)(bSize &~k_SIZE_MASK)) << nBits;
			if (!(bSize & k_SIZE_MASK))	// end marker.
				break;
			nBits += 7;
			ASSERT(nBits <= 80);
		}
		return (HRESULT)nBits;
	}

	HRESULT CStreamInput::ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) // virtual
	{
		//! Read a string up until (including) a "\n" or "\r\n". end of line. FILE_EOL.
		//! Some streams can support this better than others. like fgets(FILE*)
		//! @arg iSizeMax = Maximum number of characters to be copied into pszBuffer (including the terminating null-character).
		//! @return
		//!  >= 0 = size of the string in characters not bytes. NOT pointer to pBuffer like fgets()
		//!  HRESULT < 0 = error. HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) = not full line.

		iSizeMax--;
		StrLen_t i = 0;
		for (;;)
		{
			if (i >= iSizeMax)
			{
				return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);	// overrun ERROR_INVALID_DATA
			}
			char ch = '\0';
			HRESULT hRes = this->ReadT<char>(ch);
			if (FAILED(hRes))
			{
				pszBuffer[i] = '\0';
				return hRes;	// HRESULT_WIN32_C(ERROR_IO_INCOMPLETE) is ok.
			}
			pszBuffer[i++] = ch;
			if (ch == '\n')		// "\n" or "\r\n"
				break;
		}
		pszBuffer[i] = '\0';
		return i;
	}

	HRESULT CStreamInput::ReadStringLine(OUT wchar_t* pszBuffer, StrLen_t iSizeMax) // virtual
	{
		//! Read a string up until (including) a "\n" or "\r\n". end of line. FILE_EOL.
		//! Some streams can support this better than others. like fgets(FILE*).
		//! @arg iSizeMax = Maximum number of characters to be copied into str (including the terminating null-character).
		//! @return
		//!  >= 0 = size of the string in characters not bytes. NOT pointer to pBuffer like fgets()
		//!  HRESULT < 0 = error.

		iSizeMax--;
		StrLen_t i = 0;
		for (;;)
		{
			if (i >= iSizeMax)
				return HRESULT_WIN32_C(RPC_S_STRING_TOO_LONG);	// overrun ERROR_INVALID_DATA
			wchar_t ch = '\0';
			HRESULT hRes = this->ReadT<wchar_t>(ch);
			if (FAILED(hRes))
			{
				return hRes;
			}
			pszBuffer[i++] = ch;
			if (ch == '\n')
				break;
		}
		pszBuffer[i] = '\0';
		return i;
	}

	HRESULT CStreamInput::ReadPeek(void* pData, size_t nDataSize) // virtual
	{
		//! Peek ahead in the stream if possible. Non blocking.
		//! just try to read data but not remove from the queue.
		//! @return Amount peeked.

		HRESULT hResRead = ReadX(pData, nDataSize);
		if (FAILED(hResRead) || hResRead == 0)
		{
			return hResRead;
		}
		STREAM_SEEKRET_t nRetSeek = Seek(-hResRead, SEEK_Cur);	// back up.
		if (nRetSeek < 0)
		{
			return (HRESULT)nRetSeek;
		}
		return hResRead;
	}
}

//*************************************************************************
#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CStreamQueue.h"
#include "CLogMgr.h"
#include "CRandomDef.h"
#include "CTypes.h"

void CStream::UnitTest_StreamIntegrity(CStreamOutput& stmOut, CStreamInput& stmIn, size_t nSizeTotal)
{
	// Write to streams in random block sizes and make sure i read the same back.
	// @arg nSizeTotal = How much to write/test total ?

	size_t iSizeBlock = g_Rand.GetRandUX(1024) + 100;	// TODO Make random range bigger !! 2k ?
	CHeapBlock blockwrite(iSizeBlock * 2);
	g_Rand.GetNoise(blockwrite, iSizeBlock);
	::memcpy(blockwrite.get_DataBytes() + iSizeBlock, blockwrite.get_DataBytes(), iSizeBlock);	// double it.

	size_t iSizeWriteTotal = 0;
	size_t iSizeReadTotal = 0;

	HRESULT hRes;
	CHeapBlock blockread(iSizeBlock);
	CTimeSys tStart = CTimeSys::GetTimeNow();
	size_t nSizeReal;

	int i = 0;
	for (;; i++)
	{
		UNITTEST_TRUE(iSizeReadTotal <= iSizeWriteTotal);

		if (iSizeWriteTotal < nSizeTotal)	// write more?
		{
			size_t iSizeWriteBlock = g_Rand.GetRandUX((UINT)(iSizeBlock - 1)) + 1;
			if (iSizeWriteTotal + iSizeWriteBlock > nSizeTotal)
				iSizeWriteBlock = nSizeTotal - iSizeWriteTotal;
			UNITTEST_TRUE(iSizeWriteBlock <= iSizeBlock);
			hRes = stmOut.WriteX(blockwrite.get_DataBytes() + (iSizeWriteTotal%iSizeBlock), iSizeWriteBlock);
			UNITTEST_TRUE(SUCCEEDED(hRes));
			nSizeReal = (size_t)hRes;
			UNITTEST_TRUE(nSizeReal <= iSizeWriteBlock);
			iSizeWriteTotal += nSizeReal;
			UNITTEST_TRUE(iSizeWriteTotal <= nSizeTotal);
		}

		UNITTEST_TRUE(iSizeReadTotal <= iSizeWriteTotal);

		size_t iSizeReadBlock = g_Rand.GetRandUX((UINT)(iSizeBlock - 1)) + 1;
		UNITTEST_TRUE(iSizeReadBlock <= iSizeBlock);
		BYTE* pRead = blockread.get_DataBytes();
		hRes = stmIn.ReadX(pRead, iSizeReadBlock);
		UNITTEST_TRUE(SUCCEEDED(hRes));
		nSizeReal = (size_t)hRes;
		UNITTEST_TRUE(nSizeReal <= iSizeReadBlock);

		// Make sure i read correctly.
		const BYTE* pWrite = blockwrite.get_DataBytes() + (iSizeReadTotal%iSizeBlock);
		COMPARE_t iRet = CMem::Compare(pWrite, pRead, nSizeReal);
		UNITTEST_TRUE(iRet == 0);
		iSizeReadTotal += nSizeReal;
		UNITTEST_TRUE(iSizeReadTotal <= iSizeWriteTotal);

		if (iSizeReadTotal >= nSizeTotal)	// done?
			break;
		if (!CUnitTests::IsTestInteractive() && tStart.get_AgeSec() > 100)
		{
			UNITTEST_TRUE(false);
			return;
		}
	}
}

UNITTEST_CLASS(CStream)
{
	void UnitTest_StreamSize(CStream& q)
	{
		//! Write size to a stream and read back.

		ITERATE_t iIterations = 0;
		size_t iInc = 1;
		for (size_t i = 0; i < CTypeLimit<DWORD>::k_Max && iIterations <= 64; i += iInc, iInc *= 2, iIterations++)
		{
			HRESULT hRes = q.WriteSize(i);
			UNITTEST_TRUE(SUCCEEDED(hRes));
		}

		STREAM_SEEKRET_t nRetSeek = q.GetPosition();
		UNITTEST_TRUE(nRetSeek == 0);
		UNITTEST_TRUE(q.GetLength() == 86);

		ITERATE_t iIterations2 = 0;
		iInc = 1;
		for (size_t i = 0; i < CTypeLimit<DWORD>::k_Max && iIterations2 <= 64; i += iInc, iInc *= 2, iIterations2++)
		{
			size_t nSizeRead;
			HRESULT hRes = q.ReadSize(nSizeRead);
			UNITTEST_TRUE(SUCCEEDED(hRes));
			UNITTEST_TRUE(i == nSizeRead);
		}

		nRetSeek = q.GetPosition();
		UNITTEST_TRUE(nRetSeek == 86);
		UNITTEST_TRUE(iIterations == iIterations2);
	}

	UNITTEST_METHOD(CStream)
	{
		//! ReadSize, WriteSize()
		CStreamQueue q;
		UnitTest_StreamSize(q);
		UNITTEST_TRUE(q.isEmptyQ());	// all read back.
		UNITTEST_TRUE(q.get_ReadQty() == 0);

		CStream::UnitTest_StreamIntegrity(q, q, 10000 + g_Rand.GetRandUX(500000));
		UNITTEST_TRUE(q.isEmptyQ());	// all read back.
		UNITTEST_TRUE(q.get_ReadQty() == 0);

	}
};
UNITTEST_REGISTER(CStream, UNITTEST_LEVEL_Core);
#endif
