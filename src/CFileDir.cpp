//! @file cFileDir.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "HResult.h"
#include "StrArg.h"
#include "cAppState.h"
#include "cBits.h"
#include "cFile.h"
#include "cFileCopier.h"
#include "cFileDir.h"
#include "cLogMgr.h"

#ifdef UNDER_CE
// UNDER_CE needs no includes.

#elif defined(_WIN32)
#include <io.h>        // findfirst
#include <sys/stat.h>  // use stat() ?
#ifdef _MSC_VER
#include <shtypes.h>  // Needed for shlwapi.h
#endif
#include <shellapi.h>  // FOF_FILESONLY
#include <shlobj.h>    // M$ documentation says this nowhere, but this is the header file for SHGetPathFromIDList shfolder.h

#elif defined(__linux__)
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>

#else
#error NOOS
#endif

namespace Gray {
const LOGCHAR_t cFileDir::k_szCantMoveFile[] = "Can't Move File ";  /// MoveDirFiles failed for this.

cFileDevice::cFileDevice() : m_eType(FILESYS_t::_DEFAULT), m_nSerialNumber(0), m_dwMaximumComponentLength(0), m_bCaseSensitive(0) {}

cFileDevice::~cFileDevice() {}

bool cFileDevice::isCaseSensitive() const {
    //! The file system is case sensitive ? __linux__ = true, _WIN32 = false
    //! A network mounted SAMBA share will use whatever rules the native OS/FileSystem uses.
    //! m_sTypeName = "FAT","NTFS" system = non case sensitive. "NFS" = case sensitive.
    return m_bCaseSensitive;
}

const char* cFileDevice::k_FileSysName[static_cast<int>(FILESYS_t::_QTY)] = {
    "",       // FILESYS_t::_DEFAULT
    "FAT",    // FILESYS_t::_FAT
    "FAT32",  // FILESYS_t::_FAT32
    "NTFS",   // FILESYS_t::_NTFS
    "NFS",    // FILESYS_t::_NFS
};

HRESULT cFileDevice::UpdateInfo(const FILECHAR_t* pszDeviceId) {
    //! @arg pszDeviceId = nullptr = use the current dir/path for the app.
    //! pszDeviceId can be from _WIN32 GetLogicalDriveStrings()
    //! some drives won't be ready (if removable). Thats OK. HRESULT_WIN32_C(ERROR_NOT_READY)

#ifdef UNDER_CE
    m_sVolumeName = pszDeviceId;
    m_nSerialNumber = 1;
    m_sTypeName = "NTFS";  // "NTFS" or "FAT"
    m_eType = FILESYS_t::_NTFS;
    m_bCaseSensitive = false;

#elif defined(_WIN32)
    FILECHAR_t szVolumeNameBuffer[cFilePath::k_MaxLen];
    szVolumeNameBuffer[0] = '\0';
    FILECHAR_t szFileSystemNameBuffer[cFilePath::k_MaxLen];
    szFileSystemNameBuffer[0] = '\0';
    DWORD dwFlags = 0;
    m_nSerialNumber = 0;

    bool bRet = _FNF(::GetVolumeInformation)(pszDeviceId, szVolumeNameBuffer, STRMAX(szVolumeNameBuffer), (DWORD*)(void*)&m_nSerialNumber, &m_dwMaximumComponentLength, &dwFlags, szFileSystemNameBuffer, STRMAX(szFileSystemNameBuffer));
    if (!bRet) return HResult::GetDef(HResult::GetLast(), E_FAIL);

    m_sVolumeName = szVolumeNameBuffer;
    m_sTypeName = szFileSystemNameBuffer;  // "NTFS" or "FAT"
    m_eType = (FILESYS_t)StrT::SpanFindHead(szFileSystemNameBuffer, TOSPAN(k_FileSysName));
    if (m_eType < FILESYS_t::_DEFAULT) m_eType = FILESYS_t::_NTFS;
    m_bCaseSensitive = (dwFlags & FILE_CASE_SENSITIVE_SEARCH);

#if 0  // 0
       // szPath without trailing backslash and FILEDEVICE_PREFIX
       //  FILEDEVICE_PREFIX "X:"
       //  FILEDEVICE_PREFIX "PhysicalDrive0"
       //  "\\\\\?\\Volume{433619ed-c6ea-11d9-a3b2-806d6172696f}

		cOSHandle hDevice(_FNF(::CreateFile)(pszDeviceId, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, HANDLE_NULL));
		if (hDevice.isValidHandle())
		{
			DWORD dwOutBytes = 0;           // IOCTL output length
			::STORAGE_PROPERTY_QUERY Query;   // input param for query

			// specify the query type
			Query.PropertyId = StorageDeviceProperty;
			Query.QueryType = PropertyStandardQuery;

			char OutBuf[1024] = { 0 };  // good enough, usually about 100 bytes
			::STORAGE_DEVICE_DESCRIPTOR* pDevDesc = (::STORAGE_DEVICE_DESCRIPTOR*)OutBuf;
			pDevDesc->Size = sizeof(OutBuf);

			// Query using IOCTL_STORAGE_QUERY_PROPERTY
			bool bRet = ::DeviceIoControl(hDevice,                     // device handle
				IOCTL_STORAGE_QUERY_PROPERTY,             // info of device property
				&Query, sizeof(STORAGE_PROPERTY_QUERY),  // input data buffer
				pDevDesc, pDevDesc->Size,               // output data buffer
				&dwOutBytes,                           // out's length
				(LPOVERLAPPED)nullptr);
			if (bRet) {
				// here we are
				// BusType = pDevDesc->BusType;
			}
		}
#endif

#elif defined(__linux__)
    if (pszDeviceId == nullptr) {
        m_sVolumeName = cAppState::get_CurrentDir();
        pszDeviceId = m_sVolumeName;
    }
#ifdef USE_64BIT
    struct statfs64 fs;
    int iRet = ::statfs64(pszDeviceId, &fs);
#else
    struct statfs fs;
    int iRet = ::statfs(pszDeviceId, &fs);
#endif
    if (iRet < 0) {
        return HResult::GetPOSIXLastDef(E_FAIL);
    }
    m_dwMaximumComponentLength = fs.f_bsize;
    m_sVolumeName = "/";
    switch (fs.f_type) {
        default:                  // NFS_SUPER_MAGIC
            m_sTypeName = "NFS";  // "NTFS" or "FAT"
            m_eType = FILESYS_t::_NFS;
            break;
    }
    cMem::Copy(&m_nSerialNumber, (void*)&fs.f_fsid, sizeof(m_nSerialNumber));  // 64 bits.
    m_bCaseSensitive = (m_eType == FILESYS_t::_NFS);
#endif
    return S_OK;
}

UINT GRAYCALL cFileDevice::GetDeviceType(const FILECHAR_t* pszDeviceId) {
    //! Determines whether a disk drive is a removable, fixed, CD-ROM, RAM disk, or network drive.
    //! https://msdn.microsoft.com/en-us/library/windows/desktop/aa364939(v=vs.85).aspx
    //! @return 0 =DRIVE_UNKNOWN, 1=DRIVE_NO_ROOT_DIR, 2=DRIVE_REMOVABLE, 3=DRIVE_FIXED, 4=DRIVE_REMOTE, 5=DRIVE_CDROM, 6=DRIVE_RAMDISK
#ifdef _WIN32
    return _FNF(::GetDriveType)(pszDeviceId);
#elif defined(__linux__)
    return 0;  // TODO.
#endif
}

FILE_SIZE_t GRAYCALL cFileDevice::GetDeviceFreeSpace(const FILECHAR_t* pszDeviceId) {  // static
    //! Get disk space on the pszDeviceId.
    //! @return
    //!  the free disk space in bytes.

#ifdef _WIN32
    ULARGE_INTEGER nFreeBytesAvailableToCaller;
    ULARGE_INTEGER nTotalNumberOfBytes;
    ULARGE_INTEGER nTotalNumberOfFreeBytes;

    // GetDeviceFreeSpaceExW
    if (!_FNF(::GetDiskFreeSpaceEx)(pszDeviceId, &nFreeBytesAvailableToCaller, &nTotalNumberOfBytes, &nTotalNumberOfFreeBytes)) {
        return 0;
    }

    return nFreeBytesAvailableToCaller.QuadPart;

#elif defined(__linux__)
    cStringF sTmp;
    if (pszDeviceId == nullptr) {
        sTmp = cAppState::get_CurrentDir();
        pszDeviceId = sTmp;
    }
#ifdef USE_64BIT
    struct statfs64 fs;
    int iRet = ::statfs64(pszDeviceId, &fs);
#else
    struct statfs fs;
    int iRet = ::statfs(pszDeviceId, &fs);
#endif
    if (iRet < 0) return 0;
    // NOTE f_bfree = super user. f_bavail = anyone else.
    return (FILE_SIZE_t)(fs.f_bavail) * fs.f_bsize;
#else
#error NOOS
#endif
}

HRESULT GRAYCALL cFileDevice::GetSystemDeviceList(cArrayString<FILECHAR_t>& a) {  // static
    //! list all the devices/volumes available to the system.
#ifdef UNDER_CE
    // No devices in CE.
#elif defined(_WIN32)
    FILECHAR_t szTmp[cFilePath::k_MaxLen];
    FILECHAR_t* pszVol = szTmp;
    pszVol[0] = '\0';
    const DWORD dwLen = _FNF(::GetLogicalDriveStrings)(STRMAX(szTmp), szTmp);
    if (dwLen <= 0) return E_FAIL;
    while (*pszVol != '\0') {
        a.Add(pszVol);
        pszVol += StrT::Len(pszVol) + 1;
    }
#elif defined(__linux__)
    cFileDir fdr;
    HRESULT hRes1 = fdr.ReadDir(FILESTR_DirSep "dev" FILESTR_DirSep "disk");
    if (FAILED(hRes1)) return hRes1;
    for (const cFileDirEntry& fileEntry : fdr.m_aFiles) {
        a.Add(fileEntry.get_Name());
    }
#else
#error NOOS
#endif
    return CastN(HRESULT, a.GetSize());
}

//*******************************************************

cFileFind::cFileFind(cStringF sDir, DWORD nFileFlags) noexcept
    : m_sDirPath(sDir),
      m_nFileFlags(nFileFlags),
#ifdef _WIN32
      m_hContext(INVALID_HANDLE_VALUE)
#elif defined(__linux__)
      m_bReadStats(true),
      m_hContext(nullptr)
#endif
{
}

bool cFileFind::isContextOpen() const {
#ifdef _WIN32
    if (m_hContext == INVALID_HANDLE_VALUE) return false;
#elif defined(__linux__)
    if (m_hContext == nullptr) return false;
#endif
    return true;
}

void cFileFind::CloseContext() {
    if (!isContextOpen()) return;
#ifdef _WIN32
    ::FindClose(m_hContext);
    m_hContext = INVALID_HANDLE_VALUE;
#elif defined(__linux__)
    ::closedir(m_hContext);
    m_hContext = nullptr;
#endif
}

HRESULT cFileFind::FindFileNext(bool bFirst) {
    //! Read the next file in the directory list.
    //! ASSUME cFileFind::FindFile() was called.
    //! @return
    //!  HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no more files
    //! @note UNICODE files are converted to '?' chars if calling the non UNICODE version.
    if (!isContextOpen()) {
        return HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS);
    }

#ifdef _WIN32
    if (!bFirst) {
        if (!::FindNextFileW(m_hContext, &m_FindInfo)) {
            return HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS);
        }
    }

    m_FileEntry.m_sFileName = m_FindInfo.cFileName;  // How should we deal with USE_UNICODE_FN names ? Use UTF8

    if (!cBits::HasAny<DWORD>(m_nFileFlags, FOF_X_WantDots) && isDots()) {  // ignore the . and .. that old systems can give us.
        return FindFileNext(false);
    }

    if (cBits::HasAny<DWORD>(m_nFileFlags, FOF_X_FollowLinks) && cBits::HasAny<DWORD>(m_FindInfo.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {
    }

    m_FileEntry.InitFileStatus(m_FindInfo);

#ifdef _DEBUG
    // NOTE: m_Size for FILE_ATTRIBUTE_DIRECTORY is NOT accurate!
    if (m_FileEntry.isAttrDir()) {
        ASSERT(m_FindInfo.nFileSizeLow == 0);
    }
#endif

#elif defined(__linux__)

    struct dirent* pFileInfo = ::readdir(m_hContext);
    if (pFileInfo == nullptr) return HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS);

    m_FileEntry.m_sFileName = pFileInfo->d_name;
    if (isDots()) {
        if (!(this->m_nFileFlags & FOF_X_WantDots)) {
            return FindFileNext(false);
        }
    }

    // filter on m_sWildcardFilter
    else if (!m_sWildcardFilter.IsEmpty() && StrT::MatchRegEx<FILECHAR_t>(m_FileEntry.m_sFileName, m_sWildcardFilter, false) > 0) {  // IgnoreCase ?
        return FindFileNext(false);                                                                                                  // Skip
    }

    // Match

    if (m_bReadStats) {  // some dirs don't have stat() ability. e.g. "/proc"
        // dirent() doesn't have this stuff...it's in stat()
        cFileStatusSys statusSys;
        const HRESULT hRes = cFileStatus::GetStatusSys(statusSys, get_FilePath(), this->m_nFileFlags & FOF_X_FollowLinks);
        if (FAILED(hRes)) return hRes;
        m_FileEntry.InitFileStatus(statusSys);
    }
#endif

    m_FileEntry.UpdateLinuxHidden(m_FileEntry.m_sFileName);
    return S_OK;
}

HRESULT cFileFind::FindOpen(const FILECHAR_t* pszDirPath, const FILECHAR_t* pszWildcardFile) {
    //! start a sequential read of the files in a list of possible matches.
    //! @arg pszWildcardFile = "*.ext". if pszDirPath is empty, full path can be in pszWildcardFile
    //! @note pszWildcardFile can NOT have multiple "*.ext1;*.ext2"
    //! @return
    //!  HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no files.

    CloseContext();
    ASSERT(!isContextOpen());

    // Combine pszDirPath and pszWildcardFile
    if (pszDirPath != nullptr) {
        m_sDirPath = pszDirPath;  // store this. but may already be set.
    }
    cStringF sWildcardFile;  // if it is part of directory?
    if (m_sDirPath.IsEmpty()) {
        // full path can be in pszWildcardFile. break it up.
        if (StrT::IsNullOrEmpty(pszWildcardFile)) {
            return HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND);
        }
        m_sDirPath = cFilePath::GetFileDir(pszWildcardFile);
        pszWildcardFile = cFilePath::GetFileName(StrT::ToSpanStr(pszWildcardFile));  // this specific file. or may have wildcards.
    } else if (StrT::IsNullOrEmpty(pszWildcardFile)) {                               // NOTE: NT doesn't need this but 98 does ?
        // ASSUME m_sDirPath is just a directory path. Find all files in this path.
        if (cFilePath::HasTitleWildcards(m_sDirPath.get_SpanStr())) {
            // NOTE: we should have put this in pszWildcardFile! This is wrong. avoid ambiguous queries.
            sWildcardFile = cFilePath::GetFileName(m_sDirPath.get_SpanStr());
            pszWildcardFile = sWildcardFile;
            m_sDirPath = cFilePath::GetFileDir(m_sDirPath);
        } else {
            pszWildcardFile = _FN("*");  // All files in this directory = default.
        }
    }

#ifdef _WIN32
    // in _WIN32 wildcard filter is built in.
    cMem::Zero(&m_FindInfo, sizeof(m_FindInfo));
    m_FindInfo.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;  // docs say this is not needed

    cStringF sFilePath = GetFilePath(pszWildcardFile);
    m_hContext = ::FindFirstFileW(cFilePath::GetFileNameLongW(sFilePath), &m_FindInfo);

#elif defined(__linux__)
    // in Linux wildcard filter is done later/manually.
    m_sWildcardFilter = pszWildcardFile;

    // Need to strip out the *.EXT part. just need the dir name here.
    m_hContext = ::opendir(m_sDirPath.get_CPtr());
#endif

    if (!isContextOpen()) {
        return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND));
    }

    return S_OK;
}

HRESULT cFileFind::FindFile(const FILECHAR_t* pszDirPath, const FILECHAR_t* pszWildcardFile) {
    //! Start a sequential read of the files in a list of possible matches.
    //! @arg pszWildcardFile = "*.ext". if pszDirPath is empty, full path can be in pszWildcardFile
    //! @return
    //!  HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no files.
    HRESULT hRes = FindOpen(pszDirPath, pszWildcardFile);
    if (FAILED(hRes)) return hRes;
    return FindFileNext(true);
}

//************************************************************

HRESULT GRAYCALL cFileDir::CreateDirectory1(const FILECHAR_t* pszDirName) {  // static
    //! HRESULT_WIN32_C(ERROR_ALREADY_EXISTS) is OK ?
    //! use CreateDirectory1 name because might be "#define CreateDirectory CreateDirectoryA" in _WIN32
    //! Does NOT create missing parent folders.

#ifdef _WIN32
    if (!::CreateDirectoryW(cFilePath::GetFileNameLongW(pszDirName), nullptr))
#elif defined(__linux__)
    if (::mkdir(pszDirName, 0777) != 0)  // mode_t
#endif
    {
        HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
        if (hRes == HRESULT_WIN32_C(ERROR_ALREADY_EXISTS))  // this is OK.
            return S_FALSE;
        return hRes;
    }
    return S_OK;
}
HRESULT GRAYCALL cFileDir::RemoveDirectory1(const FILECHAR_t* pszDirName) {  // static
    //! @note will fail if directory is not empty.
    //! use RemoveDirectory1 name because might be "#define RemoveDirectory RemoveDirectoryA" in _WIN32
#ifdef _WIN32
    if (!::RemoveDirectoryW(cFilePath::GetFileNameLongW(pszDirName)))
#elif defined(__linux__)
    if (::rmdir(pszDirName) != 0)        // mode_t
#endif
    {
        return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
    }
    return S_OK;
}

//********************************************

HRESULT cFileDir::ReadDir(const FILECHAR_t* pszDirPath, const FILECHAR_t* pszWildcardFile, ITERATE_t iFilesMax, bool bFollowLink) {
    //! calls the virtual AddFileDirEntry()
    //! @arg
    //!  pszDirPath = the base directory. nullptr = current dir.
    //!  pszWildcardFile = may use a filter like: *.scp. may contain full path if pszDirPath is empty. nullptr = all.
    //! @note It seems _WIN32 does NOT like the trailing "\" alone
    //!  e.g. "d:\menace\scripts\" is bad ! use "d:\menace\scripts\*.*"
    //! @return
    //!  <0 = error.
    //!  number of files.

    if (pszDirPath != nullptr) {
        m_sDirPath = pszDirPath;  // store this
    }

    cFileFind state(m_sDirPath, bFollowLink);
    HRESULT hRes = state.FindFile(nullptr, pszWildcardFile);
    if (FAILED(hRes)) {
        if (hRes == HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) || hRes == HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND))  // no files.
            return 0;
        return hRes;
    }

    m_sDirPath = state.get_DirPath();  // in case real path is in pszWildcardFile
    ITERATE_t iFiles = 0;
    while (iFiles < iFilesMax) {
        hRes = AddFileDirEntry(state.m_FileEntry);
        if (FAILED(hRes)) continue;
        iFiles++;
        hRes = state.FindFileNext();
        if (FAILED(hRes)) break;  // HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) is normal.
    }

    return CastN(HRESULT, iFiles);  // number of files.
}

HRESULT cFileDir::ReadDirAnyExt(const FILECHAR_t* pszFilePath, ITERATE_t iFilesMax) {
    //! Find this file name but with any extension.
    //! @return # files found with this file title.
    return ReadDir(cFilePath::GetFileDir(pszFilePath), cFilePath::GetNameExtStar(StrT::ToSpanStr(pszFilePath)), iFilesMax);
}

HRESULT cFileDir::ReadDirPreferredExt(const FILECHAR_t* pszFilePath, const cSpan<const FILECHAR_t*> exts) {
    //! Find just a single file with the preferred extension from a list.
    //! Ignore any existing extension
    //! @arg
    //!  pszExtTable  = null terminated table. extension has dots.
    //! @return
    //!  the enum of the extension we got. 0 - N in pszExtTable
    //!  HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND) = nothing found.

    static const int k_ExtMax = 32;

    HRESULT hFiles = ReadDirAnyExt(pszFilePath, k_ExtMax);
    if (FAILED(hFiles)) return hFiles;
    if (hFiles == 0) return HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND);

    // Filter them.
    int iFound = 0;
    cFileDirEntry* aEntries[k_ExtMax];  // sorted by extension.
    cMem::Zero(aEntries, sizeof(aEntries));
    for (int i = 0; i < (int)hFiles; i++) {
        const FILECHAR_t* pszExt = cFilePath::GetFileNameExt(m_aFiles.GetAt(i).get_Name().get_SpanStr());
        if (pszExt == nullptr) continue;
        const ITERATE_t iExt = StrT::SpanFind(pszExt, exts);
        if (IS_INDEX_BAD_ARRAY(iExt, aEntries)) continue;
        if (iExt >= (ITERATE_t)_countof(aEntries)) continue;  // overflow !
        aEntries[iExt] = &m_aFiles[i];
        iFound++;
    }

    if (iFound <= 0) {
        RemoveAll();
        return HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND);
    }

    for (UINT i = 0; i < _countof(aEntries); i++) {
        if (aEntries[i] == nullptr) continue;
        m_aFiles[0] = *aEntries[i];
        m_aFiles.SetSize(1);
        return i;  // what entry in pszExtTable
    }

    ASSERT(0);
    return HRESULT_WIN32_C(ERROR_INTERNAL_ERROR);
}

//*************************************************

HRESULT GRAYCALL cFileDir::CreateDirectoryX(const FILECHAR_t* pszDir, StrLen_t iStart) {  // static
    //! This is like CreateDirectory1() except will create intermediate/parent directories if needed.
    //! @note like SHCreateDirectoryExA() but we can't always use that since thats only for Win2K+
    //! @return S_FALSE = already exists. equiv to ERROR_ALREADY_EXISTS. NOT a real error.

    if (pszDir == nullptr) return E_POINTER;
    StrLen_t iEnd = iStart ? iStart : cFilePath::GetFilePathDeviceLen(pszDir);
    if (cFilePath::IsCharDirSep(pszDir[iEnd])) iEnd++;

    FILECHAR_t szDirTmp[cFilePath::k_MaxLen];
    if (iEnd >= STRMAX(szDirTmp)) iEnd = STRMAX(szDirTmp);
    StrT::CopyLen(szDirTmp, pszDir, iEnd + 1);

    HRESULT hRes = S_FALSE;
    for (;;) {
        // Get dir name from path.
        iStart = iEnd;
        FILECHAR_t ch;
        for (;;) {
            if (iEnd >= STRMAX(szDirTmp)) return HRESULT_WIN32_C(ERROR_BUFFER_OVERFLOW);
            ch = pszDir[iEnd];
            szDirTmp[iEnd] = ch;
            if (ch == '\0') break;
            iEnd++;
            if (cFilePath::IsCharDirSep(ch)) break;
        }
        if (iStart >= iEnd) {
            if (ch == '\0') break;                       // done!
            return HRESULT_WIN32_C(ERROR_BAD_PATHNAME);  // bad name!
        }
        szDirTmp[iEnd] = '\0';
        hRes = CreateDirectory1(szDirTmp);
        if (FAILED(hRes)) {
            if (hRes == HRESULT_WIN32_C(ERROR_ACCESS_DENIED) && iStart == 0)  // create a drive? 'c:\' type thing.
                continue;
            return hRes;
        }
    }

    return hRes;
}

HRESULT GRAYCALL cFileDir::CreateDirForFileX(const FILECHAR_t* pszFilePath, StrLen_t iStart) {  // static
    //! CreateDirectoryX() for a file. will create intermediate/parent directories if needed.
    return CreateDirectoryX(cFilePath::GetFileDir(pszFilePath), iStart);
}

HRESULT GRAYCALL cFileDir::MovePathToTrash(const FILECHAR_t* pszPath, bool bDir) {  // static
    //! Move file/directory to the trash bin/folder.
    //! For use with FOF_ALLOWUNDO and FILEOP_t::_Delete
    //! like WIN32 SHFileOperation(FOF_ALLOWUNDO);

    UNREFERENCED_PARAMETER(pszPath);
    cStringF sDirTrash;
#if defined(UNDER_CE)
#error UNDER_CE
#elif defined(_WIN32)
    FILECHAR_t szPath[cFilePath::k_MaxLen];
    cAppState::GetFolderPath(CSIDL_BITBUCKET, szPath); 
    sDirTrash = szPath;
#elif defined(__linux__)
    // https://www.freedesktop.org/wiki/Specifications/trash-spec/
    sDirTrash = cAppState::GetEnvironStr("XDG_DATA_HOME");
    if (!StrT::IsWhitespace<FILECHAR_t>(sDirTrash)) {
        sDirTrash = cFilePath::CombineFilePathX(sDirTrash, _FN("Trash"));
    }
#else
#error NOOS
#endif

    // Make sure there is no collision.

    if (bDir) {
        return DirFileOps(FILEOP_t::_Move, pszPath, sDirTrash, FILEOPF_t::_None, nullptr, nullptr);
    } else {
        return cFileCopier::RenamePath(pszPath, cFilePath::CombineFilePathX(sDirTrash, cFilePath::GetFileName(StrT::ToSpanStr(pszPath))), nullptr);
    }
}

HRESULT GRAYCALL cFileDir::DirFileOps(FILEOP_t eOp, const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, FILEOPF_t nFileFlags, cLogProcessor* pLog, IStreamProgressCallback* pProgress) {
    //! Copy, Delete or Move a directory AND all files in the directory (pszDirSrc) to pszDirDest. with recursive descent.
    //! @arg pszDirSrc = full path.
    //! @arg nFileFlags = FOF_ALLOWUNDO, FOF_FILESONLY, FOF_RENAMEONCOLLISION, FILEOPF_t
    //! @return <0 or S_OK = nothing to do.
    //!  Number of files moved/deleted.

    const FILECHAR_t* pszWildcards = nullptr;
    cStringF sDirSrc;
    if (cFilePath::HasTitleWildcards(StrT::ToSpanStr(pszDirSrc))) {
        pszWildcards = cFilePath::GetFileName(StrT::ToSpanStr(pszDirSrc));
        sDirSrc = cFilePath::GetFileDir(pszDirSrc);
        pszDirSrc = sDirSrc;
    }

    switch (eOp) {
        case FILEOP_t::_Rename:
            if (pszDirDest == nullptr || pszWildcards != nullptr) return E_INVALIDARG;  // don't know how to deal with this.
            // Wildcards in pszDirSrc might do strange things!
            break;
        case FILEOP_t::_Delete:
            if (pszWildcards == nullptr) pszWildcards = pszDirDest;
            break;
        default:
            break;
    }

    cFileDir fileDir;
    const HRESULT hResCount = fileDir.ReadDir(pszDirSrc, pszWildcards);
    if (FAILED(hResCount)) return hResCount;
    if (hResCount <= 0) return S_OK;  // no files moved.

    HRESULT hRes = S_OK;
    if (eOp == FILEOP_t::_Move || eOp == FILEOP_t::_Copy) {
        hRes = CreateDirectoryX(pszDirDest);
        if (FAILED(hRes)) return hRes;
    }

    int iErrors = 0;
    for (const cFileDirEntry& fileEntry : fileDir.m_aFiles) {
        // Move each of the files.
        if (nFileFlags & FOF_FILESONLY) {
            if (fileEntry.isAttrDir()) continue;
        }

        cStringF sFileTitle = fileEntry.get_Name();
        cStringF sFilePathSrc = fileDir.GetFilePath(sFileTitle);
        cStringF sFilePathDst;
        if (eOp == FILEOP_t::_Move || eOp == FILEOP_t::_Copy || eOp == FILEOP_t::_Rename) {
            sFilePathDst = cFilePath::CombineFilePathX(pszDirDest, sFileTitle);
        }

        if (fileEntry.isAttrDir()) {
            // Recursive descent.
            if (eOp == FILEOP_t::_Delete) {
                if (StrT::IsWhitespace(pszWildcards)) {
                    if (nFileFlags & FOF_ALLOWUNDO) {  // move whole dir to the trash.
                        // hRes = MovePathToTrash(sFilePathSrc,true);
                    }
                } else {
                    sFilePathDst = pszWildcards;
                }
            }
            hRes = DirFileOps(eOp, sFilePathSrc, sFilePathDst, nFileFlags, pLog, pProgress);
            if (FAILED(hRes)) return hRes;
        } else {
            switch (eOp) {
                case FILEOP_t::_Move:
                case FILEOP_t::_Rename:
                    hRes = cFileCopier::RenamePath(sFilePathSrc, sFilePathDst, pProgress);
                    break;
                case FILEOP_t::_Copy:
                    hRes = cFileCopier::CopyFileX(sFilePathSrc, sFilePathDst, pProgress);
                    break;
                case FILEOP_t::_Delete:
                    if (nFileFlags & FOF_ALLOWUNDO) {
                        // hRes = MovePathToTrash(sFilePathSrc,false);
                    }
                    hRes = cFile::DeletePathX(sFilePathSrc, nFileFlags);
                    break;
            }
            if (FAILED(hRes)) {
                // Record that it failed!
                ++iErrors;
                if (pLog != nullptr) {
                    pLog->addEventF(LOG_ATTR_INIT, LOGLVL_t::_ERROR, "%s\"%s\" ERR=\"%s\". '%s' to '%s'.", LOGSTR(k_szCantMoveFile),
                                    LOGSTR(sFileTitle),  // cFilePath::MakeRelativePath( sFilePathDst, pszDirDest )
                                    LOGERR(hRes), LOGSTR(fileDir.get_DirPath()), LOGSTR(pszDirDest));
                }
            }
        }
    }

    if (eOp == FILEOP_t::_Move || (eOp == FILEOP_t::_Delete && StrT::IsWhitespace(pszWildcards))) {
        hRes = RemoveDirectory1(pszDirSrc);
    }

    return hResCount;
}

HRESULT GRAYCALL cFileDir::DeletePathX(const FILECHAR_t* pszPath, FILEOPF_t nFileFlags) {  // static
    //! Delete this file or directory. If it's a directory then delete recursively.
    //! No wildcards.
    cFileStatus fileStatus;
    HRESULT hRes = fileStatus.ReadFileStatus(pszPath);
    if (SUCCEEDED(hRes)) {
        if (nFileFlags & FOF_FILESONLY) {
            if (fileStatus.isAttrDir()) return S_FALSE;
        }
        if (nFileFlags & FOF_ALLOWUNDO) {
            // hRes = MovePathToTrash(sFilePathSrc,fileStatus.isAttrDir());
        }
        if (fileStatus.isAttrDir()) {
            hRes = DeleteDirFiles(pszPath, nullptr, nFileFlags);
        } else {
            hRes = cFile::DeletePathX(pszPath, nFileFlags);
        }
    }
    if (hRes == HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND)) return S_FALSE;
    return hRes;
}
}  // namespace Gray
