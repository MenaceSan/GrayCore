//
//! @file CFileCopier.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CFileCopier.h"
#include "CFileDir.h"

namespace Gray
{

	HRESULT GRAYCALL CFileCopier::CopyFileStream(CStreamInput& stmIn, const FILECHAR_t* pszDstFileName, bool bFailIfExists, IStreamProgressCallback* pProgress)
	{
		//! Copy this (opened OF_READ) file to some other file name/path. (pszDstFileName)
		//! manually read/copy the contents of the file via WriteStream().
		//! Similar effect to cFile::CopyFileX()

		// ASSUME SeekToBegin(); if appropriate.

		HRESULT hRes;
		cFile fileDst;
		if (bFailIfExists)
		{
			// Check if it exists first.
			hRes = fileDst.OpenX(pszDstFileName, OF_READ | OF_BINARY | OF_EXIST);
			if (FAILED(hRes))
			{
				return hRes;	// can't overwrite!
			}
			fileDst.Close();
		}

		hRes = fileDst.OpenX(pszDstFileName, OF_WRITE | OF_BINARY | OF_CREATE);
		if (FAILED(hRes))
		{
			return hRes;
		}

		hRes = fileDst.WriteStream(stmIn, fileDst.GetLength(), pProgress);
		if (FAILED(hRes))
		{
			return hRes;
		}

		return S_OK;
	}

#if defined(_WIN32) && ! defined(UNDER_CE)
	DWORD CALLBACK CFileCopier::onCopyProgressCallback(
		LARGE_INTEGER TotalFileSize,
		LARGE_INTEGER TotalBytesTransferred,
		LARGE_INTEGER StreamSize,
		LARGE_INTEGER StreamBytesTransferred,
		DWORD dwStreamNumber,
		DWORD dwCallbackReason,	// CALLBACK_CHUNK_FINISHED or CALLBACK_STREAM_SWITCH
		HANDLE hSourceFile,
		HANDLE hDestinationFile,
		void* lpData)	// static
	{
		//! CopyProgressRoutine Callback Function forwarded to IStreamProgressCallback
		//! http://msdn.microsoft.com/en-us/library/aa363854(VS.85).aspx
		//! LPPROGRESS_ROUTINE called by CopyFileStream() in the CopyFileEx() case
		//! @return PROGRESS_STOP
		UNREFERENCED_PARAMETER(hDestinationFile);
		UNREFERENCED_PARAMETER(hSourceFile);
		UNREFERENCED_PARAMETER(dwCallbackReason);
		UNREFERENCED_PARAMETER(dwStreamNumber);
		UNREFERENCED_PARAMETER(StreamBytesTransferred);
		UNREFERENCED_PARAMETER(StreamSize);
		if (lpData != nullptr)
		{
			ASSERT(TotalBytesTransferred.QuadPart <= 0xffffffff);
			ASSERT(TotalFileSize.QuadPart <= 0xffffffff); // truncation!
			HRESULT hRes = ((IStreamProgressCallback*)lpData)->onProgressCallback(
				CStreamProgress((STREAM_POS_t)TotalBytesTransferred.QuadPart, (STREAM_POS_t)TotalFileSize.QuadPart));
			if (FAILED(hRes))
			{
				return PROGRESS_STOP;
			}
		}
		return PROGRESS_CONTINUE;
	}
#endif

	HRESULT GRAYCALL CFileCopier::CopyFileX(const FILECHAR_t* pszExistingName, const FILECHAR_t* pszNewName, IStreamProgressCallback* pProgress, bool bFailIfExists) // static
	{
		//! OS Copy a file from pszExistingName to pszNewName. The pszNewName may or may not already exist.
		//! @return
		//!  ERROR_REQUEST_ABORTED = canceled by callback.

#if defined(_WIN32) && ! defined(UNDER_CE)
		bool bRet;
		if (pProgress != nullptr)
		{
			BOOL fCancel = false;
			bRet = ::CopyFileExW(
				CFilePath::GetFileNameLongW(pszExistingName),	// fix long name problems.
				CFilePath::GetFileNameLongW(pszNewName),
				CFileCopier::onCopyProgressCallback,
				pProgress,
				&fCancel,
				bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0);
		}
		else
		{
			bRet = ::CopyFileW(CFilePath::GetFileNameLongW(pszExistingName), CFilePath::GetFileNameLongW(pszNewName), bFailIfExists);
		}
		if (!bRet)
		{
			HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
			return(hRes);
		}
		return S_OK;

#else
		// copyfile() ?? in C++17
		// sendfile()  will send up to 2G

		// Fallback to stream file.
		cFile filesrc;
		HRESULT hRes = filesrc.OpenX(pszExistingName, OF_READ | OF_BINARY);
		if (FAILED(hRes))
			return hRes;
		return filesrc.CopyFileStream(pszNewName, bFailIfExists, pProgress);

#endif
	}

	HRESULT GRAYCALL CFileCopier::RenamePath(const FILECHAR_t* lpszOldName, const FILECHAR_t* lpszNewName, IStreamProgressCallback* pProgress) // static
	{
		//! Equivalent of moving a file. (or directory and its children)
		//! A new directory must be on the same drive.
		//! @note Can't move file from once device to another! without MOVEFILE_COPY_ALLOWED
		//! @return
		//!  S_OK = 0 = good.
		//!  <0 = failed.

		bool bRet;
#if defined(__linux__)
		bRet = (::rename(lpszOldName, lpszNewName) == 0);
#elif defined(UNDER_CE) || defined(__GNUC__)
		bRet = ::MoveFileW(StrArg<wchar_t>(lpszOldName), StrArg<wchar_t>(lpszNewName));
#else
		bRet = ::MoveFileWithProgressW(CFilePath::GetFileNameLongW(lpszOldName), CFilePath::GetFileNameLongW(lpszNewName),
			CFileCopier::onCopyProgressCallback,
			pProgress,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED
		);
#endif
		if (!bRet)
		{
			return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
		}
		return S_OK;
	}

	HRESULT CFileCopier::RequestFile(const FILECHAR_t* pszSrcName, const FILECHAR_t* pszDestPath, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t* pnRequestSizeEst) // virtual 
	{
		//! Request a file from a m_sServerRoot/pszSrcName to be brought back to me at local pszDestPath.
		//! @arg pnRequestSizeEst = unused/unnecessary for local file system copy.

		CStringF sSrcPath = makeFilePath(pszSrcName);
		bool bRequestSize = (pnRequestSizeEst != nullptr && *pnRequestSizeEst == (FILE_SIZE_t)-1);
		bool bDestEmpty = StrT::IsWhitespace(pszDestPath);
		if (bDestEmpty || bRequestSize)
		{
			// just retrieve the size of the file in pnRequestSizeEst using cFileStatus
			cFileStatus fs;
			HRESULT hRes = fs.ReadFileStatus(sSrcPath);
			if (FAILED(hRes))
				return hRes;
			if (pnRequestSizeEst == nullptr)
				return E_INVALIDARG;
			// return its size.
			*pnRequestSizeEst = fs.GetFileLength();
			if (bDestEmpty)
				return S_OK;
		}
		if (nOffsetStart != 0)
		{
			// A partial copy of the file. CopyFileStream
			ASSERT(0);
		}
		return CFileCopier::CopyFileX(sSrcPath, pszDestPath, pProgress, false);
	}

	HRESULT CFileCopier::SendFile(const FILECHAR_t* pszSrcPath, const FILECHAR_t* pszDestName, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t nSize) // override virtual 
	{
		//! Send a local file to a m_sServerRoot/pszDestName from local pszSrcPath storage
		//! @note I cannot set the modification time stamp for the file here.

		UNREFERENCED_PARAMETER(nSize);
		if (StrT::IsWhitespace(pszDestName))
		{
			return E_INVALIDARG;
		}
		CStringF sDestPath = makeFilePath(pszDestName);
		if (StrT::IsWhitespace(pszSrcPath))
		{
			// Acts like a delete. delete file or directory recursively.
			return CFileDir::DeletePathX(sDestPath, 0);
		}
		if (nOffsetStart != 0)
		{
			// A partial copy of the file. CopyFileStream
			ASSERT(0);
		}
		return CFileCopier::CopyFileX(pszSrcPath, sDestPath, pProgress, false);
	}

	HRESULT CFileCopier::SendAttr(const FILECHAR_t* pszDestName, CTimeFile timeChanged) // override virtual
	{
		//! Optionally set the remote side time stamp for a file.
		return cFileStatus::WriteFileTimes(makeFilePath(pszDestName), &timeChanged, &timeChanged);
	}
}

//***********************************************
#if USE_UNITTESTS
#include "CUnitTest.h"
namespace Gray
{
	void GRAYCALL CFileCopier::UnitTest_CopyTo(IFileCopier* pDst)
	{
		//! For testing an implementation of IFileCopier
		UNREFERENCED_PARAMETER(pDst);
	}
	void GRAYCALL CFileCopier::UnitTest_CopyFrom(IFileCopier* pSrc)
	{
		//! For testing an implementation of IFileCopier
		UNREFERENCED_PARAMETER(pSrc);
	}
}

#endif
