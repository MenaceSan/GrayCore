//
//! @file CFileText.cpp
//! see http://www.codeproject.com/file/handles.asp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#include "StdAfx.h"
#include "CFileText.h"
#include "CLogMgr.h"

#if defined(_WIN32) && ! defined(UNDER_CE)
#include <io.h>
#endif
#if ! defined(UNDER_CE)
#include <fcntl.h>		// O_TEXT
#endif

namespace Gray
{
	const CTextPos CTextPos::k_Invalid((STREAM_POS_t)k_ITERATE_BAD, k_ITERATE_BAD, k_StrLen_UNK);
	const CTextPos CTextPos::k_Zero(0, 0, 0);

	StrLen_t CTextPos::GetStr2(OUT char* pszOut, StrLen_t nLenOut) const
	{
		return StrT::sprintfN<char>(pszOut, nLenOut, "O=%d,Line=%d", m_lOffset, m_iLineNum);
	}

	CFileText::CFileText()
		: m_pStream(nullptr)
		, m_iCurLineNum(0)
	{
	}
	CFileText::CFileText(CStringF sFilePath, OF_FLAGS_t nOpenFlags)
		: m_pStream(nullptr)
		, m_iCurLineNum(0)
	{
		OpenX(sFilePath, nOpenFlags);
	}
	CFileText::~CFileText() // virtual
	{
		// Virtuals don't work in destructors !
		Close();
	}

	const FILECHAR_t* CFileText::get_ModeCPtr() const
	{
		//! get the proper fopen() mode arguments.
		UINT nOpenFlags = get_ModeFlags();

		if (nOpenFlags & OF_READWRITE)	// "r+b"
			return _FN("ab+");
		if (nOpenFlags & OF_CACHE_SEQ)
		{
			// Only used sequentially.
			if (isModeWrite())
			{
				return _FN("wbS");
			}
			return _FN("rbS");
		}

		if (!(nOpenFlags & OF_TEXT))	// OF_BINARY
		{
			return((isModeWrite()) ? _FN("wb") : _FN("rb"));
		}

		// text modes. (Never do "\r\n" translation?! it messes up the ftell()
		if (nOpenFlags & OF_CREATE)
			return _FN("w");
		if (isModeWrite())
			return _FN("w");	// "wb+"
		else
			return _FN("r");
	}

	HRESULT CFileText::OpenX(CStringF sFilePath, OF_FLAGS_t nOpenFlags)
	{
		//! Open a text file.
		//! @arg nOpenFlags = OF_READ|OF_WRITE|OF_READWRITE
		HRESULT hRes = OpenSetup(sFilePath, nOpenFlags);
		if (FAILED(hRes))
		{
			return(hRes);
		}
		ASSERT(m_pStream == nullptr);
		const FILECHAR_t* pszMode = get_ModeCPtr();

		// _wfsopen() for wchar_t
#if ! defined(UNDER_CE) && (_MSC_VER >= 1400)
#ifdef USE_UNICODE_FN
		errno_t eRet = ::_wfopen_s(&m_pStream, sFilePath, pszMode);
#else
		errno_t eRet = ::fopen_s(&m_pStream, sFilePath, pszMode);
#endif
#else
#ifdef USE_UNICODE_FN
		m_pStream = ::_wfopen(sFilePath, pszMode);
#else
		m_pStream = ::fopen(sFilePath, pszMode);
#endif
#endif
		if (m_pStream == nullptr)
		{
#if ! defined(UNDER_CE) && (_MSC_VER >= 1400)
			return HResult::FromPOSIX(eRet);
#else
			// Assume HResult::GetLast() was set.
			return(HResult::GetLastDef());
#endif
		}
		sm_iFilesOpen++;
		DEBUG_CHECK(sm_iFilesOpen >= 0);
		m_iCurLineNum = 0;

		// Get the low level handle for it. m_hFile cFile functions use it instead.
		// NOTE: the POSIX 'int' handle (low level file descriptor) is not the same as the OS HANDLE in windows.
#if defined(_MSC_VER)
		FILEDESC_t iFileNo = ::_fileno(m_pStream);
#else
		FILEDESC_t iFileNo = fileno(m_pStream);	// macro
#endif

#if defined(_MFC_VER) && (_MFC_VER <= 0x0600)
		m_hFile = (HFILE) ::_get_osfhandle(iFileNo);
#elif defined(_MFC_VER)
		m_hFile = (HANDLE) ::_get_osfhandle(iFileNo);
#elif defined(__linux__) || defined(UNDER_CE)
		m_hFile.AttachHandle(iFileNo);
#else
		m_hFile.AttachHandle((HANDLE) ::_get_osfhandle(iFileNo)); // _fileno is macro. _MFC_VER
#endif
		return S_OK;
	}

	HRESULT CFileText::OpenFileHandle(HANDLE h, OF_FLAGS_t nOpenFlags)
	{
		// Use _fdopen() to get the backing OSHandle.

		m_nOpenFlags = nOpenFlags;
#ifdef __GNUC__
		m_pStream = ::fdopen((FILEDESC_t)h, StrArg<char>(get_ModeCPtr()));
#else
		FILEDESC_t hConHandle = ::_open_osfhandle((intptr_t)h, O_TEXT); // convert OS HANDLE to C file int handle. _O_TEXT
		m_pStream = ::_fdopen(hConHandle, StrArg<char>(get_ModeCPtr()));
#endif
		if (m_pStream == nullptr)
		{
			return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE); // Why ??
		}
		int iRet = ::setvbuf(m_pStream, nullptr, _IONBF, 0);	// 
		if (iRet != 0)
		{
			// invalid mode parameter or to some other error allocating or assigning the buffer.
			return HResult::GetPOSIXLastDef(E_FAIL);
		}
		sm_iFilesOpen++;
#if defined(_MFC_VER)
		m_hFile = h;
#else
		m_hFile.AttachHandle(h);
#endif
		return S_OK;
	}

	void CFileText::Close()	// virtual
	{
		// virtuals don't work in destruct.
		if (!isFileOpen())
			return;
		if (isModeWrite())
		{
			::fflush(m_pStream);
		}
		bool bSuccess = (::fclose(m_pStream) == 0);
		DEBUG_CHECK(bSuccess);
		UNREFERENCED_PARAMETER(bSuccess);

		DetachFileHandle();
		m_pStream = nullptr;
	}

	STREAM_SEEKRET_t CFileText::Seek(STREAM_OFFSET_t offset, SEEK_ORIGIN_TYPE eSeekOrigin) // virtual
	{
		//! eSeekOrigin = SEEK_SET, SEEK_END
		//! @note end of line translation might be broken? ftell() and fseek() don't work correctly when you use it.
		//! @note offset < 0 for SEEK_Cur is legal.
		//! @return
		//!  <0 = FAILED
		//!  new file pointer position.
		if (!isFileOpen())
			return((STREAM_SEEKRET_t)-1);
		if (::fseek(m_pStream, (long)offset, eSeekOrigin) != 0)
			return((STREAM_SEEKRET_t)-1);
		if (eSeekOrigin == SEEK_Set) // SEEK_SET = FILE_BEGIN
		{
			if (offset == 0)
			{
				m_iCurLineNum = 0; // i actually know the line number for this position.
			}
			return offset;
		}
		m_iCurLineNum = k_ITERATE_BAD;	// invalid. no idea what the line number is now!
		return ::ftell(m_pStream);	// did it really move?
	}

	STREAM_POS_t CFileText::GetPosition() const // virtual
	{
		//! override CStream
		//! @note end of line translation might be broken? ftell and fseek don't work correctly when you use it.
		//! @return -1 = error.
		if (!isFileOpen())
			return (STREAM_POS_t)-1;
		return(::ftell(m_pStream));
	}

	HRESULT CFileText::FlushX() // virtual
	{
		if (!isFileOpen())
			return S_OK;
		ASSERT(m_pStream != nullptr);
		int iRet = ::fflush(m_pStream);
		if (iRet != 0)	// EOF
		{
			return HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
		}
		return S_OK;
	}

	bool CFileText::isEOF() const
	{
		if (!isFileOpen())
			return true;
		return(::feof(m_pStream)) ? true : false;
	}

	HRESULT CFileText::GetStreamError() const
	{
		// errno in M$
		if (!isFileOpen())
			return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
		return HResult::FromPOSIX(::ferror(m_pStream));
	}

	HRESULT CFileText::ReadX(void* pBuffer, size_t nSizeMax) // virtual
	{
		//! CStream
		//! Read a block of binary data or as much as we can until end of file.
		//! @return
		//!  the number of bytes actually read.
		//!  <0 = failed. HRESULT_WIN32_C(ERROR_READ_FAULT) HRESULT_WIN32_C(ERROR_HANDLE_EOF)
		if (pBuffer == nullptr)
			return E_INVALIDARG;
		if (!isFileOpen())
			return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
		if (isEOF())
			return(0);	// LINUX will ASSERT if we read past end.
		size_t uRet = ::fread(pBuffer, 1, nSizeMax, m_pStream);
		ASSERT(uRet >= 0);
		if (uRet > nSizeMax)
		{
			return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_READ_FAULT));
		}
		// m_iCurLineNum++; // no idea.
		return (HRESULT)uRet;	// size we got. 0 = end of file?
	}

	HRESULT CFileText::WriteX(const void* pData, size_t nDataSize) // virtual
	{
		//! CStream
		//! Write binary data to the file.
		//! all or fail.
		//! @return >0 = success else fail.
		if (pData == nullptr)
			return E_INVALIDARG;
		if (!isFileOpen())
			return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
		size_t uRet = ::fwrite(pData, nDataSize, 1, m_pStream);
		if (uRet != 1)
		{
			return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_WRITE_FAULT));
		}
		m_iCurLineNum++;
		return((HRESULT)nDataSize);
	}

	HRESULT CFileText::WriteString(const char* pszStr)	// virtual
	{
		//! override CStream
		//! @note If we did fgets() it will translate the \n for us.
		//!  so we must do the same on the output side.
		//! like .NET StreamWriter.WriteLine()
		//! @return <0 = failed.
		//!  Length of write in chars

		if (pszStr == nullptr)
			return E_INVALIDARG;
		if (!isFileOpen())
			return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);

		if (::fputs(pszStr, m_pStream) < 0)
		{
			return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_WRITE_FAULT));
		}
		return 1;
	}

#if 0
	HRESULT CFileText::ReadStringLine(wchar_t* pszBuffer, StrLen_t iSizeMax) // virtual
	{
		//! override CStream
		//! Read a line of text. like fgets()
		//! @return length of the string read in chars. (includes \r\n) (not including null)
		ASSERT(0);
		return E_NOTIMPL;
	}
#endif

	HRESULT CFileText::ReadStringLine(char* pszBuffer, StrLen_t iSizeMax) // virtual
	{
		//! override CStream
		//! Read a line of text. like fgets().
		//! Read up until (including) newline character = \n = The newline character, if read, is included in the string.
		//! like .NET StreamReader.ReadLine
		//! @return length of the string read in chars. (includes \r\n) (not including null)
		//!  0 = EOF (legit end of file).
		//!  < 0 = other error

		if (pszBuffer == nullptr)
			return E_INVALIDARG;
		if (!isFileOpen())
			return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
		if (isEOF())
		{
			return 0;	// __linux__ will ASSERT if we read past end.
		}
		char* pszRet = ::fgets(pszBuffer, iSizeMax, m_pStream);
		if (pszRet == nullptr)
		{
			if (isEOF())	// legit end of file
			{
				return 0;
			}
			return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_READ_FAULT));
		}
		if (m_iCurLineNum >= 0)
			m_iCurLineNum++;
		return StrT::Len(pszBuffer);	// Return length in chars.
	}

	HRESULT CFileText::ReadStringLineA(cStringA& r)
	{
		//! Read an ASCII or UTF8 string/line from the file.
		//! Read to the end of the single line.
		char szTmp[StrT::k_LEN_MAX];
		HRESULT hRes = ReadStringLine(szTmp, STRMAX(szTmp));
		if (FAILED(hRes))
		{
			return hRes;
		}
		r = szTmp;
		return hRes;
	}

	bool CFileText::put_TextPos(const CTextPos& rPos)
	{
		// FILE_BEGIN == SEEK_SET
		if (!rPos.isValidPos())
			return false;
		if (Seek((STREAM_OFFSET_t)rPos.get_Offset(), SEEK_Set) != (STREAM_SEEKRET_t)rPos.get_Offset())
			return false;
		m_iCurLineNum = rPos.get_LineNum();
		return true;
	}
}

//*****************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CMime.h"

UNITTEST_CLASS(CFileText)
{
	UNITTEST_METHOD(CFileText)
	{
		//! @todo test reading CFileText and seek position inside it. Must not be fooled by \r\n and \n.
		CTextPos fp1(CTextPos::k_Invalid);
		fp1 = CTextPos::k_Zero;

		CStringF sFilePathB = CFilePath::CombineFilePathX(get_TestOutDir(), _FN(GRAY_NAMES) _FN("CFileTextB") _FN(MIME_EXT_txt));
		cFile testfileB;
		HRESULT hRes = testfileB.OpenX(sFilePathB, OF_CREATE | OF_WRITE | OF_BINARY);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		hRes = testfileB.WriteString(CUnitTests::k_sTextBlob.get_CPtr());
		//UNITTEST_TRUE(hRes == CUnitTests::k_TEXTBLOB_LEN);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		CStringF sFilePathT = CFilePath::CombineFilePathX(get_TestOutDir(), _FN(GRAY_NAMES) _FN("CFileTextT") _FN(MIME_EXT_txt));

		CFileText testfileT;
		hRes = testfileT.OpenX(sFilePathT, OF_CREATE | OF_WRITE | OF_BINARY);
		UNITTEST_TRUE(SUCCEEDED(hRes));

		hRes = testfileT.WriteString(CUnitTests::k_sTextBlob.get_CPtr());
		UNITTEST_TRUE(SUCCEEDED(hRes));
		//UNITTEST_TRUE(hRes == CUnitTests::k_TEXTBLOB_LEN);
	}
};
UNITTEST_REGISTER(CFileText, UNITTEST_LEVEL_Core);
#endif
