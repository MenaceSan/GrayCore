//
//! @file cFileTextReader.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cFileTextReader_H
#define _INC_cFileTextReader_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cStreamStack.h"
#include "cTextPos.h"

namespace Gray
{
	class GRAYCORE_LINK cStreamTextReader : public cStreamStackInp
	{
		//! @class Gray::cFileTextReader 
		//! read text lines from a buffer / stream. Similar to FILE*
		//! Allow control of read buffer size and line length.
		//! Faster than cStreamInput::ReadStringLine() since it buffers ? maybe ?
		//! m_nGrowSizeMax = max line size.

	private:
		cStreamInput& m_reader;
		ITERATE_t m_iCurLineNum;	//!< track the line number we are on currently. (0 based) (for cTextPos)

	public:
		cStreamTextReader(cStreamInput& reader, size_t nSizeLineMax)
			: cStreamStackInp(&reader, nSizeLineMax)
			, m_reader(reader)
			, m_iCurLineNum(0)
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

	public:
		inline ITERATE_t get_CurrentLineNumber() const noexcept
		{
			return m_iCurLineNum;
		}

		HRESULT ReadStringLine(OUT const char** ppszLine);
		HRESULT ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) override;

		HRESULT SeekX(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin = SEEK_Set) override;
	};
}

#endif
