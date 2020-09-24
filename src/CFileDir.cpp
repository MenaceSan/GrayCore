//
//! @file CFileDir.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CFileDir.h"
#include "CLogMgr.h"
#include "CAppState.h"
#include "StrArg.h"
#include "CFile.h"
#include "HResult.h"

#ifdef UNDER_CE
// UNDER_CE needs no includes.

#elif defined(_WIN32)
#include <io.h> 		// findfirst
#include <sys/stat.h>	// use stat() ?
#ifdef _MSC_VER
#include <shtypes.h>	// Needed for shlwapi.h
#endif
#include <shlobj.h>		// M$ documentation says this nowhere, but this is the header file for SHGetPathFromIDList shfolder.h
#include <shellapi.h>	// FOF_FILESONLY

#elif defined(__linux__)
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/statfs.h>

#else
#error NOOS
#endif

namespace Gray
{
	const LOGCHAR_t CFileDir::k_szCantMoveFile[] = "Can't Move File ";	//!< MoveDirFiles failed for this.

	cFileDevice::cFileDevice()
		: m_eType(FILESYS_DEFAULT)
		, m_nSerialNumber(0)
		, m_dwMaximumComponentLength(0)
		, m_bCaseSensitive(0)
	{
	}

	cFileDevice::~cFileDevice()
	{
	}

	bool cFileDevice::isCaseSensitive() const
	{
		//! The file system is case sensitive ? __linux__ = true, _WIN32 = false
		//! A network mounted SAMBA share will use whatever rules the native OS/FileSystem uses.
		//! m_sTypeName = "FAT","NTFS" system = non case sensitive. "NFS" = case sensitive.
		return m_bCaseSensitive;
	}

	const char* cFileDevice::k_FileSysName[FILESYS_QTY] =
	{
		"",			// FILESYS_DEFAULT
		"FAT",		// FILESYS_FAT
		"FAT32",	// FILESYS_FAT32
		"NTFS",		// FILESYS_NTFS
		"NFS",		// FILESYS_NFS
	};

	HRESULT cFileDevice::UpdateInfo(const FILECHAR_t* pszDeviceId)
	{
		//! @arg pszDeviceId = nullptr = use the current dir/path for the app.
		//! pszDeviceId can be from _WIN32 GetLogicalDriveStrings()
		//! some drives won't be ready (if removable). Thats OK. HRESULT_WIN32_C(ERROR_NOT_READY)

#ifdef UNDER_CE
		m_sVolumeName = pszDeviceId;
		m_nSerialNumber = 1;
		m_sTypeName = "NTFS";	// "NTFS" or "FAT"
		m_eType = FILESYS_NTFS;
		m_bCaseSensitive = false;

#elif defined(_WIN32)
		FILECHAR_t szVolumeNameBuffer[_MAX_PATH];
		szVolumeNameBuffer[0] = '\0';
		FILECHAR_t szFileSystemNameBuffer[_MAX_PATH];
		szFileSystemNameBuffer[0] = '\0';
		DWORD dwFlags = 0;
		m_nSerialNumber = 0;

		bool bRet = _FNF(::GetVolumeInformation)(pszDeviceId,
			szVolumeNameBuffer,
			STRMAX(szVolumeNameBuffer),
			(DWORD*)(void*)&m_nSerialNumber,
			&m_dwMaximumComponentLength,
			&dwFlags,
			szFileSystemNameBuffer,
			STRMAX(szFileSystemNameBuffer));
		if (!bRet)
		{
			return HResult::GetDef(HResult::GetLast(), E_FAIL);
		}

		m_sVolumeName = szVolumeNameBuffer;
		m_sTypeName = szFileSystemNameBuffer;	// "NTFS" or "FAT"
		m_eType = (FILESYS_TYPE)StrT::TableFindHead(szFileSystemNameBuffer, k_FileSysName);
		if (m_eType < 0)
			m_eType = FILESYS_NTFS;
		m_bCaseSensitive = (dwFlags & FILE_CASE_SENSITIVE_SEARCH);

#if 0
		// szPath without trailing backslash and FILEDEVICE_PREFIX
		//  FILEDEVICE_PREFIX "X:"
		//  FILEDEVICE_PREFIX "PhysicalDrive0"
		//  "\\\\\?\\Volume{433619ed-c6ea-11d9-a3b2-806d6172696f}

		HANDLE hDevice = ::CreateFile(szPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, HANDLE_NULL);
		if (hDevice != INVALID_HANDLE_VALUE)
		{
			DWORD dwOutBytes = 0;           // IOCTL output length
			STORAGE_PROPERTY_QUERY Query;   // input param for query

			// specify the query type
			Query.PropertyId = StorageDeviceProperty;
			Query.QueryType = PropertyStandardQuery;

			char OutBuf[1024] = { 0 };  // good enough, usually about 100 bytes
			PSTORAGE_DEVICE_DESCRIPTOR pDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)OutBuf;
			pDevDesc->Size = sizeof(OutBuf);

			// Query using IOCTL_STORAGE_QUERY_PROPERTY
			bool bRet = ::DeviceIoControl(hDevice,                     // device handle
				IOCTL_STORAGE_QUERY_PROPERTY,             // info of device property
				&Query, sizeof(STORAGE_PROPERTY_QUERY),  // input data buffer
				pDevDesc, pDevDesc->Size,               // output data buffer
				&dwOutBytes,                           // out's length
				(LPOVERLAPPED)nullptr);

			::CloseHandle(hDevice);
			if (bRet)
			{
				// here we are
				BusType = pDevDesc->BusType;
			}
		}
#endif

#elif defined(__linux__)
		if (pszDeviceId == nullptr)
		{
			m_sVolumeName = CAppState::get_CurrentDir();
			pszDeviceId = m_sVolumeName;
		}
#ifdef USE_64BIT
		struct statfs64 fs;
		int iRet = ::statfs64(pszDeviceId, &fs);
#else
		struct statfs fs;
		int iRet = ::statfs(pszDeviceId, &fs);
#endif
		if (iRet < 0)
		{
			return HResult::GetPOSIXLastDef(E_FAIL);
		}
		m_dwMaximumComponentLength = fs.f_bsize;
		m_sVolumeName = "/";
		switch (fs.f_type)
		{
		default:	// NFS_SUPER_MAGIC
			m_sTypeName = "NFS";	// "NTFS" or "FAT"
			m_eType = FILESYS_NFS;
			break;
		}
		memcpy(&m_nSerialNumber, (void*)&fs.f_fsid, sizeof(m_nSerialNumber));		// 64 bits.
		m_bCaseSensitive = (m_eType == FILESYS_NFS);
#endif
		return S_OK;
	}

	UINT GRAYCALL cFileDevice::GetDeviceType(const FILECHAR_t* pszDeviceId)
	{
		//! Determines whether a disk drive is a removable, fixed, CD-ROM, RAM disk, or network drive.
		//! https://msdn.microsoft.com/en-us/library/windows/desktop/aa364939(v=vs.85).aspx
		//! @return 0 =DRIVE_UNKNOWN, 1=DRIVE_NO_ROOT_DIR, 2=DRIVE_REMOVABLE, 3=DRIVE_FIXED, 4=DRIVE_REMOTE, 5=DRIVE_CDROM, 6=DRIVE_RAMDISK
#ifdef _WIN32
		return _FNF(::GetDriveType)(pszDeviceId);
#elif defined(__linux__)
		return 0;		// TODO.
#endif
	}

	FILE_SIZE_t GRAYCALL cFileDevice::GetDeviceFreeSpace(const FILECHAR_t* pszDeviceId) // static
	{
		//! Get disk space on the pszDeviceId.
		//! @return
		//!  the free disk space in bytes.

#ifdef _WIN32
		ULARGE_INTEGER nFreeBytesAvailableToCaller;
		ULARGE_INTEGER nTotalNumberOfBytes;
		ULARGE_INTEGER nTotalNumberOfFreeBytes;

		// GetDeviceFreeSpaceExW
		if (!_FNF(::GetDiskFreeSpaceEx)(pszDeviceId,
			&nFreeBytesAvailableToCaller,
			&nTotalNumberOfBytes,
			&nTotalNumberOfFreeBytes))
		{
			return(0);
		}

		return nFreeBytesAvailableToCaller.QuadPart;

#elif defined(__linux__)
		CStringF sTmp;
		if (pszDeviceId == nullptr)
		{
			sTmp = CAppState::get_CurrentDir();
			pszDeviceId = sTmp;
		}
#ifdef USE_64BIT
		struct statfs64 fs;
		int iRet = ::statfs64(pszDeviceId, &fs);
#else
		struct statfs fs;
		int iRet = ::statfs(pszDeviceId, &fs);
#endif
		if (iRet < 0)
			return 0;
		// NOTE f_bfree = super user. f_bavail = anyone else.
		return ((FILE_SIZE_t)(fs.f_bavail) * fs.f_bsize);
#else
#error NOOS
#endif
	}

	HRESULT GRAYCALL cFileDevice::GetSystemDeviceList(CArrayString<FILECHAR_t>& a) // static
	{
		//! list all the devices/volumes available to the system.
#ifdef UNDER_CE
	// No devices in CE.
#elif defined(_WIN32)
		FILECHAR_t szTmp[_MAX_PATH];
		FILECHAR_t* pszVol = szTmp;
		pszVol[0] = '\0';
		DWORD dwLen = _FNF(::GetLogicalDriveStrings)(STRMAX(szTmp), szTmp);
		if (dwLen <= 0)
		{
			return E_FAIL;
		}
		while (*pszVol != '\0')
		{
			a.Add(pszVol);
			pszVol += StrT::Len(pszVol) + 1;
		}
#elif defined(__linux__)
		CFileDir fdr;
		HRESULT hRes1 = fdr.ReadDir(FILESTR_DirSep "dev" FILESTR_DirSep "disk");
		if (FAILED(hRes1))
			return hRes1;
		for (int i = 0; i < hRes1; i++)
		{
			const cFileFindEntry& entry = fdr.GetEnumFile(i);
			a.Add(entry.get_Name());
		}
#else 
#error NOOS
#endif
		return (HRESULT)a.GetSize();
	}

	//*******************************************************

	cFileFind::cFileFind(CStringF sDir, DWORD nFileFlags)
		: m_sDirPath(sDir)
		, m_nFileFlags(nFileFlags)
#ifdef _WIN32
		, m_hContext(INVALID_HANDLE_VALUE)
#elif defined(__linux__)
		, m_bReadStats(true)
		, m_hContext(nullptr)
#endif
	{
	}

	bool cFileFind::isContextOpen() const
	{
#ifdef _WIN32
		if (m_hContext == INVALID_HANDLE_VALUE)
			return false;
#elif defined(__linux__)
		if (m_hContext == nullptr)
			return false;
#endif
		return true;
	}

	void cFileFind::CloseContext()
	{
		if (!isContextOpen())
			return;
#ifdef _WIN32
		::FindClose(m_hContext);
		m_hContext = INVALID_HANDLE_VALUE;
#elif defined(__linux__)
		::closedir(m_hContext);
		m_hContext = nullptr;
#endif
	}

	HRESULT cFileFind::FindFileNext(bool bFirst)
	{
		//! Read the next file in the directory list.
		//! ASSUME cFileFind::FindFile() was called.
		//! @return
		//!  HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no more files
		//! @note UNICODE files are converted to '?' chars if calling the non UNICODE version.
		if (!isContextOpen())
		{
			return HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS);
		}

#ifdef _WIN32
		if (!bFirst)
		{
			if (!::FindNextFileW(m_hContext, &m_FindInfo))
			{
				return HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS);
			}
		}

		m_FileEntry.m_sFileName = m_FindInfo.cFileName;		// How should we deal with UNICODE names ?
#ifndef USE_UNICODE_FN
	// May optionally want UNICODE
		if (!(m_nFileFlags&FOF_X_UTF8))
		{
			// Convert all UNICODES to ? like the ANSI functions do.
		}
#endif
		if (!(m_nFileFlags&FOF_X_WantDots) && isDots())	// ignore the . and .. that old systems can give us.
		{
			return FindFileNext(false);
		}

		if ((m_nFileFlags&FOF_X_FollowLinks) && (m_FindInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
		{
		}

		m_FileEntry.InitFileStatus(m_FindInfo);

#ifdef _DEBUG
		// NOTE: m_Size for FILE_ATTRIBUTE_DIRECTORY is NOT accurate!
		if (m_FileEntry.m_Attributes & FILEATTR_Directory)
		{
			ASSERT(m_FindInfo.nFileSizeLow == 0);
		}
#endif

#elif defined(__linux__)

		struct dirent* pFileInfo = ::readdir(m_hContext);
		if (pFileInfo == nullptr)
		{
			return HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS);
		}
		m_FileEntry.m_sFileName = pFileInfo->d_name;
		if (isDots())
		{
			if (!(this->m_nFileFlags&FOF_X_WantDots))
			{
				return FindFileNext(false);
			}
		}

		// filter on m_sWildcardFilter
		else if (!m_sWildcardFilter.IsEmpty() &&
			StrT::MatchRegEx<FILECHAR_t>(m_FileEntry.m_sFileName, m_sWildcardFilter, false) > 0)	// IgnoreCase ?
		{
			// Skip
			return FindFileNext(false);
		}

		// Match

		if (m_bReadStats)	// some dirs don't have stat() ability. e.g. "/proc"
		{
			// dirent() doesn't have this stuff...it's in stat()
			CFileStatusSys fileStatus;
			cString sFileName = GetFilePath(m_FileEntry.m_sFileName);
			int iRet = (this->m_nFileFlags&FOF_X_FollowLinks) ? ::lstat(sFileName, &fileStatus) : ::stat(sFileName, &fileStatus);
			if (iRet != 0)
			{
				return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
			}
			m_FileEntry.InitFileStatus(fileStatus);
		}
#endif

		m_FileEntry.UpdateLinuxHidden(m_FileEntry.m_sFileName);
		return S_OK;
	}

	HRESULT cFileFind::FindOpen(const FILECHAR_t* pszDirPath, const FILECHAR_t* pszWildcardFile)
	{
		//! start a sequential read of the files in a list of possible matches.
		//! @arg pszWildcardFile = "*.ext". if pszDirPath is empty, full path can be in pszWildcardFile
		//! @note pszWildcardFile can NOT have multiple "*.ext1;*.ext2"
		//! @return
		//!  HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no files.

		CloseContext();
		ASSERT(!isContextOpen());

		// Combine pszDirPath and pszWildcardFile
		if (pszDirPath != nullptr)
		{
			m_sDirPath = pszDirPath;	// store this. but may already be set.
		}
		CStringF sWildcardFile;	// if it is part of directory?
		if (m_sDirPath.IsEmpty())
		{
			// full path can be in pszWildcardFile. break it up.
			if (StrT::IsNullOrEmpty(pszWildcardFile))
			{
				return HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND);
			}
			m_sDirPath = CFilePath::GetFileDir(pszWildcardFile);
			pszWildcardFile = CFilePath::GetFileName(pszWildcardFile);	// this specific file. or may have wildcards.
		}
		else if (StrT::IsNullOrEmpty(pszWildcardFile))	// NOTE: NT doesn't need this but 98 does ?
		{
			// ASSUME m_sDirPath is just a directory path. Find all files in this path.
			if (CFilePath::HasTitleWildcards(m_sDirPath))
			{
				// NOTE: we should have put this in pszWildcardFile! This is wrong. avoid ambiguous queries.
				sWildcardFile = CFilePath::GetFileName(m_sDirPath);
				pszWildcardFile = sWildcardFile;
				m_sDirPath = CFilePath::GetFileDir(m_sDirPath);
			}
			else
			{
				pszWildcardFile = _FN("*");	// All files in this directory = default.
			}
		}

#ifdef _WIN32
		// in _WIN32 wildcard filter is built in.
		CMem::Zero(&m_FindInfo, sizeof(m_FindInfo));
		m_FindInfo.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;	 // docs say this is not needed

		CStringF sFilePath = GetFilePath(pszWildcardFile);
		m_hContext = ::FindFirstFileW(CFilePath::GetFileNameLongW(sFilePath), &m_FindInfo);

#elif defined(__linux__)
		// in Linux wildcard filter is done later/manually.
		m_sWildcardFilter = pszWildcardFile;

		// Need to strip out the *.EXT part. just need the dir name here.
		m_hContext = ::opendir(m_sDirPath.get_CPtr());
#endif

		if (!isContextOpen())
		{
			return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND));
		}

		return S_OK;
	}

	HRESULT cFileFind::FindFile(const FILECHAR_t* pszDirPath, const FILECHAR_t* pszWildcardFile)
	{
		//! Start a sequential read of the files in a list of possible matches.
		//! @arg pszWildcardFile = "*.ext". if pszDirPath is empty, full path can be in pszWildcardFile
		//! @return
		//!  HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no files.
		HRESULT hRes = FindOpen(pszDirPath, pszWildcardFile);
		if (FAILED(hRes))
			return hRes;
		return FindFileNext(true);
	}

	//************************************************************

	HRESULT GRAYCALL CFileDir::CreateDirectory1(const FILECHAR_t* pszDirName) // static
	{
		//! HRESULT_WIN32_C(ERROR_ALREADY_EXISTS) is OK ?
		//! use CreateDirectory1 name because might be "#define CreateDirectory CreateDirectoryA" in _WIN32
#ifdef _WIN32
		if (!::CreateDirectoryW(CFilePath::GetFileNameLongW(pszDirName), nullptr))
#elif defined(__linux__)
		if (::mkdir(pszDirName, 0777) != 0)	// mode_t
#endif
		{
			HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
			if (hRes == HRESULT_WIN32_C(ERROR_ALREADY_EXISTS))	// this is OK.
				return S_FALSE;
			return hRes;
		}
		return S_OK;
	}
	HRESULT GRAYCALL CFileDir::RemoveDirectory1(const FILECHAR_t* pszDirName) // static
	{
		//! @note will fail if directory is not empty.
		//! use RemoveDirectory1 name because might be "#define RemoveDirectory RemoveDirectoryA" in _WIN32
#ifdef _WIN32
		if (!::RemoveDirectoryW(CFilePath::GetFileNameLongW(pszDirName)))
#elif defined(__linux__)
		if (::rmdir(pszDirName) != 0)	// mode_t
#endif
		{
			return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
		}
		return S_OK;
	}

	//********************************************

	HRESULT CFileDir::ReadDir(const FILECHAR_t* pszDirPath, const FILECHAR_t* pszWildcardFile, ITERATE_t iFilesMax, bool bFollowLink)
	{
		//! calls the virtual AddFileDirEntry()
		//! @arg
		//!  pszDirPath = the base directory. nullptr = current dir.
		//!  pszWildcardFile = may use a filter like: *.scp. may contain full path if pszDirPath is empty. nullptr = all.
		//! @note It seems _WIN32 does NOT like the trailing "\" alone
		//!  e.g. "d:\menace\scripts\" is bad ! use "d:\menace\scripts\*.*"
		//! @return
		//!  <0 = error.
		//!  number of files.

		if (pszDirPath != nullptr)
		{
			m_sDirPath = pszDirPath;	// store this
		}

		cFileFind state(m_sDirPath, bFollowLink);
		HRESULT hRes = state.FindFile(nullptr, pszWildcardFile);
		if (FAILED(hRes))
		{
			if (hRes == HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) || hRes == HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND))	// no files.
				return 0;
			return hRes;
		}

		m_sDirPath = state.get_DirPath();	// in case real path is in pszWildcardFile
		ITERATE_t iFiles = 0;
		while (iFiles < iFilesMax)
		{
			hRes = AddFileDirEntry(state.m_FileEntry);
			if (FAILED(hRes))
				continue;
			iFiles++;
			hRes = state.FindFileNext();
			if (FAILED(hRes)) // HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS)
				break;
		}

		return (HRESULT) iFiles;		// number of files.
	}

	HRESULT CFileDir::ReadDirAnyExt(const FILECHAR_t* pszFilePath, ITERATE_t iFilesMax)
	{
		//! Find this file name but with any extension.
		//! @return # files found with this file title.
		return ReadDir(CFilePath::GetFileDir(pszFilePath),
			CFilePath::GetNameExtStar(pszFilePath),
			iFilesMax);
	}

	HRESULT CFileDir::ReadDirPreferredExt(const FILECHAR_t* pszFilePath, const FILECHAR_t* const* pszExtTable)
	{
		//! Find just a single file with the preferred extension from a list.
		//! Ignore any existing extension
		//! @arg
		//!  pszExtTable  = null terminated table. extension has dots.
		//! @return
		//!  the enum of the extension we got. 0 - N in pszExtTable
		//!  HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND) = nothing found.

		static const int k_ExtMax = 32;

		HRESULT hFiles = ReadDirAnyExt(pszFilePath, k_ExtMax);
		if (FAILED(hFiles))
			return hFiles;
		if (hFiles == 0)
			return HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND);

		// Filter them.
		int iFound = 0;
		cFileFindEntry* aEntries[k_ExtMax]; // sorted by extension.
		CMem::Zero(aEntries, sizeof(aEntries));
		for (int i = 0; i < (int)hFiles; i++)
		{
			const FILECHAR_t* pszExt = CFilePath::GetFileNameExt(GetEnumTitleX(i));
			if (pszExt == nullptr)
				continue;
			ITERATE_t iExt = StrT::TableFind(pszExt, pszExtTable);
			if (IS_INDEX_BAD_ARRAY(iExt, aEntries))
				continue;
			if (iExt >= (ITERATE_t)_countof(aEntries))	// overflow !
			{
				continue;
			}
			aEntries[iExt] = &m_aFiles[i];
			iFound++;
		}

		if (iFound <= 0)
		{
			RemoveAll();
			return HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND);
		}

		for (UINT i = 0; i < _countof(aEntries); i++)
		{
			if (aEntries[i] == nullptr)
				continue;
			m_aFiles[0] = *aEntries[i];
			m_aFiles.SetSize(1);
			return i;	// what entry in pszExtTable
		}

		ASSERT(0);
		return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);
	}

	//*************************************************

	HRESULT GRAYCALL CFileDir::CreateDirectoryX(const FILECHAR_t* pszDir) // static
	{
		//! This is like CreateDirectory1() except will create intermediate directories if needed.
		//! @note like SHCreateDirectoryExA() but we can't always use since thats only for Win2K+
		//! @return S_FALSE = already exists. equiv to ERROR_ALREADY_EXISTS. NOT a real error.

		if (pszDir == nullptr)
			return E_POINTER;
		FILECHAR_t szTmp[_MAX_PATH];
		StrLen_t iLen = 0;
		HRESULT hRes = S_FALSE;
		for (;;)
		{
			StrLen_t iStart = iLen;
			FILECHAR_t ch;
			for (;;)
			{
				if (iLen >= STRMAX(szTmp))
				{
					return HRESULT_WIN32_C(ERROR_BUFFER_OVERFLOW);
				}
				ch = pszDir[iLen];
				szTmp[iLen] = ch;
				if (ch == '\0')
					break;
				iLen++;
				if (CFilePath::IsCharDirSep(ch))
					break;
			}
			if (iStart >= iLen)
			{
				if (ch == '\0')
					break;
				return HRESULT_WIN32_C(ERROR_BAD_PATHNAME);	// bad name!
			}
			szTmp[iLen] = '\0';
			hRes = CreateDirectory1(szTmp);
			if (FAILED(hRes))
			{
				if (hRes == HRESULT_WIN32_C(ERROR_ACCESS_DENIED) && iStart == 0) // create a drive? 'c:\' type thing.
					continue;
				return hRes;
			}
		}

		return hRes;
	}

	HRESULT GRAYCALL CFileDir::CreateDirForFileX(const FILECHAR_t* pszFilePath) // static
	{
		//! CreateDirectoryX() for a file. will create intermediate/parent directories if needed.
		return CreateDirectoryX(CFilePath::GetFileDir(pszFilePath));
	}

	HRESULT GRAYCALL CFileDir::MovePathToTrash(const FILECHAR_t* pszPath, bool bDir)  // static
	{
		//! Move file/directory to the trash bin/folder.
		//! For use with FOF_ALLOWUNDO and FILEOP_DELETE
		//! like WIN32 SHFileOperation(FOF_ALLOWUNDO);

		UNREFERENCED_PARAMETER(pszPath);
		CStringF sDirTrash;
#if defined(UNDER_CE)
#error UNDER_CE
#elif defined(_WIN32)
		FILECHAR_t szPath[_MAX_PATH];
		HRESULT hRes = _FNF(::SHGetFolderPath)(HANDLE_NULL, CSIDL_BITBUCKET, NULL, 0, szPath);	// ASSUME _MAX_PATH
		if (FAILED(hRes))
			return hRes;
		sDirTrash = szPath;
#elif defined(__linux__)
		// https://www.freedesktop.org/wiki/Specifications/trash-spec/
		sDirTrash = CAppState::GetEnvironStr("XDG_DATA_HOME");
		if (!StrT::IsWhitespace<FILECHAR_t>(sDirTrash))
		{
			sDirTrash = CFilePath::CombineFilePathX(sDirTrash, _FN("Trash"));
		}
#else
#error NOOS
#endif

		// Make sure there is no collision.

		if (bDir)
		{
			return DirFileOp(FILEOP_MOVE, pszPath, sDirTrash, 0, nullptr, nullptr);
		}
		else
		{
			return cFile::RenamePath(pszPath, CFilePath::CombineFilePathX(sDirTrash, CFilePath::GetFileName(pszPath)), nullptr);
		}
	}

	HRESULT GRAYCALL CFileDir::DirFileOp(FILEOP_TYPE eOp, const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, DWORD nFileFlags, CLogProcessor* pLog, IStreamProgressCallback* pProgress)
	{
		//! Copy, Delete or Move a directory AND all files in the directory (pszDirSrc) to pszDirDest. with recursive descent.
		//! @arg pszDirSrc = full path.
		//! @arg nFileFlags = FOF_ALLOWUNDO, FOF_FILESONLY, FOF_RENAMEONCOLLISION, FILEOP_FLAGS
		//! @return <0 or S_OK = nothing to do.
		//!  Number of files moved/deleted.

		const FILECHAR_t* pszWildcards = nullptr;
		CStringF sDirSrc;
		if (CFilePath::HasTitleWildcards(pszDirSrc))
		{
			pszWildcards = CFilePath::GetFileName(pszDirSrc);
			sDirSrc = CFilePath::GetFileDir(pszDirSrc);
			pszDirSrc = sDirSrc;
		}

		switch (eOp)
		{
		case FILEOP_RENAME:
			if (pszDirDest == nullptr || pszWildcards != nullptr)
				return E_INVALIDARG;	// don't know how to deal with this.
			// Wildcards in pszDirSrc might do strange things!
			break;
		case FILEOP_DELETE:
			if (pszWildcards == nullptr)
			{
				pszWildcards = pszDirDest;
			}
			break;
		default:
			break;
		}

		CFileDir filedir;
		HRESULT hResCount = filedir.ReadDir(pszDirSrc, pszWildcards);
		if (FAILED(hResCount))
			return hResCount;
		if (hResCount <= 0)
			return S_OK;	// no files moved.

		HRESULT hRes = S_OK;
		if (eOp == FILEOP_MOVE || eOp == FILEOP_COPY)
		{
			hRes = CreateDirectoryX(pszDirDest);
			if (FAILED(hRes))
			{
				return hRes;
			}
		}

		int iErrors = 0;
		for (int i = 0; i < (int)hResCount; i++)
		{
			// Move each of the files.
			cFileFindEntry& FileEntry = filedir.GetEnumFile(i); // entry in the local directory.
			if (nFileFlags & FOF_FILESONLY)
			{
				if (FileEntry.isAttrDir())
					continue;
			}

			CStringF sFilePathSrc = filedir.GetEnumPath(i);
			CStringF sFileTitle = filedir.GetEnumTitleX(i);
			CStringF sFilePathDst;
			if (eOp == FILEOP_MOVE || eOp == FILEOP_COPY || eOp == FILEOP_RENAME)
			{
				sFilePathDst = CFilePath::CombineFilePathX(pszDirDest, sFileTitle);
			}

			if (FileEntry.isAttrDir())
			{
				// Recursive descent.
				if (eOp == FILEOP_DELETE)
				{
					if (StrT::IsWhitespace(pszWildcards))
					{
						if (nFileFlags & FOF_ALLOWUNDO)	// move whole dir to the trash.
						{
							// hRes = MovePathToTrash(sFilePathSrc,true);
						}
					}
					else
					{
						sFilePathDst = pszWildcards;
					}
				}
				hRes = DirFileOp(eOp, sFilePathSrc, sFilePathDst, nFileFlags, pLog, pProgress);
				if (FAILED(hRes))
				{
					return hRes;
				}
			}
			else
			{
				switch (eOp)
				{
				case FILEOP_MOVE:
				case FILEOP_RENAME:
					hRes = cFile::RenamePath(sFilePathSrc, sFilePathDst, pProgress);
					break;
				case FILEOP_COPY:
					hRes = cFile::CopyFileX(sFilePathSrc, sFilePathDst, pProgress);
					break;
				case FILEOP_DELETE:
					if (nFileFlags & FOF_ALLOWUNDO)
					{
						// hRes = MovePathToTrash(sFilePathSrc,false);
					}
					hRes = cFile::DeletePathX(sFilePathSrc, nFileFlags);
					break;
				}
				if (FAILED(hRes))
				{
					// Record that it failed!
					++iErrors;
					if (pLog != nullptr)
					{
						pLog->addEventF(LOG_ATTR_INIT, LOGLEV_ERROR,
							"%s\"%s\" ERR=\"%s\". '%s' to '%s'.",
							LOGSTR(k_szCantMoveFile),
							LOGSTR(sFileTitle),	// CFilePath::MakeRelativePath( sFilePathDst, pszDirDest )
							LOGERR(hRes),
							LOGSTR(filedir.get_DirPath()), LOGSTR(pszDirDest));
					}
				}
			}
		}

		if (eOp == FILEOP_MOVE || (eOp == FILEOP_DELETE && StrT::IsWhitespace(pszWildcards)))
		{
			hRes = RemoveDirectory1(pszDirSrc);
		}

		return hResCount;
	}

	HRESULT GRAYCALL CFileDir::DeletePathX(const FILECHAR_t* pszPath, DWORD nFileFlags) // static
	{
		//! Delete this file or directory. If it's a directory then delete recursively.
		//! No wildcards.
		cFileStatus fileStatus;
		HRESULT hRes = fileStatus.ReadFileStatus(pszPath);
		if (SUCCEEDED(hRes))
		{
			if (nFileFlags & FOF_FILESONLY)
			{
				if (fileStatus.isAttrDir())
					return S_FALSE;
			}
			if (nFileFlags & FOF_ALLOWUNDO)
			{
				// hRes = MovePathToTrash(sFilePathSrc,fileStatus.isAttrDir());
			}
			if (fileStatus.isAttrDir())
			{
				hRes = DeleteDirFiles(pszPath, nullptr, nFileFlags);
			}
			else
			{
				hRes = cFile::DeletePathX(pszPath, nFileFlags);
			}
		}
		if (hRes == HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND))
			return S_FALSE;
		return hRes;
	}
}

//*************************************************************************
#ifdef USE_UNITTESTS
#include "CUnitTest.h"
#include "CFilePath.h"

UNITTEST_CLASS(CFileDir)
{
	HRESULT TestDevice(cFileDevice& di, const FILECHAR_t* pszDeviceId, CStringL sNameDisplay)
	{
		UINT uDeviceType = cFileDevice::GetDeviceType(pszDeviceId);
		if (uDeviceType <= 0)	// UNKNOWN.
			return S_OK;

		sm_pLog->addDebugInfoF("%s Device Type = %d", LOGSTR(sNameDisplay), uDeviceType);

		HRESULT hRes1 = di.UpdateInfo(pszDeviceId);
		if (FAILED(hRes1))
		{
			if (pszDeviceId != nullptr && hRes1 == HRESULT_WIN32_C(ERROR_NOT_READY))
			{
				// some drives won't be ready (if removable). Thats OK.
				sm_pLog->addDebugInfoF("%s Not Ready", LOGSTR(sNameDisplay));
			}
			else
			{
				sm_pLog->addDebugInfoF("%s ERR='%s'", LOGERR(hRes1));
				UNITTEST_TRUE(SUCCEEDED(hRes1));
			}
			return hRes1;
		}

		sm_pLog->addDebugInfoF("%s Vol='%s','%s',ser=0%sh,clen=%u,case=%d",
			LOGSTR(sNameDisplay),
			LOGSTR(di.m_sVolumeName), LOGSTR(di.m_sTypeName),
			LOGSTR2(di.m_nSerialNumber, 0x10),			// Volume serial number (time stamp of last format) e.g. 0x0ca0e613
			di.m_dwMaximumComponentLength,	// e.g. 255 bytes
			di.m_bCaseSensitive);			// e.g. 0x03e700ff, FILE_CASE_SENSITIVE_SEARCH

		FILE_SIZE_t uFreeDiskSpace = cFileDevice::GetDeviceFreeSpace(pszDeviceId);
		UNITTEST_TRUE(uFreeDiskSpace > 0 || uDeviceType == DRIVE_CDROM || uDeviceType == DRIVE_REMOTE);		// has free space or its a read only device like DVD?
		sm_pLog->addDebugInfoF("%s FreeSpace=%s", LOGSTR(sNameDisplay), LOGSTR(cStringT<LOGCHAR_t>::GetSizeK(uFreeDiskSpace, 0, true)));

		return S_OK;
	}

	void TestReadDir()
	{
		CStringF sTestInpDir = CFilePath::CombineFilePathF(CFilePath::k_DirSep, get_TestInpDir(), _FN(GRAY_NAMES) _FN("Lib/include"), nullptr);

		const int s_nMinFiles = 3; // min number of header files assumed to be in this dir.
		CFileDir fd;
		HRESULT hRes1 = fd.ReadDir(sTestInpDir, _FN("*.h"));
		UNITTEST_TRUE(SUCCEEDED(hRes1));
		UNITTEST_TRUE((int)hRes1 >= s_nMinFiles);

		// low level dir reader. does not save the list.
		cFileFind ff(sTestInpDir);
		HRESULT hRes2 = ff.FindFile(nullptr, _FN("*.h"));
		UNITTEST_TRUE(SUCCEEDED(hRes2));

		int i = 0;
		for (; SUCCEEDED(hRes2); i++)
		{
			hRes2 = ff.FindFileNext(false);
		}
		UNITTEST_TRUE(i >= s_nMinFiles);
		UNITTEST_TRUE(i == (int)hRes1);	// same number of files.

		// Get status of a single file/folder.
		cFileStatus fs;
		hRes2 = fs.ReadFileStatus(sTestInpDir);
		UNITTEST_TRUE(hRes2 == S_OK);
		UNITTEST_TRUE(fs.isAttrDir());
	}

	void TestCreateFileInDir(CStringF sTestFile, FILESYS_TYPE eFileSysType)
	{
		HRESULT hRes1 = CFileDir::CreateDirForFileX(sTestFile);
		UNITTEST_TRUE(hRes1 == S_OK || hRes1 == S_FALSE);

		// Create a file and write some data to it.
		{
			cFile fileTest;
			hRes1 = fileTest.OpenX(sTestFile, OF_WRITE | OF_CREATE);
			UNITTEST_TRUE(hRes1 == S_OK);
			hRes1 = fileTest.WriteString(k_sTextBlob.get_CPtr());
			UNITTEST_TRUE(hRes1 == CUnitTestCur::k_TEXTBLOB_LEN);
			fileTest.Close();
		}

		// FAIL to read a file as a directory. It should fail.
		CFileDir filedirFail;
		hRes1 = filedirFail.ReadDir(sTestFile);
		UNITTEST_TRUE(hRes1 == HRESULT_WIN32_C(ERROR_DIRECTORY));	// The directory name is invalid.

		static const GChar_t* k_pszTimeChange = _GT("2008-07-09 13:47:10 Z");	// TIME_FORMAT_TZ
		CTimeInt tf;
		HRESULT hRes = tf.SetTimeStr(k_pszTimeChange);
		UNITTEST_TRUE(hRes == 0x15);
		cString sTimeChange2 = tf.GetTimeFormStr(TIME_FORMAT_TZ, TZ_UTC);
		UNITTEST_TRUE(!sTimeChange2.Compare(k_pszTimeChange));

		CTimeFile tfft = tf.GetAsFileTime();
		hRes1 = cFileStatus::WriteFileTimes(sTestFile, &tfft, &tfft);
		UNITTEST_TRUE(hRes1 == S_OK);

		cFileStatus fs2;
		HRESULT hRes2 = fs2.ReadFileStatus(sTestFile);
		UNITTEST_TRUE(hRes2 == S_OK);
		UNITTEST_TRUE(fs2.IsSameChangeTime(tf));

		// NFS merges create and modified times ? Create Time NOT set.
		if (eFileSysType != FILESYS_NFS)
		{
			cString sTimeChange = CTimeInt(fs2.m_timeCreate).GetTimeFormStr(TIME_FORMAT_TZ, TZ_UTC);
			UNITTEST_TRUE(!sTimeChange.Compare(k_pszTimeChange));
		}
	}

	UNITTEST_METHOD(CFileDir)
	{
		// enumerate devices in the system.

		cFileDevice di;
		TestDevice(di, nullptr, "(default)");

		// GetDriveType()
		CArrayString<FILECHAR_t> aDevices;
		HRESULT hRes1 = cFileDevice::GetSystemDeviceList(aDevices);
		UNITTEST_TRUE(SUCCEEDED(hRes1));

		for (int i = 0; i < aDevices.GetSize(); i++)
		{
			CStringL sNameDisplay = CStringL::GetFormatf("[%d],'%s'", i, LOGSTR(aDevices[i]));
			cFileDevice di2;
			TestDevice(di2, aDevices[i], sNameDisplay);
		}

		// high level directory reader. saves a list.
		TestReadDir();

		// Create a complex path then destroy it.
		CStringF sTestDir = CFilePath::CombineFilePathX(get_TestOutDir(), _FN("a1"));
		CStringF sTestFile = CFilePath::CombineFilePathX(sTestDir, _FN("2/cc3/d4/testfile.txt"));
		TestCreateFileInDir(sTestFile, di.get_FileSysType());

		// Now delete it all.
		hRes1 = CFileDir::DeleteDirFiles(sTestDir);
		UNITTEST_TRUE(hRes1 == S_FALSE);	// 1 dir.
	}
};
UNITTEST_REGISTER(CFileDir, UNITTEST_LEVEL_Core);
#endif
