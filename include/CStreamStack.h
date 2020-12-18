//
//! @file cStreamStack.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cStreamStack_H
#define _INC_cStreamStack_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cStreamQueue.h"

namespace Gray
{
	class GRAYCORE_LINK cStreamStackInp : public cStreamQueue // cStreamInput, protected cStreamStack
	{
		//! @class Gray::cStreamStackInp
		//! Stack of input streams. Acts like a codec, decompressor, decipher, etc.
		//! This input stream will grab data and process it from some other stream. holding it for when someone calls this->ReadX()
		//! ASSUME derived class overrides ReadX and calls ReadFill
	protected:
		cStreamInput* m_pStreamInp;	//!< source input stream. called by ReadFill()

	protected:
		HRESULT ReadFill();
		HRESULT ReadFillAligned(size_t nSizeBlockAlign = 1);

	public:
		cStreamStackInp(cStreamInput* pStreamInp = nullptr, size_t nSizeMaxBuffer = cStream::k_FILE_BLOCK_SIZE) noexcept
			: cStreamQueue(MIN(nSizeMaxBuffer / 2, 8 * 1024), nSizeMaxBuffer)	// chunk size, max size.
			, m_pStreamInp(pStreamInp)
		{
			ASSERT(get_AutoReadCommit() > 0);
		}

		virtual HRESULT ReadX(void* pData, size_t nDataSize) override = 0;	// MUST be overridden. and call ReadFill() at some point.
	};

	class GRAYCORE_LINK cStreamStackOut : public cStreamQueue // cStreamOutput, protected cStreamStack
	{
		//! @class Gray::cStreamStackOut
		//! Stack of output streams. Acts like a codec, compressor, cipher, etc.
		//! This output stream will process data and push it along to another output stream via m_pOut->WriteX().
		//! @note WriteX() MUST take all data passed to it and cQueueRW it up if it cant process immediately.
		//! ASSUME derived class overrides WriteX and calls WriteFlush
	protected:
		cStreamOutput* m_pStreamOut;	//!< End result output stream. called by WriteFlush()

	protected:
		HRESULT WriteFlush();

	public:
		cStreamStackOut(cStreamOutput* pStreamOut = nullptr, size_t nSizeBuffer = cStream::k_FILE_BLOCK_SIZE)
			: cStreamQueue(8 * 1024, nSizeBuffer)
			, m_pStreamOut(pStreamOut)
		{
		}

		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override = 0; // cStreamOutput override calls WriteFlush() // MUST be overridden
	};

	class GRAYCORE_LINK cStreamStackPackets : public cStreamStackOut
	{
		//! @class Gray::cStreamStackPackets
		//! Stream out to a cStreamOutput that might not take anything but whole packets.
		//! call m_pStreamOut->WriteX() multiple times for multiple whole packets.
		//! save unfinished packets in m_buffer.
		//! nSizeBuffer = the size of the largest possible whole packet.

	public:
		cStreamStackPackets(cStreamOutput* pStreamOut = nullptr, size_t nSizeBuffer = cStream::k_FILE_BLOCK_SIZE)
			: cStreamStackOut(pStreamOut, nSizeBuffer)
		{
		}
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override;
	};
};

#endif
