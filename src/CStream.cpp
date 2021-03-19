//
//! @file cStream.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cStream.h"
#include "cHeap.h"
#include "cThreadLock.h"

namespace Gray
{
 	STREAM_POS_t cStreamBase::GetLength() const // virtual
	{
		//! default implementation using SeekX and GetPosition. override this for better implementation.
		//! @return total length of the stream in bytes. if available. not the same as Read Length.

		cStreamBase* pThis = const_cast<cStreamBase*>(this);
		STREAM_POS_t nCurrent = GetPosition();	// save current position.
		pThis->SeekX(0, SEEK_End);		// seek to the end to find the length.
		STREAM_POS_t nLength = GetPosition();
		pThis->SeekX((STREAM_OFFSET_t)nCurrent, SEEK_Set);		// restore the position pointer back.
		return nLength;
	}

	//*************************************************************************

	HRESULT cStreamOutput::WriteStream(cStreamInput& stmIn, STREAM_POS_t nSizeMax, IStreamProgressCallback* pProgress, TIMESYSD_t nTimeout)
	{
		//! Copy data from one read stream (stmIn) to another write stream (this). 
		//! @arg nSizeMax = Length of file or some arbitrary max to the stream size.
		//! like the IStream::CopyTo() or MFC CopyFrom()
		//! @return Size of data moved.

		cTimeSys tStart(cTimeSys::GetTimeNow());
		cHeapBlock Data(cStream::k_FILE_BLOCK_SIZE);	// temporary buffer.
		STREAM_POS_t dwAmount = 0;
		for (; dwAmount < nSizeMax;)
		{
			size_t nSizeBlock = cStream::k_FILE_BLOCK_SIZE;
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
						cThreadId::SleepCurrent(1);
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
				HRESULT hRes = pProgress->onProgressCallback(cStreamProgress(dwAmount, nSizeMax));
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

	HRESULT cStreamOutput::WriteSize(size_t nSize)
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

	HRESULT cStreamInput::ReadSize(OUT size_t& nSize)
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

	HRESULT cStreamInput::ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) // virtual
	{
		//! Read a string up until (including) a "\n" or "\r\n". end of line. FILE_EOL.
		//! Some streams can support this better than others. like fgets(FILE*)
		//! @arg iSizeMax = Maximum number of characters to be copied into pszBuffer (including room for the the terminating '\0' character).
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
			HRESULT hRes = this->ReadT<char>(ch);	// read one character at a time.
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

	HRESULT cStreamInput::ReadStringLine(OUT wchar_t* pszBuffer, StrLen_t iSizeMax) // virtual
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
			HRESULT hRes = this->ReadT<wchar_t>(ch);  // read one character at a time.
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

	HRESULT cStreamInput::ReadPeek(void* pData, size_t nDataSize) // virtual
	{
		//! Peek ahead in the stream if possible. Non blocking.
		//! just try to read data but not remove from the queue.
		//! @return Amount peeked.

		HRESULT hResRead = ReadX(pData, nDataSize);
		if (FAILED(hResRead) || hResRead == 0)
		{
			return hResRead;
		}
		HRESULT hResSeek = SeekX(-hResRead, SEEK_Cur);	// back up.
		if (FAILED(hResSeek))
		{
			return hResSeek;	// ERROR.
		}
		return hResRead;
	}
}
