//
//! @file cFileStatus.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cFileStatus.h"
#include "cFile.h"

#ifdef __linux__
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#endif

namespace Gray
{
	cFileStatus::cFileStatus()
		: m_Size((FILE_SIZE_t)-1)
		, m_Attributes(0)
	{
		ASSERT(!isFileValid());
	}
	cFileStatus::cFileStatus(const FILECHAR_t* pszFilePath)
		: m_Size((FILE_SIZE_t)-1)
		, m_Attributes(0)
	{
		//! @note use isFileValid() to find if this is valid.
		ASSERT(!isFileValid());
		ReadFileStatus(pszFilePath);
	}

	void cFileStatus::InitFileStatus()
	{
		m_timeCreate.InitTime();
		m_timeChange.InitTime(); // All OS support this.
		m_timeLastAccess.InitTime();
		m_Size = (FILE_SIZE_t)-1;	// Set to an invalid value.
		m_Attributes = 0;
		ASSERT(!isFileValid());
	}

	void cFileStatus::InitFileStatus(const cFileStatusSys& statusSys)
	{
		//! convert from OS native format.
#ifdef _WIN32
		m_timeCreate = statusSys.ftCreationTime; // cTimeFile
		m_timeChange = statusSys.ftLastWriteTime;
		m_timeLastAccess = statusSys.ftLastAccessTime;
		m_Size = statusSys.nFileSizeLow;
		if (statusSys.nFileSizeHigh != 0)
		{
			m_Size |= ((FILE_SIZE_t)statusSys.nFileSizeHigh) << 32;
		}
		m_Attributes = (FILEATTR_MASK_t)statusSys.dwFileAttributes; // truncated!
#elif defined(__linux__)
		// http://linux.die.net/man/2/stat
		// hidden file start with .
		m_timeCreate = cTimeInt(statusSys.st_ctime).GetAsFileTime(); // time_t
		m_timeChange = cTimeInt(statusSys.st_mtime).GetAsFileTime();
		m_timeLastAccess = cTimeInt(statusSys.st_atime).GetAsFileTime();
		m_Size = statusSys.st_size;
		m_Attributes = 0; // check the read,write,execute bits?
		if (S_ISREG(statusSys.st_mode))
		{
			m_Attributes |= FILEATTR_Normal;
		}
		else if (S_ISDIR(statusSys.st_mode))
		{
			m_Attributes |= FILEATTR_Directory;
		}
		else if (S_ISLNK(statusSys.st_mode))
		{
			m_Attributes |= FILEATTR_Link;
		}
		else	// S_ISBLK, S_ISSOCK, S_ISCHR, S_ISFIFO
		{
			m_Attributes |= FILEATTR_Volume;	// device of some sort.
		}
#endif
	}

#if defined(__linux__)
	HRESULT GRAYCALL cFileStatus::GetStatusSys(OUT cFileStatusSys& statusSys, const FILECHAR_t* pszName, bool bFollowLinks) // static 
	{
		// https://man7.org/linux/man-pages/man2/stat.2.html
		int iRet = (bFollowLinks) ? ::lstat(sFileName, &statusSys) : ::stat(sFileName, &statusSys);
		if (iRet != 0)
		{
			return HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
		}
		return S_OK;
	}
#endif

	HRESULT GRAYCALL cFileStatus::WriteFileAttributes(const FILECHAR_t* pszFilePath, FILEATTR_MASK_t dwAttributes) // static
	{
		//! Set attributes for a NON open file.
		//! dwAttributes = FILEATTR_Hidden, FILEATTR_ReadOnly
#ifdef _WIN32
		if (!::SetFileAttributesW(cFilePath::GetFileNameLongW(pszFilePath), dwAttributes))
		{
			return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
		}
#elif 0 // defined(__linux__)
	// TODO __linux__ fchmod() to set file permissions.
	// convert FILEATTR_MASK_t to Linux bits
		ASSERT(0);
		if (::chmod(pszFilePath, dwAttributes) != 0)
		{
			return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
		}
#endif
		return S_OK;
	}

	HRESULT GRAYCALL cFileStatus::WriteFileTimes(const FILECHAR_t* pszFilePath, const cTimeFile* pTimeCreate, const cTimeFile* pTimeChange) // static
	{
		//! Update the created/changed time for a file. (by name) (similar to 'touch' command)
		//! May have varying levels of support for OS, FAT, NTFS, NFS, etc
		cFile file;
		HRESULT hRes = file.OpenX(pszFilePath, OF_READWRITE | OF_BINARY); // OPEN_EXISTING
		if (FAILED(hRes))
		{
			return hRes;
		}
		bool bRet = file.SetFileTime(pTimeCreate, nullptr, pTimeChange);
		if (!bRet)
		{
			return HResult::GetLastDef(E_HANDLE);
		}
		return S_OK;
	}

	HRESULT GRAYCALL cFileStatus::WriteFileTimes(const FILECHAR_t* pszFilePath, const cFileStatus& rFileStatus) // static 
	{
		return WriteFileTimes(pszFilePath, &(rFileStatus.m_timeCreate), &(rFileStatus.m_timeChange));
	}

	HRESULT GRAYCALL cFileStatus::ReadFileStatus2(const FILECHAR_t* pszFilePath, cFileStatus* pFileStatus, bool bFollowLink) // static
	{
		//! get info/attributes/status on a single file or dir.
		//! Similar to the MFC CFileFind. Are wildcards allowed ??
		//! @note cFilePath::IsFilePathRoot will fail.
		//! @arg pFileStatus is allowed to be nullptr
		//! @return S_OK

#ifdef _WIN32
		// NOTE: same as _WIN32 GetFileAttributesEx()
		WIN32_FIND_DATAW statusSys;	// cFileStatusSys
		HANDLE hContext = ::FindFirstFileW(cFilePath::GetFileNameLongW(pszFilePath), &statusSys);
		if (hContext == INVALID_HANDLE_VALUE)
		{
			// DEBUG_ERR(( "Can't open input dir [%s]", LOGSTR(pszFilePath) ));
			return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
		}

		// Was this a link ? FILEATTR_Link
		if (bFollowLink && (statusSys.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
		{
		}

		::FindClose(hContext);

#elif defined(__linux__)

		cFileStatusSys statusSys;
		HRESULT hRes = cFileStatus::GetStatusSys(statusSys, pszFilePath, bFollowLink);
		if (FAILED(hRes))
			return hRes;
#endif

		if (pFileStatus != nullptr)
		{
			pFileStatus->InitFileStatus(statusSys);
			pFileStatus->UpdateLinuxHidden(cFilePath::GetFileName(pszFilePath));
			ASSERT(pFileStatus->isFileValid());
		}
		return S_OK;
	}
}
