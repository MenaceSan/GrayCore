//
//! @file cFileTextReader.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cFileTextReader_H
#define _INC_cFileTextReader_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cFile.h"
#include "cStreamStack.h"
#include "cTextPos.h"

namespace Gray
{
	class GRAYCORE_LINK cStreamTextReader : public cStreamStackInp
	{
		//! @class Gray::cFileTextReader 
		//! read text lines from a buffer / stream.
		//! Allow control of read buffer size and line length.
		//! Faster than cStreamInput::ReadStringLine() since it buffers ? maybe ?
		//! m_nGrowSizeMax = max line size.

		cStreamInput& m_reader;

	protected:
		cStreamTextReader(cStreamInput& reader, size_t nSizeLineMax)
			: cStreamStackInp(&reader, nSizeLineMax)
			, m_reader(reader)
		{
			// Max buffer size = max line length.
			this->put_AutoReadCommit((ITERATE_t)(nSizeLineMax / 2));		// default = half buffer.
		}

	protected:
		virtual HRESULT ReadX(void* pData, size_t nDataSize) override
		{
			// Use ReadStringLine instead. Prevent use of this.
			ASSERT(0);
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(nDataSize);
			return E_NOTIMPL;
		}
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override
		{
			// Read ONLY. Prevent use of this.
			ASSERT(0);
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(nDataSize);
			return E_NOTIMPL;
		}

		HRESULT ReadStringLine(OUT const char** ppszLine);

	public:
		HRESULT ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) override;

		HRESULT SeekX(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) override;
	};

	class GRAYCORE_LINK cFileTextReader : public cStreamTextReader
	{
		//! @class Gray::cFileTextReader 
		//! read text lines from a file stream.
		//! Try to use this instead of cFileText. 
		//! Replace the FILE* streaming file i/o reader fread() with something more under our control.

	public:
		cFile m_File;			// The backing OS file.

	public:
		cFileTextReader(size_t nSizeLineMax = cStream::k_FILE_BLOCK_SIZE * 2);
		virtual ~cFileTextReader();

		HRESULT OpenX(const FILECHAR_t* pszName, OF_FLAGS_t uShareFlags);

		virtual STREAM_POS_t GetLength() const override
		{
			return m_File.GetLength();
		}
		virtual void Close(void)
		{
			m_File.Close();
		}
		virtual STREAM_POS_t GetPosition() const override
		{
			return m_File.GetPosition() - this->get_ReadQty();
		}

		UNITTEST_FRIEND(cFileTextReader);
	};
}

#endif
