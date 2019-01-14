//
//! @file CStreamQueue.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CStreamQueue_H
#define _INC_CStreamQueue_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CStream.h"
#include "CQueue.h"
#include "HResult.h"

namespace Gray
{
	class GRAYCORE_LINK CStreamQueue : public CStream, public cQueueBytes
	{
		//! @class Gray::CStreamQueue
		//! Read and write to/from a dynamic memory CStream.
		//! Grow the cQueueBytes memory allocation as needed.
		//! similar to CFileMem, System.IO.MemoryStream

		typedef cQueueBytes SUPER_t;

	public:
		CStreamQueue(size_t nGrowSizeChunk = 4 * 1024, size_t nGrowSizeMax = CHeap::k_ALLOC_MAX)
			: cQueueBytes(nGrowSizeChunk, nGrowSizeMax)
		{
			//! @arg nGrowSizeMax = 0 = not used. write only ?
		}
		CStreamQueue(const CStreamQueue& a)
		{
			//! do nothing!
			UNREFERENCED_REFERENCE(a);
		}
		~CStreamQueue()
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
			//! similar to ReadCommit (put_AutoReadCommitSize) size. Used by CStreamTransaction.
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

	class GRAYCORE_LINK CStreamStatic : public CStream, public cQueueRW < BYTE >
	{
		//! @class Gray::CStreamStatic
		//! Read and write to a single preallocated memory block as CStream.
		//! Data block is pre-allocated and provided.
		//! DO NOT grow the memory allocation if needed.

		typedef cQueueRW<BYTE> SUPER_t;
	public:
		explicit CStreamStatic()
			: cQueueRW<BYTE>(nullptr, 0, 0, 0, 0)
		{
			//! Empty
			//! Read Only 0.
		}
		explicit CStreamStatic(void* pData, size_t iDataMax, size_t iReadLast, size_t iWriteLast, size_t iAutoReadCommit = 0)
			: cQueueRW<BYTE>((BYTE*)pData, (ITERATE_t)iDataMax, (ITERATE_t)iReadLast, (ITERATE_t)iWriteLast, (ITERATE_t)iAutoReadCommit)
		{
			//! Read/Write
		}
		explicit CStreamStatic(const void* pData, size_t iDataMax)
			: cQueueRW<BYTE>((const BYTE*)pData, (ITERATE_t)iDataMax)
		{
			//! Used to serve a memory string as a CStream. AKA StringStream or StreamString.
			//! Read Only iDataMax.
		}
		explicit CStreamStatic(const CMemBlock& m)
			: cQueueRW<BYTE>((const BYTE*)m.get_Start(), (ITERATE_t)m.get_Size())
		{
			//! Used to serve a memory string as a CStream. AKA StringStream or StreamString.
			//! Read Only m.get_Size().
		}
		CStreamStatic(const CStreamStatic& a)
		{
			//! do nothing! copy ??
			UNREFERENCED_REFERENCE(a);
		}
		~CStreamStatic()
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

	class GRAYCORE_LINK CStreamStringA : public CStreamStatic
	{
		//! @class Gray::CStreamStringA
		//! Build a string as a stream based on stack allocated/inline memory. CStreamOutput
		//! Similar to StrBuilder.  CStreamQueue
		//! equiv to STL string builder. std::stringstream
		typedef CStreamStatic SUPER_t;
	private:
		char m_szVal[StrT::k_LEN_MAX];	//!< Preallocated as MAX size.
	public:
		CStreamStringA()
			: CStreamStatic(m_szVal, STRMAX(m_szVal), 0, 0)
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

#endif	// _INC_CStreamQueue_H
