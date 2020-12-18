//
//! @file cFileTextReader.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cFileTextReader.h"

namespace Gray
{
	cFileTextReader::cFileTextReader(size_t nSizeLineMax)
		: cStreamTextReader(m_File, nSizeLineMax)
	{
	}

	cFileTextReader::~cFileTextReader()
	{
	}

	HRESULT cFileTextReader::OpenX(const FILECHAR_t* pszName, OF_FLAGS_t uShareFlags)
	{
		//! Open some existing text file.
		//! @arg uShareFlags = OF_TEXT | OF_CACHE_SEQ
		return m_File.OpenX(pszName, uShareFlags);
	}

	HRESULT cStreamTextReader::ReadStringLine(OUT const char** ppszLine)
	{
		//! Read a line of text. like fgets().
		//! Read up until (including) newline character = \n = The newline character, if read, is included in the string.
		//! like .NET StreamReader.ReadLine
		//! @return
		//!  length of the string read in chars. (includes \r\n) (not including null)
		//!  0 = EOF (legit end of file).
		//!  < 0 = other error

		ITERATE_t iReadAvail = this->get_ReadQty();
		const char* pData = (const char*)this->get_ReadPtr();
		int i = 0;
		for (; ; i++)
		{
			if (i >= iReadAvail)	// run out of data.
			{
				// Try to get more.
				ReadCommitNow();	// pData may be invalidated.

				HRESULT hRes = this->ReadFill();
				if (FAILED(hRes))
					return hRes;

				pData = (const char*)this->get_ReadPtr();

				if (hRes <= 0)	// We have no more data or no more room to read data (line is too long)
				{
					break;
				}
				ASSERT(iReadAvail < this->get_ReadQty());
				iReadAvail = this->get_ReadQty();
			}

			// Find the '\n' EOL 
			char ch = pData[i];
			if (ch == '\n')
			{
				i++;	// include it.
				break;
			}
		}

		AdvanceRead(i);

		// Found a line. return it.
		if (ppszLine != nullptr)
		{
			*ppszLine = pData;
		}
		return i;		// length.
	}

	HRESULT cStreamTextReader::ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) // override // virtual
	{
		//! @arg iSizeMax = Maximum number of characters to be copied into pszBuffer (including room for the the terminating '\0' character).
		//! @return
		//!  length of the string read in chars. (includes \r\n) (not including null)

		if (iSizeMax <= 0)
			return 0;
		const char* pszLine = nullptr;
		HRESULT hRes = ReadStringLine(&pszLine);
		if (FAILED(hRes))
			return hRes;
		size_t nSizeCopy = MIN(hRes, iSizeMax - 1);
		cMem::Copy(pszBuffer, pszLine, nSizeCopy);
		pszBuffer[nSizeCopy] = '\0';
		return (HRESULT)nSizeCopy;
	}

	STREAM_SEEKRET_t cStreamTextReader::Seek(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin) // override;
	{
		//! @arg eSeekOrigin = // SEEK_SET ?
		//! @return the New position, -1=FAILED

		STREAM_SEEKRET_t nPosFile = m_reader.GetPosition();	// WriteIndex
		ITERATE_t iReadQty = this->get_ReadQty();		// what i have buffered.

		switch (eSeekOrigin)
		{
		case SEEK_Cur:
			if (iOffset >= -get_ReadIndex() && iOffset <= iReadQty)
			{
				// Move inside buffer.
				AdvanceRead((ITERATE_t)iOffset);
				return nPosFile + iOffset - iReadQty;
			}
			// outside current buffer.
			this->EmptyQ();
			return m_reader.Seek(nPosFile + iOffset - iReadQty, SEEK_Set);

		case SEEK_End:
			if (iOffset > 0)
				break;
			iOffset += m_reader.GetLength();

			// Fall through.

		case SEEK_Set: // from beginning.
			// Are we inside the current buffer? then just reposition.
			if (iOffset >= (STREAM_OFFSET_t)(nPosFile - get_WriteIndex()) && iOffset <= (STREAM_OFFSET_t)nPosFile)
			{
				// Move inside buffer.
				AdvanceRead((ITERATE_t)(nPosFile - iOffset));
				return iOffset;
			}
			// outside current buffer.
			this->EmptyQ();
			return m_reader.Seek(iOffset, SEEK_Set);

		default:
			break;
		}

		ASSERT(0);
		return((STREAM_POS_t)-1);
	}
}
