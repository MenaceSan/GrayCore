//
//! @file CStreamStack.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CStreamStack_H
#define _INC_CStreamStack_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CStreamQueue.h"

namespace Gray
{
	class GRAYCORE_LINK CStreamStackInp : public CStreamQueue // CStreamInput, protected CStreamStack
	{
		//! @class Gray::CStreamStackInp
		//! Stack of input streams. Acts like a codec, decompressor, decipher, etc.
		//! This input stream will grab data and process it from some other stream. holding it for when someone calls this->ReadX()
		//! ASSUME derived class overrides ReadX and calls ReadFill
	protected:
		CStreamInput* m_pStreamInp;	//!< source input stream. called by ReadFill()

	protected:
		HRESULT ReadFill();
		HRESULT ReadFillAligned(size_t nSizeBlockAlign = 1);

	public:
		CStreamStackInp(CStreamInput* pStreamInp = nullptr, size_t nSizeMaxBuffer = CStream::k_FILE_BLOCK_SIZE)
			: CStreamQueue(MIN(nSizeMaxBuffer / 2, 8 * 1024), nSizeMaxBuffer)	// chunk size, max size.
			, m_pStreamInp(pStreamInp)
		{
			ASSERT(get_AutoReadCommit() > 0);
		}

		virtual HRESULT ReadX(void* pData, size_t nDataSize) override = 0;	// MUST be overridden. and call ReadFill() at some point.
	};

	class GRAYCORE_LINK CStreamStackOut : public CStreamQueue // CStreamOutput, protected CStreamStack
	{
		//! @class Gray::CStreamStackOut
		//! Stack of output streams. Acts like a codec, compressor, cipher, etc.
		//! This output stream will process data and push it along to another output stream via m_pOut->WriteX().
		//! @note WriteX() MUST take all data passed to it and cQueueRW it up if it cant process immediately.
		//! ASSUME derived class overrides WriteX and calls WriteFlush
	protected:
		CStreamOutput* m_pStreamOut;	//!< End result output stream. called by WriteFlush()

	protected:
		HRESULT WriteFlush();

	public:
		CStreamStackOut(CStreamOutput* pStreamOut = nullptr, size_t nSizeBuffer = CStream::k_FILE_BLOCK_SIZE)
			: CStreamQueue(8 * 1024, nSizeBuffer)
			, m_pStreamOut(pStreamOut)
		{
		}

		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override = 0; // CStreamOutput override calls WriteFlush() // MUST be overridden
	};

	class GRAYCORE_LINK CStreamStackPackets : public CStreamStackOut
	{
		//! @class Gray::CStreamStackPackets
		//! Stream out to a CStreamOutput that might not take anything but whole packets.
		//! call m_pStreamOut->WriteX() multiple times for multiple whole packets.
		//! save unfinished packets in m_buffer.
		//! nSizeBuffer = the size of the largest possible whole packet.

	public:
		CStreamStackPackets(CStreamOutput* pStreamOut = nullptr, size_t nSizeBuffer = CStream::k_FILE_BLOCK_SIZE)
			: CStreamStackOut(pStreamOut, nSizeBuffer)
		{
		}
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override;
	};
};

#endif
