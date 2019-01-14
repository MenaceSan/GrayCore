//
//! @file CFile.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#include "StdAfx.h"
#include "CFile.h"
#include "CHeap.h"
#include "CString.h"
#include "CLogMgr.h"
#include "CFileDir.h"
#include "CTimeSys.h"
#include "Ptr.h"

#ifdef __linux__
#include "CTimeVal.h"
#include <sys/stat.h>
#include <sys/time.h>
#else
#include <shellapi.h>	// FOF_RENAMEONCOLLISION
#endif

namespace Gray
{
	ITERATE_t cFile::sm_iFilesOpen = 0;	// debug statistical info

#ifndef _MFC_VER
// CFile::hFileNull

	HRESULT CFile::OpenCreate(CStringF sFilePath, OF_FLAGS_t nOpenFlags, _SECURITY_ATTRIBUTES* pSa)
	{
		//! Open a file handle.
		//! 1. OF_EXIST = just test if this file exists, don't really open it.
		//! 2. OF_READ = open for read. file must exist.
		//! 3. OF_WRITE = open for over write. file must exist unless OF_CREATE is set.
		//! 4. OF_READWRITE = open file for write append. file must exist unless OF_CREATE is set.
		//! | OF_CREATE = create. wipe any existing file.
		//! | OF_TEXT = this is really a FILE* handle on "t" mode. else OF_BINARY
		//! | OF_CACHE_SEQ = we read this sequentially only. no seek will be used. FILE_FLAG_SEQUENTIAL_SCAN
		//! OF_READWRITE | OF_CREATE = Open for write but create if it does not already exist.

		UNREFERENCED_PARAMETER(pSa);

#ifdef _WIN32
		// NOTE: FILE_ATTRIBUTE_HIDDEN and OF_CREATE will create a hidden file!
		DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;	// FILE_ATTRIBUTE_READONLY
		DWORD dwDesiredAccess = GENERIC_READ;
		switch (nOpenFlags & (OF_READ | OF_WRITE | OF_READWRITE))
		{
		case OF_WRITE:
			dwDesiredAccess = GENERIC_WRITE;
			break;
		case OF_READWRITE:
			dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
			dwFlagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;	// for open of directories.
			break;
		}

		// NOTE: FILE_SHARE_DELETE doesn't work for 98/ME
		DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;	// OF_SHARE_COMPAT|FILE_SHARE_DELETE
		switch (nOpenFlags & (OF_SHARE_COMPAT | OF_SHARE_EXCLUSIVE | OF_SHARE_DENY_WRITE | OF_SHARE_DENY_READ | OF_SHARE_DENY_NONE))
		{
		case OF_SHARE_COMPAT:
		case OF_SHARE_DENY_NONE:
			dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE; // |FILE_SHARE_DELETE
			break;
		case OF_SHARE_EXCLUSIVE:
			dwShareMode = 0;
			break;
		case OF_SHARE_DENY_WRITE:
			dwShareMode = FILE_SHARE_READ;
			break;
		case OF_SHARE_DENY_READ:
			dwShareMode = FILE_SHARE_WRITE; // |FILE_SHARE_DELETE
			break;
		}

		DWORD dwCreationDisposition = OPEN_EXISTING; // CREATE_NEW | OPEN_EXISTING
		if (nOpenFlags & OF_CREATE)
		{
			// Create file with dwFlagsAndAttributes
			switch (nOpenFlags & (OF_READ | OF_WRITE | OF_READWRITE))
			{
			case OF_READWRITE:
			case OF_READ:
				dwCreationDisposition = OPEN_ALWAYS; // append if exists.
				break;
			case OF_WRITE:
				dwCreationDisposition = CREATE_ALWAYS; // truncate if exist.
				break;
			}
		}
		else
		{
			// Named file with any attributes??
			// dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN;
		}
		if (nOpenFlags & OF_EXIST)
		{
			dwCreationDisposition = OPEN_EXISTING;
		}
		if (nOpenFlags & OF_CACHE_SEQ)
		{
			dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
		}

		m_hFile.AttachHandle(::CreateFileW(CFilePath::GetFileNameLongW(sFilePath), dwDesiredAccess,
			dwShareMode, nullptr, dwCreationDisposition,
			dwFlagsAndAttributes, nullptr));

#elif defined(__linux__)
		// http://linux.die.net/man/2/open
		// flags must include one of the following access modes: O_RDONLY, O_WRONLY, or O_RDWR
		// ASSUME nOpenFlags = Linux OpenFlags; O_RDONLY, O_CREAT, O_APPEND, etc.
		// O_NOFOLLOW = don't follow the symbolic link. (if a link)
		UINT uFlags = (nOpenFlags & OF_OPEN_MASK);
		UINT uMode = 0;
		if (nOpenFlags & OF_CREATE)
		{
			uFlags |= O_TRUNC;
			uMode = S_IRWXU | S_IRWXG; // S_IRWXO
		}
		if (nOpenFlags & OF_CACHE_SEQ)
		{
			uFlags |= O_DIRECT;
		}
		m_hFile.OpenHandle(sFilePath, uFlags, uMode);
#else
#error NOOS
#endif

		if (!isFileOpen())
		{
			// E_ACCESSDENIED for a hidden file?
			return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
		}
		return S_OK;
	}

	STREAM_SEEKRET_t CFile::Seek(STREAM_OFFSET_t lOffset, SEEK_ORIGIN_TYPE eSeekOrigin) // virtual
	{
		//! Change or get the current file position pointer.
		//! Compatible with MFC definition.
		//! Might be 'const' but MFC wont allow that
		//! @arg eSeekOrigin = // SEEK_SET ?
		//! @return the New position, -1=FAILED
		if (!isFileOpen())
		{
			return((STREAM_POS_t)-1);
		}
		return m_hFile.Seek(lOffset, eSeekOrigin);
	}

	STREAM_POS_t CFile::GetPosition() const // virtual
	{
		//! Get the current read position in the file.
		//! Seek( 0, SEEK_CUR ) like MFC
		//! Use _tell( m_hFile ) for __linux__ ?
		if (!isFileOpen())
		{
			return((STREAM_POS_t)-1);
		}
		return m_hFile.Seek(0, SEEK_Cur);
	}

	STREAM_POS_t CFile::GetLength() const // virtual
	{
		//! Get the size of the open file in bytes. like MFC
		//! @return <0 = error. (or directory?)
		if (!isFileOpen())
		{
			return((STREAM_POS_t)-1);	// E_HANDLE
		}
#ifdef _WIN32
#ifdef USE_FILE_POS64
		LARGE_INTEGER FileSize;
		bool bRet = ::GetFileSizeEx(m_hFile, &FileSize);
		if (!bRet)
		{
			return((STREAM_POS_t)-1);
		}
		return((STREAM_POS_t)FileSize.QuadPart);
#else
		return ::GetFileSize(m_hFile, nullptr);
#endif // defined(_MFC_VER) && ( _MFC_VER > 0x0600 )

#elif defined(__linux__)
		CFileStatusSys filedata;
		if (::fstat(m_hFile, &filedata) != 0)
		{
			return HResult::GetLastDef(E_HANDLE);
		}
		return(filedata.st_size);
#endif
	}

	void CFile::SetLength(STREAM_SEEKRET_t dwNewLen) // virtual
	{
		//! Grow/Shrink the file.
		//! Stupid MFC has void return. use HResult::GetLast()
		ASSERT(isFileOpen());
#ifdef _WIN32
		Seek(dwNewLen, SEEK_Set);	// SetFilePointer()
		if (!::SetEndOfFile(m_hFile))
		{
			// ASSUME HResult::GetLast() set.
			DEBUG_ERR(("cFile::SetLength %d ERR='%s'", dwNewLen, LOGERR(HResult::GetLast())));
			// CFileException::ThrowOsError( HResult::GetLastDef(), m_strFileName);
		}
		else
		{
			::SetLastError(NO_ERROR);
		}
#elif defined(__linux__)
		::ftruncate(m_hFile, dwNewLen);
#endif
	}

	HRESULT CFile::Write(const void* pData, size_t nDataSize)
	{
		//! Write a block to the stream. advance the current position. Like WriteX
		//! @return length written. <0 = FAILED
		//!   ERROR_INVALID_USER_BUFFER = too many async calls ?? wait
		//!   ERROR_IO_PENDING = must wait!?

		HRESULT nLengthWritten = m_hFile.WriteX(pData, nDataSize);
		if (FAILED(nLengthWritten))
		{
			if (nLengthWritten == E_HANDLE)	// handle is junk!! No idea why. Clear it.
			{
				Close();
			}
			DEBUG_ASSERT(nLengthWritten == (HRESULT)nDataSize, "Write");
		}
		return (HRESULT)nLengthWritten;
	}

#endif // ! _MFC_VER

	//***************************************************************************
	// -cFile

	CStringF cFile::get_FileTitleX() const
	{
		//! Get file name and ext
		//! Don't use MFC GetFileTitle() since MFC does not include EXT
		return CFilePath::GetFileName(get_FilePath());
	}

	CStringF cFile::get_FileExt() const
	{
		//! get the EXTension including the .
		//! Must replace the stupid MFC version of this.
		return(CFilePath::GetFileNameExt(get_FilePath(), get_FilePath().GetLength()));
	}

	bool cFile::IsFileExt(const FILECHAR_t* pszExt) const
	{
		//! is the pszExt a match?
		return CFilePath::IsFileNameExt(get_FilePath(), pszExt);
	}

	HRESULT cFile::OpenSetup(CStringF sFilePath, OF_FLAGS_t nOpenFlags)
	{
		//! Internal function to set internal params.
		//! Similar to SetFilePath() in MFC ?
		//! @arg nOpenFlags = OF_BINARY | OF_WRITE
		//! @return
		//!  S_FALSE = success , already open.
		//!  S_OK = success

		if (sFilePath.IsEmpty() || !get_FilePath().CompareNoCase(sFilePath))
		{
			// Name is already set ? and already open ?
			if (isFileOpen() && m_nOpenFlags == nOpenFlags)
			{
				ASSERT(!get_FilePath().IsEmpty());
				return S_FALSE;
			}
		}

		Close();	// Make sure it's closed first. re-using this structure.

		if (!sFilePath.IsEmpty())
		{
			m_strFileName = sFilePath;
		}

		m_nOpenFlags = nOpenFlags;

		// DEBUG_TRACE(( "Open file '%s' flags=0%x", LOGSTR(m_strFileName), nOpenFlags ));
		return S_OK;
	}

	HRESULT cFile::OpenCreate(CStringF sFilePath, OF_FLAGS_t nOpenFlags, _SECURITY_ATTRIBUTES* pSa)
	{
		//! Open a file by name.
		//! @arg nOpenFlags = OF_READ | OF_WRITE | OF_READWRITE
		//! Expect the file pointer to be at 0!
		UNREFERENCED_PARAMETER(pSa);

		HRESULT hRes = OpenSetup(sFilePath, nOpenFlags);
		if (hRes != S_OK)
		{
			// hRes == S_FALSE = no change. already open.
			if (FAILED(hRes))
			{
				return hRes;
			}
			ASSERT(isFileOpen());
			SeekToBegin();	// act like a new open()
			return S_OK;
		}

		ASSERT(!isFileOpen());
#ifdef _MFC_VER
		if (!CFile::Open(m_strFileName, nOpenFlags))	// doesn't use pSa
		{
			hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
		}
#else
		hRes = CFile::OpenCreate(m_strFileName, nOpenFlags, nullptr);
#endif
		if (hRes == HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND) && (nOpenFlags & OF_CREATE))
		{
			// must create the stupid path first.
			hRes = CFileDir::CreateDirForFileX(m_strFileName);
			if (SUCCEEDED(hRes))
			{
#ifdef _MFC_VER
				if (!CFile::Open(m_strFileName, nOpenFlags))
				{
					hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
				}
#else
				hRes = CFile::OpenCreate(m_strFileName, nOpenFlags, nullptr);
#endif
			}
		}
		if (FAILED(hRes))
		{
			return hRes;
		}
		sm_iFilesOpen++;
		DEBUG_CHECK(sm_iFilesOpen >= 0);
		return S_OK;
	}

	HRESULT cFile::OpenX(CStringF sFilePath, OF_FLAGS_t nOpenFlags) // virtual
	{
		return OpenCreate(sFilePath, nOpenFlags, nullptr);
	}

	HRESULT cFile::OpenWait(CStringF sFilePath, OF_FLAGS_t nOpenFlags, TIMESYSD_t nTimeWait)
	{
		//! Try to open the file. Wait for a bit if it fails to open.
		//! If the file is locked because 'access is denied' then just wait and keep trying.

		CTimeSys tStart(CTimeSys::GetTimeNow());
		for (int iTries = 0;; iTries++)
		{
			HRESULT hRes = OpenX(sFilePath, nOpenFlags);	// use OpenX which is the virtual.
			if (hRes == S_OK)
			{
				break;
			}

			// failed to open
			if (hRes != HRESULT_WIN32_C(ERROR_ACCESS_DENIED))	// E_ACCESSDENIED
			{
				return hRes;
			}

#ifdef _WIN32
			// try to make the file writable if marked as read only ?
			if (iTries == 0 && (nOpenFlags&OF_WRITE))
			{
				hRes = cFileStatus::WriteFileAttributes(sFilePath, FILEATTR_Normal);
				if (FAILED(hRes))
					return hRes;
				continue;	// just try again without waiting.
			}
#endif
			if (iTries > 0 && tStart.get_AgeSys() >= nTimeWait)
			{
				return hRes;
			}

			// maybe its just being synced by the system? wait a bit.
			TIMESYSD_t nTimeWaitInt = MIN(100, nTimeWait);
			CThreadId::SleepCurrent(nTimeWaitInt);
		}
		return S_OK;
	}

	void cFile::Close() // virtual
	{
		//! CStream
		if (!isFileOpen())
			return;
		sm_iFilesOpen--;
		DEBUG_CHECK(sm_iFilesOpen >= 0);
		CFile::Close();	// NOT __super from CStream
	}

	HANDLE cFile::DetachFileHandle()
	{
		if (!isFileOpen())
		{
			return INVALID_HANDLE_VALUE; //  CFile::hFileNull;
		}
		sm_iFilesOpen--;
		DEBUG_CHECK(sm_iFilesOpen >= 0);
#ifdef _MFC_VER
		HANDLE h = m_hFile;
		m_hFile = CFile::hFileNull;
#else
		HANDLE h = m_hFile.DetachHandle();	// fclose() did all the work. Dont do anything with this handle.
#endif
		return h;
	}

	bool cFile::SetFileTime(const CTimeFile* lpCreationTime, const CTimeFile* lpAccessTime, const CTimeFile* lpLastWriteTime)
	{
		//! Set the time access for an open file.
		//! lpAccessTime = can be null.
		//! @note lpLastWriteTime is the only time guaranteed to work on all systems.
		//! @note FAT32 lpLastWriteTime is only accurate to 2 seconds !!
		//! Similar to CFile::SetStatus() and __linux__ utime()
		//! @return true = OK;

		if (!isFileOpen())
		{
			return false;
		}
#ifdef _WIN32
		return ::SetFileTime(m_hFile,
			lpCreationTime,
			lpAccessTime,
			lpLastWriteTime);
#elif defined(__linux__)
		// NOTE: __linux__ can't set lpCreationTime?
		CTimeVal tv[2];
		if (lpAccessTime == nullptr || lpLastWriteTime == nullptr)
		{
			// must get defaults.
			CFileStatusSys st;
			if (!::fstat(m_hFile, &st))
			{
				tv[0].tv_sec = st.st_atime;
				tv[1].tv_sec = st.st_mtime;
				// st.st_ctime; // ??
			}
		}
		if (lpAccessTime != nullptr)
		{
			tv[0] = lpAccessTime->get_TimeVal();
		}
		if (lpLastWriteTime != nullptr)
		{
			tv[1] = lpLastWriteTime->get_TimeVal();
		}
		if (::futimes(m_hFile, tv) == -1)
		{
			return false; // HResult::GetPOSIXLast(); ?
		}
		return true;
#endif
	}

	bool cFile::SetFileTime(CTimeInt timeCreation, CTimeInt timeLastWrite)
	{
		//! Use HResult::GetLastDef() to find out why this fails.
		//! @return true = OK;
		if (!timeLastWrite.isTimeValid())
		{
			return false;
		}
		CTimeFile LastWriteTime = timeLastWrite.GetAsFileTime();
		CTimeFile CreationTime = timeCreation.GetAsFileTime();
#ifdef _DEBUG
		if (timeLastWrite == timeCreation)
		{
			ASSERT(!CMem::Compare(&CreationTime, &LastWriteTime, sizeof(LastWriteTime)));
		}
#endif
		return SetFileTime(&CreationTime, nullptr, &LastWriteTime);
	}

	HRESULT cFile::GetFileStatus(cFileStatus& attr) const
	{
		//! Get the file status info by its open handle
		//! Similar to CFile::GetStatus()
		//! Same as CFileDir::ReadFileStatus()

		if (!isFileOpen())
		{
			return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
		}

#ifdef _WIN32
		BY_HANDLE_FILE_INFORMATION fi;
		if (!::GetFileInformationByHandle(m_hFile, &fi))
		{
			return HResult::GetLastDef(E_HANDLE);
		}

		attr.m_timeCreate = fi.ftCreationTime;			// m_ctime  = (may not be supported).
		attr.m_timeChange = fi.ftLastWriteTime;			// m_mtime = real world time/date of last modification. (accurate to 2 seconds)
		attr.m_timeLastAccess = fi.ftLastAccessTime;	// m_atime = time of last access/Open. (For Caching). (may not be supported)
		attr.m_Size = fi.nFileSizeLow;					// file size in bytes.
		if (fi.nFileSizeHigh != 0)
		{
			attr.m_Size |= ((FILE_SIZE_t)fi.nFileSizeHigh) << 32;
		}
		attr.m_Attributes = fi.dwFileAttributes;		// Mask of ATTR_ attribute bits. ATTR_Normal

#elif defined(__linux__)

		CFileStatusSys filestat;
		if (::fstat(m_hFile, &filestat) != 0)
		{
			return HResult::GetLastDef(E_HANDLE);
		}
		attr.InitFileStatus(filestat);
#endif

		return S_OK;
	}

	HRESULT cFile::ReadX(void* pData, size_t nDataSize) // virtual - disambiguate.
	{
		//! Read a block from the stream. advance the current position.
		//! @return
		//!  length of the read data. 0 or HRESULT_WIN32_C(ERROR_HANDLE_EOF) = end of file.

		if (nDataSize <= 0)
		{
			return 0;
		}
		if (pData == nullptr)
		{
			// just skip it.
			STREAM_SEEKRET_t nRet = Seek(nDataSize, SEEK_Cur);
			if (nRet <= 0)
			{
				return 0;
				// return HResult::GetLastDef();
			}
			return (HRESULT)nDataSize;
		}

#if defined(_MFC_VER)
		// catch throw ?
		UINT nLengthRead = __super::Read(pData, (UINT)nDataSize);
		if (nLengthRead <= 0)
		{
			HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_READ_FAULT));
			// GetLastError() = ERROR_HANDLE_EOF = No more data ?
			return hRes;
		}
		return nLengthRead;
#else
		return m_hFile.ReadX(pData, nDataSize);
#endif
	}

	HRESULT cFile::WriteX(const void* pData, size_t nDataSize) // virtual - disambiguate.
	{
		//! Write a block to the stream. advance the current position.
		//! @arg
		//!  pData == nullptr = just test if i could write this much.
		//! @return
		//!  length written.
		//!  <0 = FAILED
		if (nDataSize == 0)	// nothing to do.
		{
			return 0;
		}
		if (pData == nullptr)
		{
			ASSERT(pData != nullptr);
			return E_POINTER;
		}
#if defined(_MFC_VER) && ( _MFC_VER > 0x0600 )
		// NOTE: _MFC_VER >= v7 returns void from Write!!
		__super::Write(pData, (UINT)nDataSize);
		return (HRESULT) nDataSize;
#else
		return CFile::Write(pData, nDataSize);
#endif
	}

	HRESULT cFile::FlushX() // virtual
	{
		//! synchronous flush of write data to file.
		if (!isFileOpen())
			return S_OK;
#ifdef _MFC_VER
		CFile::Flush();	// catch throw ?
#else
		HRESULT hRes = m_hFile.FlushX();
		if (FAILED(hRes))
		{
			DEBUG_WARN(("File Flush failed"));
			// CFileException::ThrowOsError( HResult::GetLast(), m_strFileName);
			return hRes;
		}
#endif
		return S_OK;
	}

	HRESULT cFile::CopyFileStream(const FILECHAR_t* pszDstFileName, bool bFailIfExists, IStreamProgressCallback* pProgress)
	{
		//! Copy this (opened OF_READ) file to some other file name/path. (pszDstFileName)
		//! manually read/copy the contents of the file via WriteStream().
		//! Similar effect to cFile::CopyFileX()

		if (!isFileOpen())	// should already be open for reading.
		{
			return HRESULT_WIN32_C(ERROR_OPEN_FAILED);
		}
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

		hRes = fileDst.WriteStream(*this, fileDst.GetLength(), pProgress);
		if (FAILED(hRes))
		{
			return hRes;
		}

		return S_OK;
	}

	//**************************************************************

#if defined(_WIN32) && ! defined(UNDER_CE)
	DWORD CALLBACK cFile::onCopyProgressCallback(
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

	HRESULT GRAYCALL cFile::CopyFileX(const FILECHAR_t* pszExistingName, const FILECHAR_t* pszNewName, IStreamProgressCallback* pProgress, bool bFailIfExists) // static
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
				cFile::onCopyProgressCallback,
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

	HRESULT GRAYCALL cFile::RenamePath(const FILECHAR_t* lpszOldName, const FILECHAR_t* lpszNewName, IStreamProgressCallback* pProgress) // static
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
			cFile::onCopyProgressCallback,
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

	HRESULT GRAYCALL cFile::DeletePath(const FILECHAR_t* pszFilePath) // static
	{
		//! Use 'DeletePath' name because windows uses a "#define" macro to overload DeleteFile()!
		//! Same as MFC CFile::Remove()
		//! @note This can't be used with wildcards or to delete folders ! use CFileDir::DeleteDirFiles for that.
		//! @return
		//!  S_OK = 0.
		//!  S_FALSE = not here, but thats probably OK.
		//!  <0 = HRESULT failed for some other reason. (ERROR_ACCESS_DENIED,ERROR_PATH_NOT_FOUND)
		//!  Cant delete a directory this way !
#ifdef _WIN32
		if (!::DeleteFileW(CFilePath::GetFileNameLongW(pszFilePath)))
#else
	// same as ::unlink(). ENOENT or EACCES
		if (::remove(pszFilePath) != 0)
#endif
		{
			HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
			if (hRes == HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND))
			{
				return S_FALSE;
			}
			return hRes;
	}
		return S_OK;
	}

	HRESULT GRAYCALL cFile::DeletePathX(const FILECHAR_t* pszPath, DWORD nFileFlags) // static
	{
		//! Delete this file and fix collisions and read only marking.
		HRESULT hRes = cFile::DeletePath(pszPath);
#ifdef _WIN32
		if (hRes == E_ACCESSDENIED && (nFileFlags & FOF_RENAMEONCOLLISION))
		{
			// remove read only flag. retry delete.
			// Try to change the attributes. then try delete again.
			hRes = cFileStatus::WriteFileAttributes(pszPath, FILEATTR_Normal);
			if (SUCCEEDED(hRes))
			{
				hRes = cFile::DeletePath(pszPath);
			}
		}
#endif
		return hRes;
	}

	HRESULT GRAYCALL cFile::LoadFile(const FILECHAR_t* pszFilePath, OUT CHeapBlock& block, size_t nSizeExtra) // static
	{
		//! Read file into memory.
		//! @arg nSizeExtra = allocate some extra space at end.
		//! @return size read (Not including nSizeExtra). or <0 = error.
		cFile file;
		HRESULT hRes = file.OpenX(pszFilePath, OF_READ);
		if (FAILED(hRes))
			return hRes;
		hRes = file.ReadAll(block, nSizeExtra);
		if (FAILED(hRes))
			return hRes;
		// Zero the extra (if any);
		ASSERT(hRes == (HRESULT)(block.get_Size() - nSizeExtra));
		if (nSizeExtra > 0)
		{
			block.get_DataBytes()[hRes] = 0;	// terminator.
		}
		return hRes;
	}

	//**********************************************************

	HRESULT cFileCopy::RequestFile(const FILECHAR_t* pszSrcName, const FILECHAR_t* pszDestPath, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t* pnRequestSizeEst) // virtual 
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
		return cFile::CopyFileX(sSrcPath, pszDestPath, pProgress, false);
	}

	HRESULT cFileCopy::SendFile(const FILECHAR_t* pszSrcPath, const FILECHAR_t* pszDestName, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t nSize) // override virtual 
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
		return cFile::CopyFileX(pszSrcPath, sDestPath, pProgress, false);
	}

	HRESULT cFileCopy::SendAttr(const FILECHAR_t* pszDestName, CTimeFile timeChanged) // override virtual
	{
		//! Optionally set the remote side time stamp for a file.
		return cFileStatus::WriteFileTimes(makeFilePath(pszDestName), &timeChanged, &timeChanged);
	}

	}

//*****************************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CMime.h"

namespace Gray
{
	void GRAYCALL cFile::UnitTest_Write(CStreamOutput& testfile1) // static
	{
		//! Write strings to it.
		for (ITERATE_t i = 0; !CUnitTests::k_asTextLines[i].isNull(); i++)
		{
			HRESULT hRes = testfile1.WriteString(CUnitTests::k_asTextLines[i].get_CPtr());
			UNITTEST_TRUE(SUCCEEDED(hRes));
			testfile1.WriteString(_GT(STR_NL));
		}
	}

	void GRAYCALL cFile::UnitTest_Read(CStreamInput& testfile2, bool bString) // static
	{
		//! Other side of UnitTest_Write()
		//! Read strings from it (as binary).
		GChar_t szTmp[256];

		for (ITERATE_t j = 0; !CUnitTests::k_asTextLines[j].isNull(); j++)
		{
			const GChar_t* pszLine = CUnitTests::k_asTextLines[j];
			StrLen_t iLenStr = StrT::Len(pszLine);
			UNITTEST_TRUE(iLenStr < (StrLen_t)STRMAX(szTmp));
			size_t nSizeBytes = (iLenStr + 1) * sizeof(GChar_t);
			HRESULT hResRead = bString ? testfile2.ReadStringLine(szTmp, STRMAX(szTmp)) : testfile2.ReadX(szTmp, nSizeBytes);
			UNITTEST_TRUE(hResRead == (HRESULT)(bString ? (iLenStr + 1) : nSizeBytes));
			UNITTEST_TRUE(!CMem::Compare(szTmp, pszLine, iLenStr * sizeof(GChar_t)));	// pszLine has no newline.
			UNITTEST_TRUE(szTmp[iLenStr] == '\n');
		}

		// Check for proper read past end of file.
		HRESULT hResRead = testfile2.ReadX(szTmp, STRMAX(szTmp));
		UNITTEST_TRUE(hResRead == 0);
		hResRead = testfile2.ReadX(szTmp, STRMAX(szTmp));
		UNITTEST_TRUE(hResRead == 0);
	}

	void GRAYCALL cFile::UnitTest_CopyTo(IFileCopy* pDst)
	{
		//! For testing an implementation of IFileCopy
		UNREFERENCED_PARAMETER(pDst);
	}
	void GRAYCALL cFile::UnitTest_CopyFrom(IFileCopy* pSrc)
	{
		//! For testing an implementation of IFileCopy
		UNREFERENCED_PARAMETER(pSrc);
	}
}

UNITTEST_CLASS(cFile)
{
	UNITTEST_METHOD(cFile)
	{
		//! Create a test file.
		HRESULT hRes;
		CStringF sFilePath = CFilePath::CombineFilePathX(get_TestOutDir(), _FN(GRAY_NAMES) _FN("CoreUnitTestFile") _FN(MIME_EXT_txt));

		{
			cFile testfile1;
			hRes = testfile1.OpenX(sFilePath, OF_CREATE | OF_WRITE | OF_BINARY);
			UNITTEST_TRUE(SUCCEEDED(hRes));
			cFile::UnitTest_Write(testfile1);
		}

		// Read it back.
		for (int i = 0; i < 2; i++)
		{
			cFile testfile2;
			hRes = testfile2.OpenX(sFilePath, OF_READ | OF_BINARY);
			UNITTEST_TRUE(SUCCEEDED(hRes));
			cFileStatus filestatus2;
			hRes = testfile2.GetFileStatus(filestatus2);
			UNITTEST_TRUE(SUCCEEDED(hRes));
			cFile::UnitTest_Read(testfile2, (bool)i);
		}

		// Fail to delete directory.
		hRes = cFile::DeletePath(get_TestOutDir());
		UNITTEST_TRUE(hRes == E_ACCESSDENIED);	// this should fail! E_ACCESSDENIED=WIN32
	}
};
UNITTEST_REGISTER(cFile, UNITTEST_LEVEL_Core);	// UNITTEST_LEVEL_Core
#endif
