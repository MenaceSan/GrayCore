//
//! @file cQueueChunked.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cQueueChunked_H
#define _INC_cQueueChunked_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cQueue.h"

namespace Gray
{

	template<class TYPE = BYTE, ITERATE_t _QTY = 1024>
	class GRAYCORE_LINK cQueueStatic : public cQueueIndex
	{
		//! @class Gray::cQueueStatic
		//! Create a generic thread/multi process safe (static sized) wrapping queue.
		//! similar to std::istringstream except NOT on Heap. static allocations.
		//! @note Get and Put are NOT reentrant safe against themselves, just each other.
		//! m_iWriteLast and m_iReadLast will wrap to fill.

	public:
		TYPE m_Data[_QTY];		// Not heap allocation.

	protected:
		inline ITERATE_t GetWrapIndex(ITERATE_t i) const noexcept
		{
			return i % _QTY;
		}

	private:
		inline ITERATE_t get_ReadQty() const	// Don't use this if we wrap.
		{
			ASSERT(false);
			return 0;
		}

	public:
		cQueueStatic() noexcept
		{
			STATIC_ASSERT(_QTY > 0, _QTY);
#if defined(_DEBUG)
			cMem::Zero(m_Data, sizeof(m_Data));
#endif
		}
		bool isFullQ() const noexcept
		{
			return GetWrapIndex(m_iWriteLast + 1) == m_iReadLast;
		}
		ITERATE_t get_ReadQtyT() const noexcept
		{
			//! How much Total data is in the Queue ? may be wrapped. Thread safe.
			ITERATE_t iWrite = m_iWriteLast;
			ITERATE_t iRead = m_iReadLast;
			if (iRead > iWrite)	// wrap
			{
				iWrite += _QTY;
				DEBUG_ASSERT(iWrite > iRead, "get_ReadQtyT");	// sanity check. should never happen!
			}
			return iWrite - iRead;
		}
		ITERATE_t get_ReadQtyC() const noexcept
		{
			//! Max we can get in a single CONTIGUOUS block peek/read. For use with get_ReadPtr().
			ITERATE_t iTop = (m_iWriteLast >= m_iReadLast) ? m_iWriteLast : _QTY;
			return iTop - m_iReadLast;
		}
		const TYPE* get_ReadPtr() const
		{
			//! use get_ReadQtyC() to get the allowed size.
			ASSERT(!isEmptyQ());	// ONLY call this if there is data to read.
			return &m_Data[m_iReadLast];
		}
		void AdvanceRead(ITERATE_t iCount = 1)
		{
			ITERATE_t iReadCount = get_ReadQtyT();
			if (iCount > iReadCount)
				iCount = iReadCount;
			m_iReadLast = GetWrapIndex(m_iReadLast + iCount);
		}

		ITERATE_t get_WriteQtyT() const noexcept
		{
			//! total available space to write - not contiguous
			//! @note since read=write=empty we can only use QTY-1 to write.
			return (_QTY - 1) - get_ReadQtyT();
		}

		TYPE Read1()
		{
			//! Read a single TYPE element. Thread safe against Write.
			//! @note This is NOT reentrant safe. i.e. multi calls to Read().
			//! @note ASSERT if empty!
			ASSERT(!isEmptyQ());
			ITERATE_t iRead = m_iReadLast;
			ASSERT(IS_INDEX_GOOD(iRead, _QTY));
			ITERATE_t iReadNext = GetWrapIndex(iRead + 1);
			TYPE val = m_Data[iRead];
			m_iReadLast = iReadNext;
			return val;
		}
		ITERATE_t ReadQty(TYPE* pBuf, ITERATE_t nCountMax)
		{
			//! copy TYPE data out. NOT thread safe.
			//! @todo use cValArray::CopyQty ?

			ITERATE_t i = 0;
			for (; !isEmptyQ() && i < nCountMax; i++)
			{
				pBuf[i] = Read1();
			}
			return i;
		}
		ITERATE_t ReadQtySafe(TYPE* pBuf, ITERATE_t nCountMax)
		{
			//! copy TYPE data out. Thread Safe against write.
			ITERATE_t i = 0;
			for (; !isEmptyQ() && i < nCountMax; i++)
			{
				pBuf[i] = Read1();
			}
			return i;
		}

		bool WriteQ(TYPE val)
		{
			//! Add a single TYPE element. Thread Safe against Read.
			//! @note This is NOT reentrant safe. i.e. multi calls to Write()
			//! @return
			//!  false = full.
			ITERATE_t iWrite = m_iWriteLast;
			ASSERT(IS_INDEX_GOOD(iWrite, _QTY));
			ITERATE_t iWriteNext = GetWrapIndex(iWrite + 1);
			if (iWriteNext == m_iReadLast)	// isFullQ() ?
				return false;
			m_Data[iWrite] = val;
			m_iWriteLast = iWriteNext;
			return true;
		}
		HRESULT WriteQty(const TYPE* pVal, ITERATE_t iLength)
		{
			//! Add several TYPE items to the Q using cValArray::CopyQty. NOT thread safe.
			//! @note This is NOT reentrant/thread safe.
			//! @return
			//!   length put. 0 = full. I cant write anything.
			ITERATE_t iRoom = get_WriteQtyT();
			ASSERT(iRoom >= 0 && iRoom <= _QTY);
			ITERATE_t iWrite = m_iWriteLast;
			ASSERT(iWrite >= 0 && iWrite < _QTY);
			ITERATE_t iLengthMin = MIN(iRoom, iLength); // max i can write and hold.
			if (iWrite + iLengthMin > _QTY)	// will overflow/wrap?
			{
				ITERATE_t iTmp1 = _QTY - iWrite;
				cValArray::CopyQty(m_Data + iWrite, pVal, iTmp1);	// Write to end of buffer.
				ITERATE_t iTmp2 = iLengthMin - iTmp1;
				cValArray::CopyQty(m_Data, pVal + iTmp1, iTmp2);	// Wrap back from beginning.
				m_iWriteLast = iTmp2;
			}
			else
			{
				cValArray::CopyQty(m_Data + iWrite, pVal, iLengthMin);
				m_iWriteLast = iWrite + iLengthMin;
			}
			return (HRESULT)iLengthMin;
		}

		HRESULT WriteQtySafe(const TYPE* pVal, ITERATE_t iLength)
		{
			//! Thread Safe against Read.
			ITERATE_t i = 0;
			for (; !isFullQ() && i < iLength; i++)
			{
				WriteQ(pVal[i]);
			}
			return (HRESULT)i;
		}
	};

	//***********************************************************************

	template<class TYPE = BYTE>
	class cQueueChunked
	{
		//! @class Gray::cQueueChunked
		//! a list of Q chunks to use as a large queue.
		//! Delete chunks when fully read.
		//! allocate new chunks when we need to write.
		//! No attempt to pool allocated chunks.

	public:
		class cQueueChunk : public cQueueDyn < TYPE >		// TODO cQueueStatic ??
		{
			//! @class Gray::cQueueChunked::cQueueChunk
			//! A single chunk in cQueueChunked list. 
			//! ??? USE cQueueStatic size here ???
		public:
			cQueueChunk* m_pNext;  // linked list of chunks.
		public:
			cQueueChunk(ITERATE_t nGrowSizeChunk)
				: cQueueDyn<TYPE>(nGrowSizeChunk)
				, m_pNext(nullptr)
			{}
		};

	private:
		const ITERATE_t m_nGrowSizeChunk;	//!< size of each block. (count of TYPE)
		ITERATE_t m_nTotalQty;			//!< total data waiting to be read. (total count of TYPE)
		cQueueChunk* m_pFirst;		// linked list of chunks.
		cQueueChunk* m_pLast;

	public:
		cQueueChunked(ITERATE_t nGrowSizeChunk) noexcept
			: m_nGrowSizeChunk(nGrowSizeChunk)
			, m_nTotalQty(0)
			, m_pFirst(nullptr)
			, m_pLast(nullptr)
		{
		}
		~cQueueChunked()
		{
			SetEmptyQ();
		}

		bool isEmptyQ() const noexcept
		{
			return m_nTotalQty == 0;
		}
		void SetEmptyQ()
		{
			AdvanceRead(m_nTotalQty);
			ASSERT(m_nTotalQty == 0);
			ASSERT(m_pFirst == nullptr);
			ASSERT(m_pLast == nullptr);
		}

		ITERATE_t get_ReadQtyT() const noexcept
		{
			//! get total number of entries
			return m_nTotalQty;
		}
		ITERATE_t get_ReadQtyC() const noexcept
		{
			//! C = get contiguous entries in the first block.
			if (m_pFirst == nullptr)
				return 0;
			return m_pFirst->get_ReadQty();
		}
		const TYPE* get_ReadPtrC() const
		{
			//! C = contiguous entries in the first block.
			//! @return a pointer to new data.
			ASSERT(!isEmptyQ());
			ASSERT_NN(m_pFirst);
			return m_pFirst->get_ReadPtr();
		}
		void AdvanceRead(ITERATE_t iCount = 1)
		{
			//! Destructive read.
			ITERATE_t iCountLeft = iCount;
			while (iCountLeft >= 0)
			{
				cQueueChunk* pCur = m_pFirst;
				if (pCur == nullptr)
					break;
				ITERATE_t iSize = pCur->get_ReadQty();
				if (iCountLeft < iSize)
				{
					pCur->AdvanceRead(iCountLeft);
					break;
				}
				pCur->AdvanceRead(iSize);
				m_pFirst = pCur->m_pNext;
				delete pCur;
				iCountLeft -= iSize;
			}
			m_nTotalQty -= iCount;
			ASSERT(m_nTotalQty >= 0);
			if (m_pFirst == nullptr)
			{
				m_pLast = nullptr;
			}
		}
		ITERATE_t get_WriteSpaceQty(void) const
		{
			//! Qty Avail to be written to.
			if (m_pLast == nullptr)
			{
				return m_nGrowSizeChunk;
			}
			return m_pLast->get_WriteSpaceQty();
		}
		TYPE* GetWritePrepared(ITERATE_t iNeedCount = 1)
		{
			//! Use get_WriteSpaceQty() to check size.
			//! iNeedCount of TYPE
			cQueueChunk* pCur = m_pLast;
			if (pCur == nullptr)
			{
				m_pFirst = m_pLast = new cQueueChunk(m_nGrowSizeChunk);
			}
			else if (m_pLast->isFullQ())
			{
				pCur->m_pNext = m_pLast = new cQueueChunk(m_nGrowSizeChunk);
			}
			return m_pLast->GetWritePrepared(iNeedCount);
		}
		void AdvanceWrite(ITERATE_t iCount = 1)
		{
			// iCount of TYPE
			ASSERT(m_pLast != nullptr);
			ASSERT(iCount <= get_WriteSpaceQty());
			m_pLast->AdvanceWrite(iCount);
			m_nTotalQty += iCount;
		}

		TYPE Read1(void)
		{
			ASSERT(m_pFirst != nullptr);
			const TYPE* pBuf = get_ReadPtrC();
			TYPE val = *pBuf;
			AdvanceRead(1);
			return val;
		}
		ITERATE_t ReadQty(TYPE* pBuf, ITERATE_t nCountMax)
		{
			//! copy data out.
			ITERATE_t i = 0;
			for (; !isEmptyQ() && i < nCountMax; i++)
			{
				pBuf[i] = Read1();
			}
			return i;
		}
		void WriteQ(TYPE val)
		{
			TYPE* pBuf = GetWritePrepared(1);
			*pBuf = val;
			AdvanceWrite(1);
		}
		HRESULT WriteQty(const TYPE* pBuf, ITERATE_t nCount)
		{
			//! copy data in.
			ITERATE_t i = 0;
			for (; i < nCount; i++)
			{
				WriteQ(pBuf[i]);
			}
			return (HRESULT)i;
		}
	};

}	// Gray
#endif // _INC_cQueueChunked_H
