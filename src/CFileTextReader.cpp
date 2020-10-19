//
//! @file CFileTextReader.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CFileTextReader.h"

namespace Gray
{
	CFileTextReader::CFileTextReader(size_t nSizeLineMax)
		: CStreamStackInp(&m_File, nSizeLineMax)
	{
		// Max buffer size = max lien length.
		this->put_AutoReadCommit((ITERATE_t)(nSizeLineMax / 2));		// default = half buffer.
	}

	CFileTextReader::~CFileTextReader()
	{
	}

	HRESULT CFileTextReader::OpenX(const FILECHAR_t* pszName, OF_FLAGS_t uShareFlags)
	{
		//! Open some existing text file.
		//! @arg uShareFlags = OF_TEXT | OF_CACHE_SEQ
		return m_File.OpenX(pszName, uShareFlags);
	}

	HRESULT CFileTextReader::ReadStringLine(OUT const char** ppszLine) 
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

	HRESULT CFileTextReader::ReadStringLine(OUT char* pszBuffer, StrLen_t iSizeMax) // override // virtual
	{
		const char* pszRet;
		HRESULT hRes = ReadStringLine(&pszRet);
		if (FAILED(hRes))
			return hRes;
		iSizeMax = MIN(hRes, iSizeMax);
		CMem::Copy(pszBuffer, pszRet, iSizeMax);
		return iSizeMax;
	}

	STREAM_SEEKRET_t CFileTextReader::Seek(STREAM_OFFSET_t iOffset, SEEK_ORIGIN_TYPE eSeekOrigin) // override;
	{
		//! @arg eSeekOrigin = // SEEK_SET ?
		//! @return the New position, -1=FAILED

		STREAM_SEEKRET_t nPosFile = m_File.GetPosition();	// WriteIndex
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
			return m_File.Seek(nPosFile + iOffset - iReadQty, SEEK_Set);

		case SEEK_End:
			if (iOffset > 0)
				break;
			iOffset += m_File.GetLength();

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
			return m_File.Seek(iOffset, SEEK_Set);

		default:
			break;
		}

		ASSERT(0);
		return((STREAM_POS_t)-1);
	}
}

//*****************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CMime.h"

UNITTEST_CLASS(CFileTextReader)
{
	UNITTEST_METHOD(CFileTextReader)
	{
		//! test reading CFileTextReader.
		//! @note any text changes to this file can invalidate the test results.

		CStringF sFilePath = CFilePath::CombineFilePathX(get_TestInpDir(), _FN(GRAY_NAMES) _FN("Core/src/CFileTextReader.cpp"));	// Find myself. __FILE__

		static const int k_MaxLineLen = 180;	// was CStream::k_FILE_BLOCK_SIZE 256. Assume no other line is this long for my test.

		CFileTextReader tr(k_MaxLineLen);
		HRESULT hRes = tr.OpenX(sFilePath, OF_READ | OF_TEXT | OF_SHARE_DENY_NONE | OF_CACHE_SEQ);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		int iLineNumber = 1;	// 1 based.
		for (;; )
		{
			const char* pszLine = nullptr;
			hRes = tr.ReadStringLine(&pszLine);
			UNITTEST_TRUE(SUCCEEDED(hRes));
			if (hRes == 0)
				break;

			//*** Make this over k_MaxLineLen chars long ****************************************************************************************************************************************************

			if (hRes >= k_MaxLineLen)
			{
				// Warning  line length was too long !
				UNITTEST_TRUE(iLineNumber == 167);	// Fix this if source changes.
				DEBUG_MSG(("line %d length was > %d", iLineNumber, hRes));
			}
			else
			{
				iLineNumber++;	// don't count split lines.
			}

			UNITTEST_TRUE(hRes <= k_MaxLineLen);
		}

		UNITTEST_TRUE(iLineNumber == 188);	// Fix this if source changes.
	}
};
UNITTEST_REGISTER(CFileTextReader, UNITTEST_LEVEL_Core);
#endif
