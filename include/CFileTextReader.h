//
//! @file cFileTextReader.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cFileTextReader_H
#define _INC_cFileTextReader_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CFile.h"
#include "cStreamStack.h"
#include "cTextPos.h"

UNITTEST_PREDEF(cFileTextReader)

namespace Gray
{
	class GRAYCORE_LINK cFileTextReader : public cStreamStackInp
	{
		//! @class Gray::cFileTextReader 
		//! read text lines from a buffer / stream.
		//! Replace the FILE* streaming file i/o reader fread() with something more under our control.
		//! Try to use this instead of cFileText.
		//! Allow control of read buffer size and line length.
		//! m_nGrowSizeMax = max line size.

	public:
		cFile m_File;			// The backing OS file.

	protected:
		virtual HRESULT ReadX(void* pData, size_t nDataSize) override
		{
			// Use ReadStringLine instead.
			ASSERT(0);
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(nDataSize);
			return E_NOTIMPL;
		}
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override
		{
			// Read ONLY.
			ASSERT(0);
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(nDataSize);
			return E_NOTIMPL;
		}

	public:
		cFileTextReader(size_t nSizeLineMax = CStream::k_FILE_BLOCK_SIZE * 2);
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

		HRESULT ReadStringLine(OUT const char** ppszLine);
		virtual HRESULT ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) override;

		virtual STREAM_SEEKRET_t Seek(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) override;

		UNITTEST_FRIEND(cFileTextReader);
	};
}

#endif
