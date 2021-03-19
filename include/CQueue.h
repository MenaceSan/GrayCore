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
#include "cArray.h"
#include "cStreamProgress.h"

namespace Gray
{
	class GRAYCORE_LINK cQueueBase
	{
		//! @class Gray::cQueueBase
		//! All types of queues have this in common. read index and write index.
		//! This might be grow-able, static vs dynamic memory, fixed size, wrappable, etc.
		//! Base class for all Queues.

	protected:
		ITERATE_t m_iReadLast;	//!< old items removed/read from here.
		ITERATE_t m_iWriteLast;	//!< new items added/written here. end of read.

	public:
		cQueueBase(ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) noexcept
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
		void EmptyQ() noexcept
		{
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
		inline ITERATE_t get_ReadQty() const
		{
			//! How much data is avail to read? // Assume will not will wrap to fill.
			//! @return Quantity of TYPE COUNT_t
			ASSERT(m_iWriteLast >= m_iReadLast);	// Assume will not will wrap to fill.
			return m_iWriteLast - m_iReadLast;
		}

		inline void AdvanceRead(ITERATE_t iCount = 1)
		{
			// Assume will not will wrap to fill.
			m_iReadLast += iCount;
			ASSERT(m_iReadLast >= 0 && m_iReadLast <= m_iWriteLast);	// Assume will not will wrap to fill.
		}

		HRESULT SeekQ(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set);	// support virtual

		UNITTEST_FRIEND(cQueue);
	};

	//*********************************************************************

	template<class TYPE = BYTE, ITERATE_t _QTY = 1024>
	class cStackStatic
	{
		//! @class Gray::cStackStatic
		//! Create a generic thread/multi process safe (static sized) stack.

		TYPE m_Data[_QTY];		//!< Not heap allocation. static/inline allocated
		ITERATE_t m_iWriteNext;	//!< last .

	public:
		inline cStackStatic() noexcept
			: m_iWriteNext(0)
		{
			STATIC_ASSERT(_QTY > 0, cStackStatic);
		}
		inline bool isEmpty() const noexcept
		{
			return m_iWriteNext == 0;
		}
		inline bool isFull() const noexcept
		{
			return m_iWriteNext >= _QTY;
		}
		inline TYPE Pop()
		{
			ASSERT(m_iWriteNext >= 1);
			return m_Data[--m_iWriteNext];
		}
		inline void Push(TYPE v)
		{
			ASSERT(m_iWriteNext < _QTY);
			m_Data[m_iWriteNext++] = v;
		}
	};

	template<class TYPE = BYTE, ITERATE_t _QTY = 1024>
	class GRAYCORE_LINK cQueueStatic : public cQueueBase
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
		void EmptyQ() noexcept
		{
			//! thread safe. single instruction operations cannot be time sliced.
			//! @note Should NOT be called by the Put thread !
			m_iReadLast = m_iWriteLast;
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
			return (HRESULT) i;
		}
	};

	//*********************************************************************

	template <class TYPE = BYTE>
	class GRAYCORE_LINK cQueueRead : public cQueueBase
	{
		//! @class Gray::cQueueRead
		//! a simple read only queue. un-managed memory

	protected:
		TYPE* m_pData;			//!< NOT owned/managed block of memory I read from. not freed on destruct.

	public:
		cQueueRead(const TYPE* pData = nullptr, ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0) noexcept
			: cQueueBase(iReadLast, iWriteLast)
			, m_pData(const_cast<TYPE*>(pData))
		{
		}
		~cQueueRead()
		{
			// NOT free m_pData memory.
		}

		// Peek into/read from the Queue's data.
		inline const TYPE* get_ReadPtr() const
		{
			//! get start of data i could read directly. contiguous
			//! isEmptyQ() is OK, might be 0 length.
			ASSERT(m_pData != nullptr);
			return(m_pData + m_iReadLast);
		}
		void SetQueueRead(const TYPE* pData, ITERATE_t iReadLast = 0, ITERATE_t iWriteLast = 0)
		{
			m_pData = const_cast<TYPE*>(pData);
			InitQ(iReadLast, iWriteLast);
		}

		TYPE Read1(void)
		{
			//! get a single TYPE element.
			ASSERT(!isEmptyQ());
			return m_pData[m_iReadLast++];
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
				cValArray::CopyQty(pData, m_pData + m_iReadLast, iDataMaxQty);
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
				cValArray::CopyQty(pData, m_pData + m_iReadLast, iDataMaxQty);
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
			ITERATE_t iSize = this->get_ReadQty();
			if (iSize > 0)	// there is data to move ?
			{
				const TYPE* pTmp = this->m_pData + this->m_iReadLast;
				cMem::CopyOverlap(this->m_pData, pTmp, iSize * sizeof(TYPE));
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
		ITERATE_t m_iDataSizeAlloc;		//!< The max qty we can write into m_pData. Maybe NOT exactly same as m_pData true OS allocated size?
		ITERATE_t m_iAutoReadCommit;	//!< Read data is destroyed once read more than this amount. make more room for writing. 0 = don't do this, just fail write if we run out of space.

	public:
		cQueueRW() noexcept
			: cQueueRead<TYPE>(nullptr, 0, 0)
			, m_iDataSizeAlloc(0)
			, m_iAutoReadCommit(0)
		{
			// empty.
		}
		explicit cQueueRW(TYPE* pData, ITERATE_t iDataAlloc, ITERATE_t iReadLast, ITERATE_t iWriteLast, ITERATE_t iAutoReadCommit = 0)
			: cQueueRead<TYPE>(pData, iReadLast, iWriteLast)
			, m_iDataSizeAlloc(iDataAlloc)
			, m_iAutoReadCommit(iAutoReadCommit)
		{
			// Read / Write.
		}
		explicit cQueueRW(const TYPE* pData, ITERATE_t iDataMax)
			: cQueueRead<TYPE>(const_cast<TYPE*>(pData), 0, iDataMax)
			, m_iDataSizeAlloc(iDataMax)
			, m_iAutoReadCommit(0)
		{
			// Read Only iDataMax.
		}
		~cQueueRW()
		{
			//! Does NOT free m_pData memory.
		}

		//***************************************************
		// Reader functions.

		void ReadCommitCheck()
		{
			//! is it time to attempt to reclaim the space in the queue
			//! @note beware of the roll back that protocols like to do if they get a bad request/underflow. can't SeekX() back now ?
			if (m_iAutoReadCommit != 0 && this->m_iReadLast >= m_iAutoReadCommit) // (ITERATE_t) m_nGrowSizeChunk
			{
				this->ReadCommitNow();
			}
		}
		ITERATE_t get_AutoReadCommit() const
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
			return (HRESULT) this->m_iReadLast;
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
			//! bytes
			const ITERATE_t nReadQty = ReadQty((TYPE*)pData, (ITERATE_t)(nDataSize / sizeof(TYPE)));
			return (HRESULT)(nReadQty * sizeof(TYPE));
		}

		//***************************************************
		// Writer functions.

		inline bool isFullQ() const
		{
			// Cant fit any more. would have to grow buffer.
			return(this->m_iWriteLast >= m_iDataSizeAlloc);
		}

		inline ITERATE_t get_WriteQty() const
		{
			//! How much space is avail for write into buffer? (given the current m_iDataSizeAlloc allocation size)
			//! @return Qty of TYPE
			ASSERT(this->m_iWriteLast <= m_iDataSizeAlloc);
			return(m_iDataSizeAlloc - this->m_iWriteLast);
		}
		inline TYPE* get_WritePtr() const
		{
			//! get start of data i could write directly. contiguous
			ASSERT(this->m_pData != nullptr);
			return(this->m_pData + this->m_iWriteLast);
		}

		HRESULT WriteQty(const TYPE* pData, ITERATE_t iQtyMax)
		{
			//! add iQty * TYPE data to the q. like WriteX but for TYPE
			//! @return
			//!  How much can did write in to the queue, before it gets full.

			ITERATE_t iWriteSpace = get_WriteQty();
			if (iWriteSpace <= 0)
				return 0;
			if ((ITERATE_t)iQtyMax > iWriteSpace)
				iQtyMax = iWriteSpace;
			if (pData != nullptr)
			{
				WriteQN(pData, iQtyMax);
			}
			return (HRESULT)iQtyMax;
		}

		HRESULT WriteX(const void* pData, size_t nDataSize)
		{
			//! nDataSize = bytes NOT instances of TYPE
			HRESULT iReadQty = WriteQty((TYPE*)pData, (ITERATE_t)(nDataSize / sizeof(TYPE)));
			return iReadQty * sizeof(TYPE);
		}

		inline void AdvanceWrite(ITERATE_t iCount = 1)
		{
			//! paired with GetWritePrepared
			ASSERT_N(iCount <= get_WriteQty());
			this->m_iWriteLast += iCount;
			ASSERT(m_iDataSizeAlloc >= this->m_iWriteLast);
		}

	protected:
		void WriteQN(const TYPE* pData, ITERATE_t iQtyMax)
		{
			//! Copy stuff into the Q
			//! ASSUME iQtyMax is safe size.
			if (iQtyMax <= 0)
				return;
			ASSERT_N(this->m_iWriteLast + iQtyMax <= m_iDataSizeAlloc);
			if (pData != nullptr)
			{
				cValArray::CopyQty(this->m_pData + this->m_iWriteLast, pData, iQtyMax);
			}
			AdvanceWrite(iQtyMax);
		}
	};

	//*********************************************************************

	template<class TYPE = BYTE>
	class cQueueDyn : public cQueueRW < TYPE >
	{
		//! @class Gray::cQueueDyn
		//! Create a generic (dynamic sized) contiguous queue.
		//! Does NOT wrap! just grows as more is written.
		//! @note Needs to EmptyQ() or AutoReadCommit() periodically so it doesn't grow > nGrowSizeMax and FAIL!
		//! free on destruct.

		typedef cQueueRW<TYPE> SUPER_t;

	private:
		cArrayVal<TYPE> m_aData;		//!< m_pData

	protected:
		ITERATE_t m_nGrowSizeChunk;		//!< number of TYPE elements to grow by in a single re-alloc chunk. 0 = never grow.
		ITERATE_t m_nGrowSizeMax;		//!< Total arbitrary max allowed for m_iDataSizeAlloc. 0 = never grow.

	protected:
		bool AllocSizeMaxQ(ITERATE_t iDataAlloc)
		{
			//! (re)Allocate the total size we will need.
			if (iDataAlloc > m_nGrowSizeMax)	// too big !
			{
				return false;
			}
			if (this->m_iDataSizeAlloc != iDataAlloc)
			{
				m_aData.SetSize(iDataAlloc);	// realloc
				this->m_pData = m_aData.get_DataWork();
				this->m_iDataSizeAlloc = iDataAlloc;
			}
			return true;
		}

	public:
		cQueueDyn(ITERATE_t nGrowSizeChunk = 128, ITERATE_t nGrowSizeMax = (cHeap::k_ALLOC_MAX / sizeof(TYPE))) noexcept
			: m_nGrowSizeChunk(nGrowSizeChunk)
			, m_nGrowSizeMax(nGrowSizeMax)
		{
			//! @arg nGrowSizeMax = 0 = not used. write only ?

			ASSERT(m_nGrowSizeChunk >= 0);
			ASSERT(m_nGrowSizeMax >= 0);
			if (m_nGrowSizeMax > 0)
			{
				if (m_nGrowSizeChunk < 128)
					m_nGrowSizeChunk = 128;
				if (m_nGrowSizeChunk > m_nGrowSizeMax)
					m_nGrowSizeMax = m_nGrowSizeChunk;
			}
			else
			{
				m_nGrowSizeChunk = 0;	// Must both be 0.
			}
			AllocSizeMaxQ(m_nGrowSizeChunk);
			this->put_AutoReadCommit(m_nGrowSizeChunk / 2);		// default = half buffer.
		}
		~cQueueDyn()
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
		ITERATE_t get_GrowSizeChunk() const noexcept
		{
			return m_nGrowSizeChunk;
		}

		bool MakeWritePrepared(ITERATE_t iDesiredCount = 1)
		{
			//! Use get_WriteQty() to check size avail. Grow buffer if i need to.
			//! paired with AdvanceWrite
			ITERATE_t iRoom = this->get_WriteQty();
			if (iRoom >= iDesiredCount)	// all set.
				return true;

			ITERATE_t iGrowRequest = iDesiredCount - iRoom;	// how much MORE do i need?
			ASSERT(iGrowRequest > 0);
			ITERATE_t iChunksGrow = iGrowRequest / m_nGrowSizeChunk;
			ITERATE_t iRem = iGrowRequest % m_nGrowSizeChunk;
			if (iRem != 0)
				iChunksGrow++;

			ITERATE_t iTotalSize = this->m_iDataSizeAlloc + iChunksGrow * m_nGrowSizeChunk;
			if (iTotalSize > m_nGrowSizeMax)	// too big !
				iTotalSize = m_nGrowSizeMax;

			if (iTotalSize - this->m_iWriteLast <= 0)		// can i get any?
				return false;	// Got no more space. we must wait?

			return AllocSizeMaxQ(iTotalSize);
		}
		TYPE* GetWritePrepared(ITERATE_t iDesiredCount = 1)
		{
			//! Use get_WriteQty() to check size avail.
			//! paired with AdvanceWrite
			if (!MakeWritePrepared(iDesiredCount))
				return nullptr;
			// ASSERT(get_WriteQty() >= iDesiredCount); except for m_nGrowSizeMax
			return this->get_WritePtr();
		}

		bool WriteQ(TYPE val)
		{
			//! Write a single value into the Q.
			if (this->m_iWriteLast >= this->m_iDataSizeAlloc)
			{
				if (!AllocSizeMaxQ(this->m_iDataSizeAlloc + m_nGrowSizeChunk))
				{
					return false;
				}
			}
			ITERATE_t iWrite = this->m_iWriteLast++;
			m_aData[iWrite] = val;
			return true;
		}
		HRESULT WriteQty(const TYPE* pVal, ITERATE_t iCount)
		{
			//! Write array into the Q
			//! pVal = nullptr = just test if it has enough room.
			if (!MakeWritePrepared(iCount))
			{
				return HRESULT_WIN32_C(ERROR_DATABASE_FULL);	// full!
			}
			ASSERT(this->get_WriteQty() >= iCount);
			if (pVal != nullptr)
			{
				this->WriteQN(pVal, iCount);
			}
			return (HRESULT)iCount;
		}

		void WriteQ(cQueueRead<BYTE>& queue)
		{
			//! Write a Q into the Q
			WriteQty(queue.get_ReadPtr(), queue.get_ReadQty());
			queue.EmptyQ();
		}

		HRESULT WriteX(const void* pData, size_t nDataSize)
		{
			//! Write a buffer/array into the Q
			//! @return size of data added.
			HRESULT hRes = this->WriteQty((const TYPE*)pData, (ITERATE_t)(nDataSize / sizeof(TYPE)));
			if (FAILED(hRes))
				return hRes;
			return (HRESULT)nDataSize;	// hRes * sizeof(TYPE)
		}
	};

	//*********************************************************************

	class GRAYCORE_LINK cQueueBytes : public cQueueDyn < BYTE >
	{
		//! @class Gray::cQueueBytes
		//! a queue that grows the memory allocation as needed.
		//! free on destruct.

		typedef cQueueDyn<BYTE> SUPER_t;
	public:
		explicit cQueueBytes(size_t nGrowSizeChunk = 8 * 1024, size_t nGrowSizeMax = cHeap::k_ALLOC_MAX) noexcept
			: cQueueDyn<BYTE>((ITERATE_t)nGrowSizeChunk, (ITERATE_t)nGrowSizeMax)
		{
			//! @arg nGrowSizeMax = 0 = not used. write only ? total size < nGrowSizeMax.
		}
		~cQueueBytes()
		{
			// Memory is freed
		}

		bool InsertDataHead(const BYTE* pDataSrc, size_t iLen)
		{
			//! insert data at the head of the queue. first out.
			if (!MakeWritePrepared((ITERATE_t)iLen))
				return false;
			cMem::CopyOverlap(m_pData + m_iReadLast + iLen, m_pData + m_iReadLast, get_ReadQty());
			cMem::Copy(m_pData + m_iReadLast, pDataSrc, iLen);
			AdvanceWrite((ITERATE_t)iLen);
			return true;
		}
		bool SetAllData(const BYTE* pData, size_t iLen)
		{
			//! Toss any previous data. replace with new data.
			//! just set the data in the queue. erase any previous data
			if (iLen > (size_t)m_iDataSizeAlloc)	// grow a little?
			{
				if (!AllocSizeMaxQ((ITERATE_t)iLen))
					return false;
			}
			cMem::CopyOverlap(m_pData, pData, iLen);	// may be from the same buffer!? use memmove to be safe.
			InitQ(0, (ITERATE_t)iLen);
			return true;
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
		class cQueueChunk : public cQueueDyn < TYPE >
		{
			//! @class Gray::cQueueChunked::cQueueChunk
			//! A single chunk in cQueueChunked list.
		public:
			cQueueChunk* m_pNext;
		public:
			cQueueChunk(ITERATE_t nGrowSizeChunk)
				: cQueueDyn<TYPE>(nGrowSizeChunk)
				, m_pNext(nullptr)
			{}
		};

	private:
		const ITERATE_t m_nGrowSizeChunk;	//!< size of each block. (count of TYPE)
		ITERATE_t m_nTotalQty;			//!< total data waiting to be read. (total count of TYPE)
		cQueueChunk* m_pFirst;
		cQueueChunk* m_pLast;

	public:
		cQueueChunked(ITERATE_t nGrowSizeChunk)
			: m_nGrowSizeChunk(nGrowSizeChunk)
			, m_nTotalQty(0)
			, m_pFirst(nullptr)
			, m_pLast(nullptr)
		{
		}
		~cQueueChunked()
		{
			EmptyQ();
		}

		bool isEmptyQ() const
		{
			return(m_nTotalQty == 0);
		}
		void EmptyQ()
		{
			AdvanceRead(m_nTotalQty);
			ASSERT(m_nTotalQty == 0);
			ASSERT(m_pFirst == nullptr);
			ASSERT(m_pLast == nullptr);
		}

		ITERATE_t get_ReadQtyT() const
		{
			//! get total number of entries
			return m_nTotalQty;
		}
		ITERATE_t get_ReadQtyC() const
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
			ASSERT_N(m_pFirst != nullptr);
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
		ITERATE_t get_WriteQty(void) const
		{
			//! Qty Avail to be written to.
			if (m_pLast == nullptr)
			{
				return m_nGrowSizeChunk;
			}
			return m_pLast->get_WriteQty();
		}
		TYPE* GetWritePrepared(ITERATE_t iDesiredCount = 1)
		{
			//! Use get_WriteQty() to check size.
			//! iDesiredCount of TYPE
			cQueueChunk* pCur = m_pLast;
			if (pCur == nullptr)
			{
				m_pFirst = m_pLast = new cQueueChunk(m_nGrowSizeChunk);
			}
			else if (m_pLast->isFullQ())
			{
				pCur->m_pNext = m_pLast = new cQueueChunk(m_nGrowSizeChunk);
			}
			return m_pLast->GetWritePrepared(iDesiredCount);
		}
		void AdvanceWrite(ITERATE_t iCount = 1)
		{
			// iCount of TYPE
			ASSERT(m_pLast != nullptr);
			ASSERT(iCount <= get_WriteQty());
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

#ifndef GRAY_STATICLIB // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cQueueRead < BYTE >;
	template class GRAYCORE_LINK cQueueRW < BYTE >;
#endif

}	// Gray
#endif // _INC_cQueue_H
