//
//! @file cStreamQueue.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cStreamQueue_H
#define _INC_cStreamQueue_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cStream.h"
#include "cQueue.h"
#include "HResult.h"

namespace Gray
{
	class GRAYCORE_LINK cStreamQueue : public CStream, public cQueueBytes
	{
		//! @class Gray::cStreamQueue
		//! Read and write to/from a dynamic memory CStream.
		//! Grow the cQueueBytes memory allocation as needed.
		//! similar to CFileMem, System.IO.MemoryStream

		typedef cQueueBytes SUPER_t;

	public:
		cStreamQueue(size_t nGrowSizeChunk = 4 * 1024, size_t nGrowSizeMax = cHeap::k_ALLOC_MAX)
			: cQueueBytes(nGrowSizeChunk, nGrowSizeMax)
		{
			//! @arg nGrowSizeMax = 0 = not used. write only ?
		}
		cStreamQueue(const cStreamQueue& a)
		{
			//! do nothing!
			UNREFERENCED_REFERENCE(a);
		}
		~cStreamQueue()
		{
		}

		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override
		{
			return SUPER_t::WriteX(pData, nDataSize);
		}
		virtual HRESULT ReadX(void* pData, size_t nDataSize) override
		{
			return SUPER_t::ReadX(pData, nDataSize);
		}
		virtual HRESULT ReadPeek(void* pData, size_t nDataSize) override
		{
			return SUPER_t::ReadPeek((BYTE*)pData, (ITERATE_t)nDataSize);
		}
		virtual size_t SetSeekSizeMin(size_t nSizeMin = k_FILE_BLOCK_SIZE) override
		{
			//! similar to ReadCommit (put_AutoReadCommitSize) size. Used by cStreamTransaction.
			//! @arg nSizeMin = 0 = turn off auto read commit. Allow Seek() back.
			//! @return previous value for get_AutoReadCommit.
			ITERATE_t iAutoReadCommit = this->get_AutoReadCommit();
			this->put_AutoReadCommit((ITERATE_t)nSizeMin);
			return (size_t)iAutoReadCommit;
		}
		virtual STREAM_SEEKRET_t Seek(STREAM_OFFSET_t offset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) override
		{
			return SUPER_t::SeekQ(offset, eSeekOrigin);
		}
		virtual STREAM_POS_t GetLength() const override
		{
			//! Get the full Seek-able length. line Seek(SEEK_END).
			return this->get_WriteIndex();	// get_ReadQty()
		}
	};

	class GRAYCORE_LINK cStreamStatic : public CStream, public cQueueRW < BYTE >
	{
		//! @class Gray::cStreamStatic
		//! Read and write to a single preallocated memory block as CStream.
		//! Data block is pre-allocated and provided.
		//! DO NOT grow the memory allocation if needed.

		typedef cQueueRW<BYTE> SUPER_t;
	public:
		explicit cStreamStatic()
			: cQueueRW<BYTE>(nullptr, 0, 0, 0, 0)
		{
			//! Empty
			//! Read Only 0.
		}
		explicit cStreamStatic(void* pData, size_t iDataMax, size_t iReadLast, size_t iWriteLast, size_t iAutoReadCommit = 0)
			: cQueueRW<BYTE>((BYTE*)pData, (ITERATE_t)iDataMax, (ITERATE_t)iReadLast, (ITERATE_t)iWriteLast, (ITERATE_t)iAutoReadCommit)
		{
			//! Read/Write
		}
		explicit cStreamStatic(const void* pData, size_t iDataMax)
			: cQueueRW<BYTE>((const BYTE*)pData, (ITERATE_t)iDataMax)
		{
			//! Used to serve a memory string as a CStream. AKA StringStream or StreamString.
			//! Read Only iDataMax.
		}
		explicit cStreamStatic(const cMemBlock& m)
			: cQueueRW<BYTE>((const BYTE*)m.get_Start(), (ITERATE_t)m.get_Size())
		{
			//! Used to serve a memory string as a CStream. AKA StringStream or StreamString.
			//! Read Only m.get_Size().
		}
		cStreamStatic(const cStreamStatic& a)
		{
			//! do nothing! copy ??
			UNREFERENCED_REFERENCE(a);
		}
		~cStreamStatic()
		{
		}

		virtual size_t SetSeekSizeMin(size_t nSizeMin) override
		{
			UNREFERENCED_PARAMETER(nSizeMin);
			return 0;	// Indicate this does nothing.
		}
		virtual STREAM_SEEKRET_t Seek(STREAM_OFFSET_t offset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) override
		{
			return SUPER_t::SeekQ(offset, eSeekOrigin);
		}
		virtual STREAM_POS_t GetLength() const override
		{
			//! Get the full Seek-able length. not just get_ReadQty() left to read. Assume Seek(0) read length.
			return SUPER_t::get_WriteIndex();
		}

		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override
		{
			return SUPER_t::WriteX(pData, nDataSize);
		}
		virtual HRESULT ReadX(void* pData, size_t nDataSize) override
		{
			return SUPER_t::ReadX(pData, nDataSize);
		}
		virtual HRESULT ReadPeek(void* pData, size_t nDataSize) override
		{
			return SUPER_t::ReadPeek((BYTE*)pData, (ITERATE_t)nDataSize);
		}
	};

	class GRAYCORE_LINK cStreamStringA : public cStreamStatic
	{
		//! @class Gray::cStreamStringA
		//! Build a string as a stream based on stack allocated/inline memory. cStreamOutput
		//! Similar to StrBuilder.  cStreamQueue
		//! equiv to STL string builder. std::stringstream
		typedef cStreamStatic SUPER_t;
	private:
		char m_szVal[StrT::k_LEN_MAX];	//!< Preallocated as MAX size.
	public:
		cStreamStringA()
			: cStreamStatic(m_szVal, STRMAX(m_szVal), 0, 0)
		{
			// Read/Write
			m_szVal[0] = '\0';
		}
		StrLen_t get_StrLen() const
		{
			return (StrLen_t)SUPER_t::get_WriteIndex();
		}
		char* ref_StrA()
		{
			// Terminate string.
			StrLen_t nLen = get_StrLen();
			m_szVal[nLen] = '\0';
			return m_szVal;
		}
		operator char* ()
		{
			return ref_StrA();
		}
	};
}

#endif	// _INC_cStreamQueue_H
