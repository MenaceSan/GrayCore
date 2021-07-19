//
//! @file cQueue.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cQueue_H
#define _INC_cQueue_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cHeap.h"
#include "HResult.h"
#include "cArrayT.h"
#include "cStreamProgress.h"

namespace Gray
{
	class GRAYCORE_LINK cQueueIndex
	{
		//! @class Gray::cQueueIndex
		//! All types of queues have indexes in common. read index and write index.
		//! Derived class might be grow-able, static vs dynamic memory, fixed size, wrappable, etc.
		//! Base class for all cQueue*.

	protected:
		ITERATE_t m_iReadLast;	//!< old items removed/read from here.
		ITERATE_t m_iWriteLast;	//!< new items added/written here. end of readable.

	public:
		cQueueIndex(ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) noexcept
			: m_iReadLast(iReadLast)
			, m_iWriteLast(iWriteLast)
		{
		}
		void InitQ(ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) noexcept
		{
			m_iReadLast = iReadLast;
			m_iWriteLast = iWriteLast;	// put new data here.
		}
		bool isEmptyQ() const noexcept
		{
			return m_iReadLast == m_iWriteLast;
		}
		void SetEmptyQ() noexcept
		{
			//! thread safe. single instruction operations cannot be time sliced.
			//! @note Should NOT be called by the Put thread !
			m_iReadLast = m_iWriteLast = 0;
		}

		ITERATE_t get_ReadIndex() const noexcept
		{
			//! @return Next read position.
			return m_iReadLast;
		}
		ITERATE_t get_WriteIndex() const noexcept
		{
			//! @return Next write position.
			return m_iWriteLast;
		}

		inline ITERATE_t get_ReadQty() const noexcept
		{
			//! How much data is avail to read? // Assume will not will wrap to fill.
			//! @return Quantity of TYPE COUNT_t
			DEBUG_CHECK(m_iWriteLast >= m_iReadLast);	// Assume will not will wrap to fill.
			return m_iWriteLast - m_iReadLast;
		}

		inline void AdvanceRead(ITERATE_t iCount = 1) noexcept
		{
			// Assume will not will wrap to fill.
			m_iReadLast += iCount;
			DEBUG_CHECK(m_iReadLast >= 0 && m_iReadLast <= m_iWriteLast);	// Assume will not will wrap to fill.
		}

		HRESULT SeekQ(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set);	// support parents SeekX
 
	};

	//*********************************************************************

	template <class TYPE = BYTE>
	class GRAYCORE_LINK cQueueRead : public cQueueIndex, protected cMemBlock
	{
		//! @class Gray::cQueueRead
		//! a simple read only queue. un-managed memory
		//! cMemBlock = NOT owned/managed block of memory I read from. not freed on destruct.

	public:
		cQueueRead(const TYPE* pData = nullptr, ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) noexcept
			: cQueueIndex(iReadLast, iWriteLast)
			, cMemBlock(pData, iWriteLast * sizeof(TYPE))
		{
		}
		~cQueueRead() noexcept
		{
			// NOT free m_pData memory.
		}

		inline const TYPE* get_Data() const noexcept
		{
			// get typed data pointer.
			return (const TYPE*)m_pData;
		}
		inline TYPE* get_Data() noexcept
		{
			// get typed data pointer.
			return (TYPE*)m_pData;
		}

		// Peek into/read from the Queue's data.
		inline const TYPE* get_ReadPtr() const noexcept
		{
			//! get start of data i could read directly. contiguous
			//! isEmptyQ() is OK, might be 0 length.
			DEBUG_CHECK(isValidPtr());
			return get_Data() + m_iReadLast;
		}
		void SetQueueRead(const TYPE* pData, ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0)
		{
			this->SetBlock(const_cast<TYPE*>(pData), iWriteLast * sizeof(TYPE));
			InitQ(iReadLast, iWriteLast);
		}

		TYPE Read1(void)
		{
			//! get a single TYPE element.
			ASSERT(!isEmptyQ());
			return get_Data()[m_iReadLast++];
		}
		HRESULT ReadPeek(TYPE* pData, ITERATE_t iDataMaxQty)
		{
			//! @arg iDataMaxQty = max qty of TYPE units i can fit in pData.
			//! @arg pData = nullptr = just return how much data i might get.
			//! @return quantity i actually read.
			ITERATE_t iQtyAvail = get_ReadQty();
			if (iDataMaxQty > iQtyAvail)
				iDataMaxQty = iQtyAvail;
			if (pData != nullptr)
			{
				//! copy data out.
				cValArray::CopyQty(pData, get_Data() + m_iReadLast, iDataMaxQty);
			}
			return (HRESULT)iDataMaxQty;
		}

		ITERATE_t ReadQty(TYPE* pData, ITERATE_t iDataMaxQty)
		{
			//! Just read a block. like ReadX but for TYPE
			//! @arg iDataMaxQty = max qty of TYPE units i can fit in pData.
			//! @return iQty i actually read.

			const ITERATE_t iQtyAvail = get_ReadQty();
			if (iDataMaxQty > iQtyAvail)
				iDataMaxQty = iQtyAvail;
			if (pData != nullptr)
			{
				//! copy data out.
				cValArray::CopyQty(pData, get_Data() + m_iReadLast, iDataMaxQty);
			}
			AdvanceRead(iDataMaxQty);		// advance m_iReadLast pointer.
			return iDataMaxQty;
		}

		void ReadCommitNow()
		{
			//! Prepare to read. move (read) data down to not waste space. allow more space for writing.
			//! commit the read = can't get the data back. SeekX will fail.
			//! @note beware of the rollback that protocols like to do if they get a bad request or non atomic transactions.
			//! can't SeekX() back after this.
			//! pointers into this are now bad!
			if (this->m_iReadLast <= 0)					// next read is already at 0.
				return;
			const ITERATE_t iSize = this->get_ReadQty();
			if (iSize > 0)	// there is data to move ?
			{
				const TYPE* pTmp = this->get_Data() + this->m_iReadLast;
				cMem::CopyOverlap(this->get_Data(), pTmp, iSize * sizeof(TYPE));
			}
			this->InitQ(0, iSize);
		}
	};

	//*********************************************************************

	template <class TYPE = BYTE>
	class GRAYCORE_LINK cQueueRW : public cQueueRead < TYPE >
	{
		//! @class Gray::cQueueRW
		//! Create an simple arbitrary queue of TYPE elements. can read and write.
		//! @note This queue does NOT wrap. Does NOT grow. Non managed memory.
		//! @note This is NOT thread safe
		//! @note Does NOT free memory on destruct. Use cQueueBytes for that.
		//! Does NOT auto expand buffer to hold more data if write past end.

		typedef cQueueRead<TYPE> SUPER_t;

	protected:
		ITERATE_t m_iAutoReadCommit;	//!< Read data is destroyed once read more than this amount. make more room for writing. 0 = don't do this, just fail write if we run out of space.

	public:
		inline ITERATE_t get_AllocQty() const noexcept
		{
			// How much total space allocated for this?
			return (ITERATE_t)(this->get_DataSize() / sizeof(TYPE));
		}

		inline bool isFullQ() const noexcept
		{
			// Cant fit any more. would have to grow buffer.
			return this->m_iWriteLast >= get_AllocQty();
		}

		inline ITERATE_t get_WriteSpaceQty() const
		{
			//! How much space is avail for write into buffer? (given the current get_AllocQty() allocation size)
			//! @return Qty of TYPE
			ASSERT(this->m_iWriteLast <= get_AllocQty());
			return get_AllocQty() - this->m_iWriteLast;
		}

		virtual TYPE* GetWritePrepared(ITERATE_t iNeedCount)
		{
			//! get start of data i could write directly. contiguous
			UNREFERENCED_PARAMETER(iNeedCount);
			if (!isValidPtr())
				return nullptr;
			return this->get_Data() + this->m_iWriteLast;
		}

		inline void AdvanceWrite(ITERATE_t iCount = 1)
		{
			//! paired with GetWritePrepared
			//! iCount <0 is ok.
			ASSERT(iCount <= get_WriteSpaceQty());
			this->m_iWriteLast += iCount;
			ASSERT(this->m_iWriteLast <= get_AllocQty());
		}

	public:
		cQueueRW() noexcept
			: cQueueRead<TYPE>(nullptr, 0, 0)
			, m_iAutoReadCommit(0)
		{
			// empty.
		}
		explicit cQueueRW(TYPE* pData, ITERATE_t iDataAlloc, ITERATE_t iReadLast, ITERATE_t iWriteLast, ITERATE_t iAutoReadCommit = 0) noexcept
			: cQueueRead<TYPE>(pData, iReadLast, iWriteLast)
			, m_iAutoReadCommit(iAutoReadCommit)
		{
			// Read / Write.
			this->put_DataSize(iDataAlloc * sizeof(TYPE));
		}
		explicit cQueueRW(const TYPE* pData, ITERATE_t iDataMax) noexcept
			: cQueueRead<TYPE>(const_cast<TYPE*>(pData), 0, iDataMax)
			, m_iAutoReadCommit(0)
		{
			// Read Only iDataMax.
		}
		~cQueueRW() noexcept
		{
			//! Does NOT free m_pData memory.
		}

		//***************************************************
		// Reader functions.

		void ReadCommitCheck()
		{
			//! is it time to attempt to reclaim space in the queue. (So we can write more)
			//! @note beware of the roll back that protocols like to do if they get a bad request/underflow. 
			//! can't SeekX() back now !
			if (m_iAutoReadCommit != 0 && this->m_iReadLast >= m_iAutoReadCommit) // (ITERATE_t) m_nGrowSizeChunk
			{
				this->ReadCommitNow();
			}
		}
		ITERATE_t get_AutoReadCommit() const noexcept
		{
			return m_iAutoReadCommit;
		}
		void put_AutoReadCommit(ITERATE_t iAutoReadCommit = 8 * 1024)
		{
			//! For SetSeekSizeMin
			//! @arg iAutoReadCommit = the size at which we 'commit' contents and erase already read data. to make room for more writing.
			//!		0 = never do auto commit. we are reading and we may need to SeekX back.
			m_iAutoReadCommit = iAutoReadCommit;
			if (iAutoReadCommit != 0)
			{
				this->ReadCommitNow();
			}
		}

		void put_ReadIndex(ITERATE_t iReadLo)
		{
			//! Reset the read index back to some new place.
			ASSERT(iReadLo >= 0);
			ASSERT(iReadLo <= this->m_iWriteLast);
			this->m_iReadLast = iReadLo;
			ReadCommitCheck();
		}

		// Act as a stream
		HRESULT SeekQ(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set)
		{
			//! move the current read start location.
			//! @arg
			//!  iOffset = quantity of TYPE
			//!  eSeekOrigin = SEEK_CUR, etc
			//! @return
			//!  the New position,  <0=FAILED = INVALID_SET_FILE_POINTER
			SUPER_t::SeekQ(iOffset, eSeekOrigin);
			ReadCommitCheck();
			return (HRESULT)this->m_iReadLast;
		}

		ITERATE_t ReadQty(TYPE* pData, ITERATE_t iDataMaxQty)
		{
			//! Just read a block. like ReadX but for TYPE
			//! @return iQty i actually read.
			const ITERATE_t nReadQty = SUPER_t::ReadQty(pData, iDataMaxQty);
			ReadCommitCheck();
			return nReadQty;
		}

		HRESULT ReadX(void* pData, size_t nDataSize)
		{
			//! read bytes
			const ITERATE_t nReadQty = ReadQty((TYPE*)pData, (ITERATE_t)(nDataSize / sizeof(TYPE)));
			return (HRESULT)(nReadQty * sizeof(TYPE));
		}

		//***************************************************
		// Writer functions.

		HRESULT WriteQty(const TYPE* pData, ITERATE_t iQty)
		{
			//! Write up to iQty * TYPE data to the q. like WriteX but for TYPE
			//! @return
			//!  How much can did actually write in to the queue, before it gets full.

			TYPE* pWrite = GetWritePrepared(iQty);
			const ITERATE_t iWriteSpace = get_WriteSpaceQty();
			const ITERATE_t iWriteQty = MIN(iWriteSpace, iQty);
			ASSERT(this->m_iWriteLast + iWriteQty <= get_AllocQty());		// assume enough space.

			if (pData != nullptr && pWrite != nullptr)
			{
				cValArray::CopyQty(pWrite, pData, iWriteQty);
			}

			AdvanceWrite(iWriteQty);
			return (HRESULT)iWriteQty;
		}

		HRESULT WriteX(const void* pData, size_t nDataSize)
		{
			//! Write a buffer/array of bytes into the Q
			//! @arg nDataSize = bytes NOT instances of TYPE
			//! @return size of data added in bytes.
			HRESULT iWriteQty = WriteQty((TYPE*)pData, (ITERATE_t)(nDataSize / sizeof(TYPE)));
			return iWriteQty * sizeof(TYPE);
		}

		bool WriteQ(TYPE val)
		{
			//! Write a single value into the Q.
			TYPE* pWrite = GetWritePrepared(1);
			const ITERATE_t iWriteSpace = get_WriteSpaceQty();
			if (iWriteSpace == 0)
				return false;
			if (pWrite != nullptr)
			{
				*pWrite = val;
			}
			AdvanceWrite(1);
			return true;
		}

		void WriteQ(cQueueRead<TYPE>& queue)
		{
			//! Write a Q into the Q
			WriteQty(queue.get_ReadPtr(), queue.get_ReadQty());
			queue.SetEmptyQ();
		}
	};

	//*********************************************************************

	template<class TYPE = BYTE>
	class cQueueDyn : public cQueueRW < TYPE >
	{
		//! @class Gray::cQueueDyn
		//! Create a generic (dynamic sized) contiguous queue.
		//! Does NOT wrap! just grows as more is written.
		//! @note Needs to SetEmptyQ() or AutoReadCommit() periodically so it doesn't grow > nGrowSizeMax and FAIL!
		//! free on destruct.

		typedef cQueueRW<TYPE> SUPER_t;

	private:
		cArrayT<TYPE> m_aData;		//!< dynamic sized m_pData for Queue.

	protected:
		ITERATE_t m_nGrowSizeChunk;		//!< number of TYPE elements to grow by in a single re-alloc chunk. 0 = never grow.
		ITERATE_t m_nGrowSizeMax;		//!< Total arbitrary max allowed for get_AllocQty(). 0 = never grow.

	protected:
		bool AllocSizeMaxQ(ITERATE_t iDataAlloc)
		{
			//! (re)Allocate the total size we will need.
			if (iDataAlloc > m_nGrowSizeMax)	// too big !
			{
				return false;
			}
			if (this->get_AllocQty() != iDataAlloc)
			{
				m_aData.put_Count(iDataAlloc);	// realloc
				this->SetBlock(m_aData.get_DataWork(), iDataAlloc * sizeof(TYPE));
			}
			return true;
		}

	public:
		cQueueDyn(ITERATE_t nGrowSizeChunk = 64, ITERATE_t nGrowSizeMax = (cHeap::k_ALLOC_MAX / sizeof(TYPE))) noexcept
			: m_nGrowSizeChunk(nGrowSizeChunk)
			, m_nGrowSizeMax(nGrowSizeMax)
		{
			//! @arg nGrowSizeMax = 0 = not used. write only ?

			DEBUG_CHECK(m_nGrowSizeChunk >= 0);
			DEBUG_CHECK(m_nGrowSizeMax >= 0);
			if (m_nGrowSizeMax > 0)
			{
				if (m_nGrowSizeChunk < 64)		// reasonable size MIN.
					m_nGrowSizeChunk = 64;
				if (m_nGrowSizeChunk > m_nGrowSizeMax)		// Not allowed.
					m_nGrowSizeMax = m_nGrowSizeChunk;
			}
			else
			{
				m_nGrowSizeChunk = 0;	// Must both be 0.
			}
			AllocSizeMaxQ(m_nGrowSizeChunk);
			this->put_AutoReadCommit(m_nGrowSizeChunk / 2);		// default = half buffer.
		}
		virtual ~cQueueDyn()
		{
			//! m_aData is freed
		}

		void put_GrowSizeChunk(ITERATE_t nGrowSizeChunk) noexcept
		{
			//! How big are the chunks if we need to grow.
			if (nGrowSizeChunk > m_nGrowSizeMax)	// Must not be greater!
				m_nGrowSizeMax = nGrowSizeChunk;
			m_nGrowSizeChunk = nGrowSizeChunk;
		}
		inline ITERATE_t get_GrowSizeChunk() const noexcept
		{
			return m_nGrowSizeChunk;
		}

		TYPE* GetWritePrepared(ITERATE_t iNeedCount) override
		{
			//! Try to get enough room to write iNeedCount of TYPE.
			//! Use get_WriteSpaceQty() to check size avail. Grow buffer if i need to/can.
			//! paired with AdvanceWrite()

			const ITERATE_t iRoom = this->get_WriteSpaceQty();
			if (iRoom < iNeedCount)	// all set?
			{
				// need more.
				const ITERATE_t iOldAllocQty = this->get_AllocQty();
				if (iOldAllocQty < m_nGrowSizeMax)	// I cant get enough room. I cant write.
				{
					const ITERATE_t iGrowRequest = iNeedCount - iRoom;	// how much MORE do i need?
					ASSERT(iGrowRequest > 0);

					// must grow by rounded up chunk size.
					ASSERT(m_nGrowSizeChunk > 0);
					ITERATE_t iChunksGrow = iGrowRequest / m_nGrowSizeChunk;
					const ITERATE_t iRem = iGrowRequest % m_nGrowSizeChunk;
					if (iRem != 0)
						iChunksGrow++;
					ASSERT(iChunksGrow > 0);

					ITERATE_t iNewAllocQty = iOldAllocQty + iChunksGrow * m_nGrowSizeChunk;
					if (iNewAllocQty > m_nGrowSizeMax)	// Chunks too big !
					{
						iNewAllocQty = m_nGrowSizeMax;
					}

					AllocSizeMaxQ(iNewAllocQty);
				}
			}

			return SUPER_t::GetWritePrepared(iNeedCount);
		}
	};

	//*********************************************************************

	class GRAYCORE_LINK cQueueBytes : public cQueueDyn < BYTE >
	{
		//! @class Gray::cQueueBytes
		//! a dynamic BYTE queue that grows the memory allocation as needed.
		//! free on destruct.

		typedef cQueueDyn<BYTE> SUPER_t;

	public:
		explicit cQueueBytes(size_t nGrowSizeChunk = 8 * 1024, size_t nGrowSizeMax = cHeap::k_ALLOC_MAX) noexcept
			: cQueueDyn<BYTE>((ITERATE_t)nGrowSizeChunk, (ITERATE_t)nGrowSizeMax)
		{
			//! @arg nGrowSizeMax = 0 = not used. write only ? total size < nGrowSizeMax.
		}
		virtual ~cQueueBytes()
		{
			// Memory is freed
		}

		bool InsertDataHead(const BYTE* pDataSrc, size_t iLen)
		{
			//! insert data at the head of the queue. first out.
			BYTE* pWriteSpace = GetWritePrepared((ITERATE_t)iLen);
			UNREFERENCED_PARAMETER(pWriteSpace);
			if ((size_t)get_WriteSpaceQty() < iLen)
				return false;
			BYTE* pDataHead = get_Data();
			cMem::CopyOverlap(pDataHead + m_iReadLast + iLen, pDataHead + m_iReadLast, get_ReadQty());
			cMem::Copy(pDataHead + m_iReadLast, pDataSrc, iLen);
			AdvanceWrite((ITERATE_t)iLen);
			return true;
		}
		bool SetAllData(const BYTE* pData, size_t iLen)
		{
			//! Replace with new data. Toss any previous data. 
			//! just set the data in the queue. erase any previous data
			if (iLen > get_DataSize())	// need to grow ?
			{
				if (!AllocSizeMaxQ((ITERATE_t)iLen))
					return false;
			}
			cMem::CopyOverlap(get_Data(), pData, iLen);	// may be from the same buffer!? use memmove to be safe.
			InitQ(0, (ITERATE_t)iLen);
			return true;
		}
	};

#ifndef GRAY_STATICLIB // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cQueueRead < BYTE >;
	template class GRAYCORE_LINK cQueueRW < BYTE >;
#endif

}	// Gray
#endif // _INC_cQueue_H
