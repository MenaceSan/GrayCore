//
//! @file CFileTextReader.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CFileTextReader_H
#define _INC_CFileTextReader_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CFile.h"
#include "CStreamStack.h"
#include "CTextPos.h"

UNITTEST_PREDEF(CFileTextReader)

namespace Gray
{
	class GRAYCORE_LINK CFileTextReader : public CStreamStackInp
	{
		//! @class Gray::CFileTextReader 
		//! Replace the FILE* streaming file i/o reader. fread() with something more under our control.
		//! Try to use this instead of CFileText.
		//! Allow control of read buffer size and line length.
		//! m_nGrowSizeMax = max line size.

	public:
		cFile m_File;			// The backing OS file.

	protected:
		virtual HRESULT ReadX(void* pData, size_t nDataSize) override
		{
			ASSERT(0);
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(nDataSize);
			return E_NOTIMPL;
		}
		virtual HRESULT WriteX(const void* pData, size_t nDataSize) override
		{
			ASSERT(0);
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(nDataSize);
			return E_NOTIMPL;
		}

	public:
		CFileTextReader(size_t nSizeLineMax = CStream::k_FILE_BLOCK_SIZE * 2);
		virtual ~CFileTextReader();

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

		UNITTEST_FRIEND(CFileTextReader);
	};

}

#endif
